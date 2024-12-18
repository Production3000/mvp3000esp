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
#include "NetWebSockets.h"

#include "NetWeb_HtmlStrings.h"
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
         * @brief Set an alternate page as root and move the main MVP3000 page to a sub-uri.
         * 
         * @param alternateResponseFiller The function to fill the alternate page.
         * @param alternateTemplateProcessor (optional) The function to process the alternate page template.
         * @param mvpUri (optional) The URI of the main MVP3000 page.
         */
        void setAlternateRoot(AwsResponseFiller alternateResponseFiller, std::function<String (uint16_t)> alternateTemplateProcessor, const String& mvpUri)  { altResponseFiller = alternateResponseFiller; altTemplateProcessor = alternateTemplateProcessor; rootUri = mvpUri; }

    public:

        void setup();
        void loop();

        NetWebSockets webSockets = NetWebSockets(server);

    private:

        AsyncWebServer server = AsyncWebServer(80);

        LinkedListWebActions linkedListWebActions = LinkedListWebActions(); // Adaptive size
        LinkedListWebCfg linkedListWebCfg = LinkedListWebCfg(); // Adaptive size

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

        int8_t requestedModuleIndex = -1; // None selected, or there is just no module
        void serveModulePage(AsyncWebServerRequest *request);

        String rootUri = "/";
        AwsResponseFiller altResponseFiller = nullptr;
        std::function<String (uint16_t)> altTemplateProcessor = nullptr;

        size_t responseFillerHome(uint8_t *buffer, size_t maxLen, size_t index);
        size_t extendedResponseFiller(PGM_P html, uint8_t *buffer, size_t maxLen, size_t index);
        String templateProcessorWrapper(const String& var);

};

#endif