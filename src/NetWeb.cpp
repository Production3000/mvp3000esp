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

#include "NetWeb.h"

#include "MVP3000.h"
extern MVP3000 mvp;

#include "_Helper.h"
extern _Helper _helper;


void NetWeb::setup() {
    // IMPORTANT: /foo is matched by foo, foo/, /foo/bar, /foo?bar - but not by /foobar

    // Main mvp page, uri is root or moved if alternate root page is set
    server.on(rootUri.c_str(), HTTP_GET, [&](AsyncWebServerRequest *request) {
        request->sendChunked("text/html", std::bind(&NetWeb::responseFillerHome, this, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3), std::bind(&NetWeb::templateProcessorWrapper, this, std::placeholders::_1) );
    });
    
    // Set alternate root page if set
    if (!rootUri.equals("/")) {
        server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
            request->sendChunked("text/html", altResponseFiller, std::bind(&NetWeb::templateProcessorWrapper, this, std::placeholders::_1) );
        });
    }

    // Form actions
    server.on("/save", std::bind(&NetWeb::editCfg, this, std::placeholders::_1));
    server.on("/checksave", std::bind(&NetWeb::editCfg, this, std::placeholders::_1));
    server.on("/start", std::bind(&NetWeb::startAction, this, std::placeholders::_1));
    server.on("/checkstart", std::bind(&NetWeb::startAction, this, std::placeholders::_1));

    // Module folders are registered separately

    // Catch all redirect to root
    server.onNotFound([&](AsyncWebServerRequest *request) {
        request->redirect("/");
    });

    // Start server, independent of wifi status
    server.begin();
}

void NetWeb::loop() {
    // The async server is running in the background, nothing to do here

    // WebSocket controll callbacks need to be executed in the main loop
    webSockets.loop();
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::registerAction(const String& actionKey, WebActionCallback actionCallback) {
    linkedListWebActions.appendUnique(actionKey, LinkedListWebActions::ResponseType::RESTART, actionCallback, "");
}

void NetWeb::registerAction(const String& actionKey, WebActionCallback actionCallback, const String& successMessage) {
    linkedListWebActions.appendUnique(actionKey, LinkedListWebActions::ResponseType::MESSAGE, actionCallback, successMessage);
};

void NetWeb::registerCfg(CfgJsonInterface *cfg, std::function<void()> callback) {
    linkedListWebCfg.append(cfg, callback);
}

void NetWeb::registerFillerPage(const String& uri, ArRequestHandlerFunction onRequest) {
    server.on(uri.c_str(), HTTP_GET, onRequest);
}

void NetWeb::registerModulePage(const String& uri) {
    // Death of to many lambdas, no place has all info and objects get destroyed
    //  1. Bind the onRequest to NetWeb
    //  2. Use the request->url to select (index of) the module
    //  3. Store the index in NetWeb
    //  4. Create the request send using he index to point to the html text
    //  5. The template processor also uses the index
    // NOTE: this of course breaks two simultaneous requests to different modules
    server.on(uri.c_str(), HTTP_GET, std::bind(&NetWeb::serveModulePage, this, std::placeholders::_1));
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::editCfg(AsyncWebServerRequest *request) {
    if (!formInputCheck(request)) {
        return;
    }
    // This is always a single setting that is updated at a time, try update and respond
    if (linkedListWebCfg.updateSetting(request->getParam(0)->name(), request->getParam(0)->value(), [&](CfgJsonInterface &cfg) { mvp.config.writeCfg(cfg); })) {
        responseRedirect(request, "Settings saved!");
    } else {
        responseRedirect(request, "Input error!");
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid form input from: %s", request->client()->remoteIP().toString().c_str());
    }
}

void NetWeb::startAction(AsyncWebServerRequest *request) {
    if (!formInputCheck(request)) {
        return;
    }

    DataStructWebAction* webAction = linkedListWebActions.findAction(request->getParam(0)->name());

    if (webAction == nullptr) {
        // Not found
        responseRedirect(request, "Action not found!");
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Action not found from: %s", request->client()->remoteIP().toString().c_str());
        return;
    }

    if (!webAction->actionCallback(request->params(), [&](int i) { return request->getParam(i)->name(); }, [&](int i) { return request->getParam(i)->value(); })) {
        // Execution failed
        responseRedirect(request, "Invalid action input!");
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid action input from: %s", request->client()->remoteIP().toString().c_str());
        return;
    }

    // Report success or restart
    switch (webAction->responseType) {
        case LinkedListWebActions::ResponseType::MESSAGE:
            responseRedirect(request, webAction->successMessage.c_str());
            break;
        case LinkedListWebActions::ResponseType::RESTART:
            responseMetaRefresh(request); // Restarts after 25 ms, page reloads after 4 s
            break;

        default:// WebActionList::ResponseType::NONE - should not occur
            responseRedirect(request);
            break;
    }
}

bool NetWeb::formInputCheck(AsyncWebServerRequest *request) {
    if (request->params() == 0) { // Likely a reload-from-locationbar error
        responseRedirect(request, "Redirected ...");
        return false;
    }

    // Double check for deviceId for confirmation
    if (request->url().substring(1,6) == "check") {
        if (request->hasParam("deviceId", true)) {
            if ( (_helper.isValidInteger(request->getParam("deviceId", true)->value())) && (request->getParam("deviceId", true)->value().toInt() == _helper.ESPX->getChipId()) ) {
                return true;
            }
        }
    } else {
        // No deviceId check
        return true;
    }

    // Failed confirmation check or no deviceId provided
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Invalid deviceId input from: %s",  request->client()->remoteIP().toString().c_str());
    return false;
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::responseRedirect(AsyncWebServerRequest *request, const char* message) {
    // Message to serve on next page load and set expiry time
    postMessage = message;
    postMessageExpiry = millis() + postMessageLifetime;

    // Redirect to avoid post reload, 303 temporary
    // Points to the referer [sic] to stay on the page the form was on
    request->redirect(request->header("Referer"));
}

void NetWeb::responseMetaRefresh(AsyncWebServerRequest *request) {
    // http-equiv seems to not show up in history
    request->send(200, "text/html", htmlRedirect);
}

void NetWeb::serveModulePage(AsyncWebServerRequest *request) {
    // Find the matching module
    requestedModuleIndex = -1;
    for (uint8_t i = 0; i < mvp.moduleCount; i++) {
        if (mvp.xmodules[i]->uri.equals(request->url())) {
            requestedModuleIndex = i;
            break;
        }
    }
    if (requestedModuleIndex == -1) // If the register function is used this can never happen
        request->redirect("/");

    request->sendChunked("text/html", [&](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        return extendedResponseFiller(mvp.xmodules[requestedModuleIndex]->getWebPage(), buffer, maxLen, index);
    }, std::bind(&NetWeb::templateProcessorWrapper, this, std::placeholders::_1));
}



///////////////////////////////////////////////////////////////////////////////////

size_t NetWeb::responseFillerHome(uint8_t *buffer, size_t maxLen, size_t index) {
    // Head
    if (index < strlen_P(htmlHead))
        return extendedResponseFiller(htmlHead, buffer, maxLen, index);
    index -= strlen_P(htmlHead);
    // Main
    if (index < strlen_P(htmlSystem))
        return extendedResponseFiller(htmlSystem, buffer, maxLen, index);
    index -= strlen_P(htmlSystem);
    // Log
    if (index < strlen_P(mvp.logger.getHtml()))
        return extendedResponseFiller(htmlLogger, buffer, maxLen, index);
    index -= strlen_P(mvp.logger.getHtml());
    // Network
    if (index < strlen_P(htmlNet))
        return extendedResponseFiller(htmlNet, buffer, maxLen, index);
    index -= strlen_P(htmlNet);
    // WebSocket
    if (index < strlen_P(mvp.net.netWeb.webSockets.getHtml()))
        return extendedResponseFiller(mvp.net.netWeb.webSockets.getHtml(), buffer, maxLen, index);
    index -= strlen_P(mvp.net.netWeb.webSockets.getHtml());
    // MQTT
    if (index < strlen_P(mvp.net.netMqtt.getHtml()))
        return extendedResponseFiller(mvp.net.netMqtt.getHtml(), buffer, maxLen, index);
    index -= strlen_P(mvp.net.netMqtt.getHtml());
    // UDP
    if (index < strlen_P(mvp.net.netCom.getHtml()))
        return extendedResponseFiller(mvp.net.netCom.getHtml(), buffer, maxLen, index);
    index -= strlen_P(mvp.net.netCom.getHtml());
    // Footer
    return extendedResponseFiller(htmlFoot, buffer, maxLen, index);
}

size_t NetWeb::extendedResponseFiller(PGM_P html, uint8_t *buffer, size_t maxLen, size_t index) {
    // Chunked response filler for the html template
    size_t len = strlen_P(html);
    if (index + maxLen > len) {
        maxLen = len - index;
    }
    memcpy_P(buffer, html + index, maxLen);
    return maxLen;
}

String NetWeb::templateProcessorWrapper(const String& var) {
    if (!_helper.isValidInteger(var)) {
        mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Invalid placeholder in template: %s", var.c_str());
        return "[PLACEHOLDERERROR]";
    }
    uint16_t varInt = var.toInt();
    switch (varInt) {

        // Main placeholders
        case 0: // Standard HTML head
            return String(htmlHead);
        case 1: // Device ID
            return String(_helper.ESPX->getChipId());
        case 2: // Device IP
            return mvp.net.myIp.toString();
        case 3: // Post message
            // Send message if within lifetime
            if (millis() < postMessageExpiry) {
                postMessageExpiry = 0; // Expire message for next load, saves a string copy
                return postMessage;
            }
            return "";
        case 4: // Main settings url, / or alternate root
            return rootUri;
        case 9: // Standard HTML foot
            return String(htmlFoot);

        // Class placeholders
        case 10 ... 29: // System
            return mvp.templateProcessor(varInt);
        case 30 ... 39: // Log
            return mvp.logger.templateProcessor(varInt);
        case 40 ... 49: // Network
            return mvp.net.templateProcessor(varInt);
        case 50 ... 59: // UDP
            return mvp.net.netCom.templateProcessor(varInt);
        case 60 ... 79: // MQTT
            return mvp.net.netMqtt.templateProcessor(varInt);
        case 80 ... 89: // WebSockets
            return mvp.net.netWeb.webSockets.templateProcessor(varInt);

        //  Xmodules placeholders
        case 100 ... 255:
            if (requestedModuleIndex != -1)
                return mvp.xmodules[0]->webPageProcessor(varInt);
            return "";

        default:
            if (altTemplateProcessor != nullptr) {
                return altTemplateProcessor(varInt);
            }
            return "";
    }
}
