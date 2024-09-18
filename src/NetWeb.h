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
        void setup();
        void loop();

        /**
         * @brief Register a new page for the web interface.
         * 
         * @param uri The URI of the page.
         * @param html The HTML content of the page, used together with the processor to fill the template.
         * @param processor The processor function for the page.
         * @param responseFiller The response filler function for the page, typically used for large pages or datasets.
         * @param contentType The content type of the page, optional, Default is "text/html".
         */
        void registerPage(const String& uri, const char* html, AwsTemplateProcessorInt processor, const String& contentType = "text/html");
        void registerPage(const String& uri, AwsResponseFiller responseFiller, const String& contentType = "text/html");

        /**
         * @brief Register a configuration interface to make its settings editable using a form on the web interface.
         * 
         * @param Cfg The configuration to add.
         */
        void registerCfg(CfgJsonInterface* Cfg, std::function<void()> callback = nullptr);

        /**
         * @brief Register an action to be executed using a form on the web interface.
         * 
         * @param actionKey The actionKey to add.
         * @param actionFkt The function to execute.
         * @param successMessage (optional) The message to display on success. Leave empty to not display a message. If omitted, the device will display a 'restarting'-page, but not restart itself automatically.
         */
        void registerAction(const String& actionKey, WebActionFunction actionFkt); // One cannot overload bool with String
        void registerAction(const String& actionKey, WebActionFunction actionFkt, const String& successMessage);

        /**
         * @brief Register a websocket to be used with the web interface.
         * 
         * @param uri The URI of the websocket.
         * @param dataCallback The function to execute when data is received.
         * @return Returns the function to write data to the websocket.
         */
        std::function<void(const String& message)> registerWebSocket(const String& uri) { return registerWebSocket(uri, nullptr); };
        std::function<void(const String& message)> registerWebSocket(const String& uri, WebSocketDataCallback dataCallback);

    private:

        AsyncWebServer server = AsyncWebServer(80);

        // Message to serve on next page load after form save
        const char *postMessage = "";
        uint64_t postMessageTime = 0;
        uint16_t postMessageLifetime = 15000; // 15 seconds

        LinkedListWebActions linkedListWebActions = LinkedListWebActions(); // Adaptive size
        LinkedListWebCfg linkedListWebCfg = LinkedListWebCfg(); // Adaptive size

        String webPageProcessorMain(const String& var, AwsTemplateProcessorInt processorCustom);
        WebPageColl webPageColl = WebPageColl(&server, std::bind(&NetWeb::webPageProcessorMain, this, std::placeholders::_1, std::placeholders::_2));

        void webSocketEventLog(AsyncWebSocketClient *client, AwsEventType type);
        WebSocketColl webSocketColl = WebSocketColl(&server, std::bind(&NetWeb::webSocketEventLog, this, std::placeholders::_1, std::placeholders::_2));

        // Handle form action (post)
        void editCfg(AsyncWebServerRequest *request);
        void startAction(AsyncWebServerRequest *request);
        bool formInputCheck(AsyncWebServerRequest *request);

        void responseRedirect(AsyncWebServerRequest *request, const char* message = "");
        void responseMetaRefresh(AsyncWebServerRequest *request);

};

#endif