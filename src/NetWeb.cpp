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
    // Module folders are registered seperately
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
        request->sendChunked("text/html", std::bind(&NetWeb::responseFillerHome, this, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3), std::bind(&NetWeb::templateProcessorWrapper, this, std::placeholders::_1) );
    });
    server.on("/save", std::bind(&NetWeb::editCfg, this, std::placeholders::_1));
    server.on("/checksave", std::bind(&NetWeb::editCfg, this, std::placeholders::_1));
    server.on("/start", std::bind(&NetWeb::startAction, this, std::placeholders::_1));
    server.on("/checkstart", std::bind(&NetWeb::startAction, this, std::placeholders::_1));
    server.onNotFound([&](AsyncWebServerRequest *request) { // Catch all
        request->redirect("/");
    });

    // Set the save function for the config
    linkedListWebCfg.setSaveCfgFkt([&](CfgJsonInterface &cfg) { mvp.config.writeCfg(cfg); });

    // Start server, independent of wifi status
    server.begin();
}

void NetWeb::loop() {
    // There is actually nothing to do here, the async server is running in the background
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

void NetWeb::registerWebSocket(const String& uri, WebSocketCtrlCallback ctrlCallback) {
    linkedListWebSocket.appendUnique(uri, ctrlCallback, std::bind(&NetWeb::webSocketEventCallbackWrapper, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), &server);
};


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::editCfg(AsyncWebServerRequest *request) {
    if (!formInputCheck(request)) {
        return;
    }
    // This is always a single setting that is updated at a time, try update and respond
    if (linkedListWebCfg.updateSetting(request->getParam(0)->name(), request->getParam(0)->value())) {
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

void NetWeb::webSocketPrint(const String& topic, const String& message) {
    linkedListWebSocket.webSocketPrint(topic, message);
}

void NetWeb::webSocketEventCallbackWrapper(AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len, WebSocketCtrlCallback ctrlCallback) {
    switch (type) {
        case WS_EVT_CONNECT:
            mvp.logger.writeFormatted(CfgLogger::Level::INFO, "WS client %d connected from: %s", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            mvp.logger.writeFormatted(CfgLogger::Level::INFO, "WS client %d disconnected.", client->id()); // No IP available
            break;
        case WS_EVT_ERROR:
            mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "WS error from: %s, client %d", client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DATA:
            mvp.logger.writeFormatted(CfgLogger::Level::INFO, "WS client %d data from: %s", client->id(), client->remoteIP().toString().c_str());
            if (ctrlCallback != nullptr) { // Only parse data if there is something to do
                AwsFrameInfo *info = (AwsFrameInfo*)arg;
                if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                    data[len] = 0; // Terminate string
                    // Execute callback
                    ctrlCallback((char*)data);
                }
            }
            break;
        default: // WS_EVT_PONG
            break;
    }
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
    request->send(200, "text/html", webPageRedirect);
}

void NetWeb::serveModulePage(AsyncWebServerRequest *request) {
    // Finde the matching module
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
    if (index < strlen(webPageHead))
        return extendedResponseFiller(webPageHead, buffer, maxLen, index);
    index -= strlen(webPageHead);
    // Main
    if (index < strlen(mvp.webPage))
        return extendedResponseFiller(mvp.webPage, buffer, maxLen, index);
    index -= strlen(mvp.webPage);
    // Log
    if (index < strlen(mvp.logger.webPage))
        return extendedResponseFiller(mvp.logger.webPage, buffer, maxLen, index);
    index -= strlen(mvp.logger.webPage);
    // Network
    if (index < strlen(mvp.net.webPage))
        return extendedResponseFiller(mvp.net.webPage, buffer, maxLen, index);
    index -= strlen(mvp.net.webPage);
    // UDP
    if (index < strlen(mvp.net.netCom.webPage))
        return extendedResponseFiller(mvp.net.netCom.webPage, buffer, maxLen, index);
    index -= strlen(mvp.net.netCom.webPage);
    // MQTT
    if (index < strlen(mvp.net.netMqtt.webPage))
        return extendedResponseFiller(mvp.net.netMqtt.webPage, buffer, maxLen, index);
    // Footer
    index -= strlen(mvp.net.netMqtt.webPage);
    return extendedResponseFiller(webPageFoot, buffer, maxLen, index);
}

size_t NetWeb::extendedResponseFiller(const char* html, uint8_t *buffer, size_t maxLen, size_t index) {
    // Chunked response filler for the html template
    size_t len = strlen(html);
    if (index + maxLen > len) {
        maxLen = len - index;
    }
    memcpy(buffer, html + index, maxLen);
    return maxLen;
}

String NetWeb::templateProcessorWrapper(const String& var) {
    if (!_helper.isValidInteger(var)) {
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid placeholder in template: %s", var.c_str());
        return "[" + var + "]";
    }
    int32_t varInt = var.toInt();
    switch (varInt) {

        // Main placeholders
        case 0: // Standard HTML head
            return webPageHead;
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
        case 9: // Standard HTML foot
            return webPageFoot;

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

        //  Xmodules placeholders
        case 100 ... 255:
            if (requestedModuleIndex != -1)
                return mvp.xmodules[0]->webPageProcessor(varInt);
            return "";

        default:
            return "";
    }
}
