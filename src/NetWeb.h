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


#ifdef ESP8266
    // #include <ESP8266WebServer.h>
    #include <ESPAsyncWebServer.h>
#else
    #include <WebServer.h>
    // #include <ESPAsyncWebServer.h>                                                       // TODO
#endif

#include "ESPX.h"
#ifdef ESP8266
    extern EspClass ESPX;
#else
    extern EspClassX ESPX;
#endif

// #include "_Helper.h"
// #include "_LinkedList.h"
#include "Config_JsonInterface.h"


class NetWeb {
    public:

        struct WebPageList {
            struct Node {
                String url;
                AwsResponseFiller responseFiller;
                AwsTemplateProcessor processor;
                Node* next;
            };
            Node* head = nullptr;
            
            /**
             * @brief Add a web page to the web interface.
             * 
             * @param url The URL of the page. IMPORTANT: include the '/' at the beginning.
             * @param responseFiller The function to fill the response.
             * @param processor The function to process the template.
             */
            void add(String url, AwsResponseFiller responseFiller, AwsTemplateProcessor processor) {
                Node* newNode = new Node();
                newNode->url = url;
                newNode->responseFiller = responseFiller;
                newNode->processor = processor;
                newNode->next = head;
                head = newNode;
            }

            Node* selectPage(String url) {
                Node* current = head;
                while (current != nullptr) {
                    if (current->url == url) {
                        return current;
                    }
                    current = current->next;
                }
                return nullptr;
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

        void registerPage(String url, AwsResponseFiller responseFiller, AwsTemplateProcessor processor);
        void registerCfg(CfgJsonInterface* Cfg);
        void registerAction(String action, WebActionList::ResponseType successResponse, std::function<bool(int, std::function<String(int)>, std::function<String(int)>)> actionFkt, String successMessage = "");


    // DELETE
        void sendFormatted(const char* formatString, ...) {} // Used in modules contentHome(), string length limited to WEB_CHUNK_LENGTH
        void responseRedirect(const char* message); // Used in modules editCfg(), startAction()     DELETE
        void responsePrepareRestart() { };
        



    private:

        #ifdef ESP8266
            // ESP8266WebServer server;
            AsyncWebServer server = AsyncWebServer(80);
        #else
            WebServer server;
        #endif

        WebPageList webPageList;
        WebCfgList webCfgList;
        WebActionList webActionList;

        // Message to serve on next page load after form save
        const char *postMessage = "";

        // Handle form action (post)
        void editCfg(AsyncWebServerRequest *request);
        void startAction(AsyncWebServerRequest *request);
        bool formInputCheck(AsyncWebServerRequest *request);

        void responseRedirect(AsyncWebServerRequest *request, const char* message = "");
        void responsePrepareRestart(AsyncWebServerRequest *request);

        const char* index_html = R"RAW(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %1%</title>
    <script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
    <style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h1>MVP3000 - Device ID %1%</h1> <h3 style='color: red;'>%2%</h3>
    <h3>System</h3> <ul>
        <li>ID: %1%</li>
        <li>Build: %3%</li>
        <li>Memory: %4%, fragmentation %5%&percnt;</li>
        <li>Uptime: %6%</li>
        <li>Last restart reason: %7%</li>
        <li>CPU frequency: %8% MHz</li>
        <li>Main loop duration: %9% ms (mean/min/max)</li> </ul>
    <h3>Network</h3> <ul>
        <li>Fallback AP SSID: '%31%'</li>
        <li>Network credentials: leave SSID empty to remove, any changes are applied at once.<br> <form action='/start' method='post' onsubmit='return confirm(`Change network?`);'> <input name='setwifi' type='hidden'> SSID <input name='newSsid' value='%32%'> Passphrase <input type='password' name='newPass' value='%33%'> <input type='submit' value='Save'> </form> </li>
        <li>Reconnect tries: <br> <form action='/save' method='post'> <input name='clientConnectRetries' type='number' value='%34%' min='1' max='255'> <input type='submit' value='Save'> </form> </li>
        <li>Force client mode. WARNING: If the credentials are wrong, the device will be inaccessible via network, thus require re-flashing!
         <form action='/checksave' method='post' onsubmit='return promptId(this);'> <input name='forceClientMode' type='checkbox' %35% value='1'> <input name='forceClientMode' type='hidden' value='0'> <input name='deviceId' type='hidden'> <input type='submit' value='Save'> </form> </li> </ul>
    <h3>MQTT Communication</h3> <ul>
        <li>Status: %51% </li>
        <li>Auto-discovery port local broker: 1024-65535, default is 4211.<br> <form action='/save' method='post'> <input name='discoveryPort' value='%52%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li>
        <li>Forced external broker:<br> <form action='/save' method='post'> <input name='mqttForcedBroker' value='%53%'> <input type='submit' value='Save'> </form> </li>
        <li>MQTT port: default is 1883 (unsecure) <br> <form action='/save' method='post'> <input name='mqttPort' value='%54%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li>
        <li>Topic: <br> <form action='/save' method='post'> %55% <input name='mqttTopicSuffix' value='%56%' minlength='5'> <input type='submit' value='Save'> </form> </li> </ul>
    <h3>Modules</h3> <ul>
        %99% </ul>
    <h3>Maintenance</h3> <ul>
        <li> <form action='/start' method='post' onsubmit='return confirm(`Restart?`);'> <input name='restart' type='hidden'> <input type='submit' value='Restart' > </form> </li>
        <li> <form action='/checkstart' method='post' onsubmit='return promptId(this);'> <input name='resetdevice' type='hidden'> <input name='deviceId' type='hidden'> <input type='submit' value='Factory reset'> </form> </li> </ul>
<p>&nbsp;</body></html>
)RAW";

};

#endif