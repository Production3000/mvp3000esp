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

        struct WebPage {
            String uri;
            const char* html;
            String type;
            AwsResponseFiller responseFiller;
            AwsTemplateProcessor processor;

            /**
             * @brief Construct a new Web Page object
             * 
             * Make sure there is no unencoded percent symbol in the html string or any of replacment strings as it messes up the placeholder parsing.
             * The ESPAsyncWebServer URL matching is not perfect. If /example is defined, /example/something will yield crazy results.
             * 
             * @param uri The URI of the page.
             * @param html The HTML content of the page.
             * @param responseFiller The response filler to use for the page. Warning, it is called a very last time after the string is already at its end, probably to make sure everything was sent
             * @param processor The processor to use for the page.
             * @param type The url-content type of the connection. Use 'application/octet-stream' for data to force download in browser.
             */
            WebPage() { }
            WebPage(String _uri, const char* _html, AwsTemplateProcessor _processor, String _type = "text/html") : uri(_uri), html(_html), processor(_processor), type(_type) {
                // Generate the response filler for the provided html string
                responseFiller = [&](uint8_t *buffer, size_t maxLen, size_t index)-> size_t {
                    // Put the substring from html into the buffer, starting at index until index + maxLen
                    size_t len = strlen(html);
                    if (index + maxLen > len) {
                        maxLen = len - index;
                    }
                    memcpy(buffer, html + index, maxLen);
                    return maxLen;
                };
            }
            WebPage(String _uri, AwsResponseFiller _responseFiller, String _type = "text/html") : uri(_uri), responseFiller(_responseFiller), type(_type) { }
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

        void registerPage(WebPage& webPage);
        void registerCfg(CfgJsonInterface* Cfg);
        void registerAction(String action, WebActionList::ResponseType successResponse, std::function<bool(int, std::function<String(int)>, std::function<String(int)>)> actionFkt, String successMessage = "");

    private:

        AsyncWebServer server = AsyncWebServer(80);
        AsyncWebSocket websocket = AsyncWebSocket("/ws");

        // Message to serve on next page load after form save
        const char *postMessage = "";

        WebCfgList webCfgList;
        WebActionList webActionList;

        WebPage* webPageHome;

        // Handle form action (post)
        void editCfg(AsyncWebServerRequest *request);
        void startAction(AsyncWebServerRequest *request);
        bool formInputCheck(AsyncWebServerRequest *request);

        void responseRedirect(AsyncWebServerRequest *request, const char* message = "");
        void responsePrepareRestart(AsyncWebServerRequest *request);

};

#endif