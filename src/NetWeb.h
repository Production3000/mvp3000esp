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

#include "Config_JsonInterface.h"
#include "NetWeb_WebStructs.h"


class NetWeb {
    public:
        /**
         * @brief Register an action to be executed using a form on the web interface.
         *
         * @param actionKey The actionKey to add.
         * @param actionCallback The function to execute.
         * @param successMessage (optional) The message to display on success. Leave empty to not display a message. If omitted, the device will display a 'restarting'-page, but not restart itself automatically.
         */
        void registerAction(const String& actionKey, WebActionCallback actionCallback); // One cannot overload bool with String
        void registerAction(const String& actionKey, WebActionCallback actionCallback, const String& successMessage);

        /**
         * @brief Register a configuration interface to make its settings editable using a form on the web interface.
         *
         * @param Cfg The configuration to add.
         * @param callback (optional) The function to execute after the configuration has been saved. Leave empty to not execute a function.
         */
        void registerCfg(CfgJsonInterface* Cfg, std::function<void()> callback = nullptr);

        /**
         * @brief Register a new page for the web interface.
         *
         * @param uri The URI of the page.
         * @param onRequest The function to execute when the page is requested.
         */
        void registerFillerPage(const String& uri, ArRequestHandlerFunction onRequest);

        /**
         * @brief Register a new module page for the web interface.
         *
         * @param uri The URI of the page.
         */
        void registerModulePage(const String& uri);

        /**
         * @brief Register a websocket to be used with the web interface.
         *
         * @param uri The URI of the websocket.
         * @param dataCallback (optional) The function to execute when data is received. Leave empty to not execute a function.
         * @return Returns the function to write data to the websocket.
         */
        void registerWebSocket(const String& uri, WebSocketCtrlCallback dataCallback = nullptr);

    public:

        void setup();
        void loop();

        void webSocketPrint(const String& topic, const String& message);

    private:

        AsyncWebServer server = AsyncWebServer(80);

        LinkedListWebActions linkedListWebActions = LinkedListWebActions(); // Adaptive size
        LinkedListWebCfg linkedListWebCfg = LinkedListWebCfg(); // Adaptive size
        LinkedListWebSocket linkedListWebSocket; // Adaptive size

        // Message to serve on next page load after form save
        const char* postMessage = "";
        uint64_t postMessageExpiry = 0;
        uint16_t postMessageLifetime = 15000; // 15 seconds

        // Handle form action (post)
        void editCfg(AsyncWebServerRequest *request);
        void startAction(AsyncWebServerRequest *request);
        bool formInputCheck(AsyncWebServerRequest *request);

        void responseRedirect(AsyncWebServerRequest *request, const char* message = "");
        void responseMetaRefresh(AsyncWebServerRequest *request);

        void webSocketEventCallbackWrapper(AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len, WebSocketCtrlCallback ctrlCallback);

        int8_t requestedModuleIndex = -1; // None selected, or there is just no module
        void serveModulePage(AsyncWebServerRequest *request);

        size_t responseFillerHome(uint8_t *buffer, size_t maxLen, size_t index);
        size_t extendedResponseFiller(const char* html, uint8_t *buffer, size_t maxLen, size_t index);
        String templateProcessorWrapper(const String& var);

        const char* webPageHead = R"===(<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %1%</title>
<script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
<style>body { font-family: sans-serif; } table { border-collapse: collapse; border-style: hidden; } td { border: 1px solid black; vertical-align:top; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h2>MVP3000 - Device ID %1%</h2> <h3 style='color: red;'>%3%</h3>)===";

        const char* webPageFoot = "<p>&nbsp;</body></html>";

        const char* webPageRedirect = "<!DOCTYPE html> <head> <meta http-equiv='refresh' content='4;url=/'> </head> <body> <h3 style='color: red;'>Restarting ...</h3> </body> </html>";

};

#endif