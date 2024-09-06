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
        void registerPage(String uri, const char* html, AwsTemplateProcessor processor, String contentType = "text/html");
        void registerPage(String uri, AwsResponseFiller responseFiller, String contentType = "text/html");

        /**
         * @brief Register a configuration interface to make its settings editable using a form on the web interface.
         * 
         * @param Cfg The configuration to add.
         */
        void registerCfg(CfgJsonInterface* Cfg, std::function<void()> callback = nullptr);

        /**
         * @brief Register an action to be executed using a form on the web interface.
         * 
         * @param action The action to add.
         * @param actionFkt The function to execute.
         * @param successMessage (optional) The message to display on success. Leave empty to not display a message. If omitted, the device will display a 'restarting'-page, but not restart itself automatically.
         */
        void registerAction(String action, std::function<bool(int, std::function<String(int)>, std::function<String(int)>)> actionFkt); // One cannot overload bool with String
        void registerAction(String action, std::function<bool(int, std::function<String(int)>, std::function<String(int)>)> actionFkt, String successMessage);

        /**
         * @brief Register a websocket to be used with the web interface.
         * 
         * @param uri The URI of the websocket.
         * @param dataCallback The function to execute when data is received.
         * @return Returns the function to write data to the websocket.
         */
        std::function<void(const String &message)> registerWebSocket(String uri) { return registerWebSocket(uri, nullptr); };
        std::function<void(const String &message)> registerWebSocket(String uri, std::function<void(char*)> dataCallback);        // should return a function to write data to the websocket

    private:

        AsyncWebServer server = AsyncWebServer(80);

        // Message to serve on next page load after form save
        const char *postMessage = "";

        WebActionList webActionList;

        WebCfgList webCfgList;

        WebPageColl webPageColl = WebPageColl(&server);

        void webSocketEventLog(AsyncWebSocketClient *client, AwsEventType type);
        WebSocketColl webSocketColl = WebSocketColl(&server, std::bind(&NetWeb::webSocketEventLog, this, std::placeholders::_1, std::placeholders::_2));

        // Handle form action (post)
        void editCfg(AsyncWebServerRequest *request);
        void startAction(AsyncWebServerRequest *request);
        bool formInputCheck(AsyncWebServerRequest *request);

        void responseRedirect(AsyncWebServerRequest *request, const char* message = "");
        void responseMetaRefresh(AsyncWebServerRequest *request);

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
        <li>Log websocket: ws://%9%/wslog </li>
        <li><a href='/net'>Network</a></li>
        <li> %11% </li>
        <li><a href='/netmqtt'>MQTT</a></li> </ul>
    <h3>Modules</h3> <ul>
        %21% </ul>
    <h3>Maintenance</h3> <ul>
        <li> <form action='/start' method='post' onsubmit='return confirm(`Restart?`);'> <input name='restart' type='hidden'> <input type='submit' value='Restart' > </form> </li>
        <li> <form action='/checkstart' method='post' onsubmit='return promptId(this);'> <input name='reset' type='hidden'> <input name='deviceId' type='hidden'> <input type='submit' value='Factory reset'> <input type='checkbox' name='keepwifi' checked value='1'> keep Wifi </form> </li> </ul>
<p>&nbsp;</body></html>
)===";

};

#endif