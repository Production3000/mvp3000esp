/*
Copyright Production 3000

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef MVP3000_NETWEB
#define MVP3000_NETWEB

#include <Arduino.h>

#include <ESPAsyncWebServer.h>

#include "ESPX.h"
#ifdef ESP8266
    extern EspClass ESPX;
#else
    extern EspClassX ESPX;
#endif

#include "Config_JsonInterface.h"


class NetWeb {
    public:

        struct WebPageList {
            struct Node {
                String uri;
                const char* html;
                String type;
                AwsResponseFiller responseFiller;
                AwsTemplateProcessor processor;

                Node () { }
                Node(String uri, const char* html, String type, AwsTemplateProcessor processor) : uri(uri), html(html), type(type), processor(processor) { 
                    responseFiller = std::bind(&Node::htmlTemplateResponseFiller, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
                }
                Node(String uri, String type, AwsResponseFiller responseFiller) : uri(uri), html(""), type(type), responseFiller(responseFiller), processor(nullptr) { }

                size_t htmlTemplateResponseFiller(uint8_t *buffer, size_t maxLen, size_t index) {
                    // Chunked response filler for the html template
                    size_t len = strlen(html);
                    if (index + maxLen > len) {
                        maxLen = len - index;
                    }
                    memcpy(buffer, html + index, maxLen);
                    return maxLen;
                }
            };

            uint8_t nodeCount = 0;
            static const uint8_t nodesSize = 10;
            Node* nodes[nodesSize];

            uint8_t add(String uri, const char* _html, AwsTemplateProcessor processor, String type) {
                if (nodeCount >= nodesSize) {
                    return 255;
                }
                nodes[nodeCount] = new Node(uri, _html, type, processor);
                return nodeCount++; // Cool, this return the value before incrementing
            }

            uint8_t add(String uri, AwsResponseFiller responseFiller, String type) {
                if (nodeCount >= nodesSize) {
                    return 255;
                }
                nodes[nodeCount] = new Node(uri, type, responseFiller);
                return nodeCount++;
            }

        };

        struct WebCfgList {
            struct Node {
                CfgJsonInterface* Cfg;
                Node* next;
            };
            Node* head = nullptr;

            std::function<void(CfgJsonInterface&)> saveCfgFkt; // Function to save the configuration

            WebCfgList() { }
            WebCfgList(std::function<void(CfgJsonInterface&)> saveCfgFkt) {
                this->saveCfgFkt = saveCfgFkt;
            }

            /**
             * @brief Make a configuration available for web interface.
             * 
             * @param Cfg The configuration to add.
             */
            void add(CfgJsonInterface* Cfg) {
                Node* newNode = new Node;
                newNode->Cfg = Cfg;
                newNode->next = head;
                head = newNode;
            }

            // Loops through all cfgs and updates the value if found
            bool loopUpdateSingleValue(String key, String value) {
                Node* current = head;
                // Loop through all nodes
                while (current != nullptr) {
                    // Try to update value, if successful save Cfg and return
                    if (current->Cfg->updateSingleValue(key, value)) {
                        saveCfgFkt(*current->Cfg);
                        return true;
                    }
                    current = current->next;
                }
                return false;
            }
        };

        struct WebActionList {
            enum ResponseType {
                NONE = 0,
                MESSAGE = 1,
                RESTART = 2
            };

            struct Node {
                String action;
                ResponseType successResponse;
                String successMessage;
                std::function<bool(int, std::function<String(int)>, std::function<String(int)>)> actionFkt;
                Node* next;
            };
            Node* head = nullptr;

            WebActionList() { }

            /**
             * @brief Make a configuration available for web interface.
             * 
             * @param action The action of the configuration.
             * @param successResponse The response type on success.
             * @param actionFkt The function to execute.
             * @param successMessage The message to display on success.
             */
            void add(String action, ResponseType successResponse, std::function<bool(int, std::function<String(int)>, std::function<String(int)>)> actionFkt, String successMessage = "") {
                Node* newNode = new Node;
                newNode->action = action;
                newNode->successResponse = successResponse;
                newNode->successMessage = successMessage;
                newNode->actionFkt = actionFkt;
                newNode->next = head;
                head = newNode;
            }

            // Loops through all cfgs and updates the value if found
            Node* loopActions(int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
                Node* current = head;
                // Loop through all nodes
                while (current != nullptr) {
                    // Check if key matches and execute action
                    if (argKey(0) == current->action) {
                        if (current->actionFkt(args, argKey, argValue)) {
                            return current;
                        } else {
                            return nullptr;
                        }
                    }
                    current = current->next;
                }
                return nullptr;
            }
        };

        void setup();
        void loop();

        void registerPage(String uri, const char* html, AwsTemplateProcessor processor, String type = "text/html");
        void registerPage(String uri, AwsResponseFiller responseFiller, String type = "text/html");

        void registerPage(uint8_t nodeIndex);

        void registerCfg(CfgJsonInterface* Cfg);
        void registerAction(String action, WebActionList::ResponseType successResponse, std::function<bool(int, std::function<String(int)>, std::function<String(int)>)> actionFkt, String successMessage = "");

    private:

        AsyncWebServer server = AsyncWebServer(80);
        AsyncWebSocket websocket = AsyncWebSocket("/ws");

        // Message to serve on next page load after form save
        const char *postMessage = "";

        WebPageList webPageList;
        WebCfgList webCfgList;
        WebActionList webActionList;

        // Handle form action (post)
        void editCfg(AsyncWebServerRequest *request);
        void startAction(AsyncWebServerRequest *request);
        bool formInputCheck(AsyncWebServerRequest *request);

        void responseRedirect(AsyncWebServerRequest *request, const char* message = "");
        void responsePrepareRestart(AsyncWebServerRequest *request);

        String webPageProcessor(const String& var);

        char* webPage = R"===(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %0%</title>
    <script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
    <style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h2>MVP3000 - Device ID %0%</h2> <h3 style='color: red;'>%1%</h3>
    <h3>System</h3> <ul>
        <li>ID: %0%</li>
        <li>Build: %2%</li>
        <li>Memory: %3%, fragmentation %4%&percnt;</li>
        <li>Uptime: %5%</li>
        <li>Last restart reason: %6%</li>
        <li>CPU frequency: %7% MHz</li>
        <li>Main loop duration: %8% ms (mean/min/max)</li> </ul>
    <h3>Components</h3> <ul>
        <li><a href='/net'>Network</a></li>
        <li>%11% </ul>
    <h3>Modules</h3> <ul>
        %21% </ul>
    <h3>Maintenance</h3> <ul>
        <li> <form action='/start' method='post' onsubmit='return confirm(`Restart?`);'> <input name='restart' type='hidden'> <input type='submit' value='Restart' > </form> </li>
        <li> <form action='/checkstart' method='post' onsubmit='return promptId(this);'> <input name='reset' type='hidden'> <input name='deviceId' type='hidden'> <input type='submit' value='Factory reset'> <input type='checkbox' name='keepwifi' checked value='1'> keep Wifi </form> </li> </ul>
<p>&nbsp;</body></html>
        )===";

};

#endif