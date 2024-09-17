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
    // Initialize folders/requests
    // Root folder / and all module folders are initialized with registering the pages
    // IMPORTANT: /foo is matched by foo, foo/, /foo/bar, /foo?bar - but not by /foobar
    server.on("/save", std::bind(&NetWeb::editCfg, this, std::placeholders::_1));
    server.on("/checksave", std::bind(&NetWeb::editCfg, this, std::placeholders::_1));
    server.on("/start", std::bind(&NetWeb::startAction, this, std::placeholders::_1));
    server.on("/checkstart", std::bind(&NetWeb::startAction, this, std::placeholders::_1));
    server.onNotFound([&](AsyncWebServerRequest *request) { // Catch all
        request->redirect("/");
    });

    // Initialize cfgList
    webCfgList = WebCfgList([&](CfgJsonInterface &cfg) { mvp.config.writeCfg(cfg); });

    
    // Start server, independent of wifi status
    server.begin();
}

void NetWeb::loop() {
    // There is actually nothing to do here, the async server is running in the background
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::registerAction(const String& actionKey, WebActionFunction actionFkt) {
    linkedListWebActions.appendUnique(actionKey, DataStructWebAction::ResponseType::RESTART, actionFkt, "");
}

void NetWeb::registerAction(const String& actionKey, WebActionFunction actionFkt, const String& successMessage) {
    linkedListWebActions.appendUnique(actionKey, DataStructWebAction::ResponseType::MESSAGE, actionFkt, successMessage);
};

void NetWeb::registerCfg(CfgJsonInterface *Cfg, std::function<void()> callback) {
    webCfgList.add(Cfg, callback);
}

void NetWeb::registerPage(String uri, const char* html, AwsTemplateProcessorInt processor, String type) {
    if (!webPageColl.add(uri, html, processor, type))
        mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Too many pages registered, max %d", WebPageColl::nodesSize);
}

void NetWeb::registerPage(String uri, AwsResponseFiller responseFiller, String type) {
    if (!webPageColl.add(uri, responseFiller, type))
        mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Too many pages registered, max %d", WebPageColl::nodesSize);
}

std::function<void(const String &message)> NetWeb::registerWebSocket(String uri, WebSocketDataCallback dataCallback) {
    if (!webSocketColl.add(uri, dataCallback)) {
        mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Too many websockets registered, max %d", WebSocketColl::nodesSize);
        return nullptr;
    }
    return webSocketColl.getTextAll();
};

void NetWeb::webSocketEventLog(AsyncWebSocketClient *client, AwsEventType type) {
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
            break;
        default: // WS_EVT_PONG
            break;
    }
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::editCfg(AsyncWebServerRequest *request) {
    if (!formInputCheck(request)) {
        return;
    }

    // Try to change setting and respond
    if (webCfgList.loopUpdateSingleValue(request->getParam(0)->name(), request->getParam(0)->value())) {
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

    if (!webAction->actionFkt(request->params(), [&](int i) { return request->getParam(i)->name(); }, [&](int i) { return request->getParam(i)->value(); })) {
        // Execution failed
        responseRedirect(request, "Invalid action input!");
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid action input from: %s", request->client()->remoteIP().toString().c_str());
        return;
    }

    // Report success or restart
    switch (webAction->successResponse) {
        case DataStructWebAction::ResponseType::MESSAGE:
            responseRedirect(request, webAction->successMessage.c_str());
            break;
        case DataStructWebAction::ResponseType::RESTART:
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
            if ( (_helper.isValidInteger(request->getParam("deviceId", true)->value())) && (request->getParam("deviceId", true)->value().toInt() == ESPX.getChipId()) ) {
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

void NetWeb::responseRedirect(AsyncWebServerRequest *request, const char *message) {
    // Message to serve on next page load and timestamp to discard if it is too old       
    postMessage = message;
    postMessageTime = millis();

    // Redirect to avoid post reload, 303 temporary
    // Points to the referer [sic] to stay on the page the form was on 
    request->redirect(request->header("Referer"));
}

void NetWeb::responseMetaRefresh(AsyncWebServerRequest *request) {
    // http-equiv seems to not show up in history
    request->send(200, "text/html", "<!DOCTYPE html> <head> <meta http-equiv='refresh' content='4;url=/'> </head> <body> <h3 style='color: red;'>Restarting ...</h3> </body> </html>");
}


///////////////////////////////////////////////////////////////////////////////////

String NetWeb::webPageProcessorMain(const String& var, AwsTemplateProcessorInt processorCustom) {
    if (!_helper.isValidInteger(var)) {
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid placeholder in template: %s", var.c_str());
        return "[" + var + "]";
    }

    String str; // Needs to be defined outside of switch
    switch (var.toInt()) {
        // Main placeholders

        case 0: // Post message
            // Discard if it is too old, clear message for next load
            if (millis() < postMessageTime + postMessageLifetime) {
                str = postMessage;
            }
            postMessage = "";
            return str;
        case 1: // Device ID
            return String(ESPX.getChipId());
        case 2: // Device IP
            return mvp.net.myIp.toString();

        // Custom placeholders, core framework starts at 10+, Xmodules should start at 100+
        default:
            return processorCustom(var.toInt());
            // Sadly there is no way to know if no placeholder was matched or if the string is just empty
            // We could encode it, but this would just make it more complex to implement in new templates
            // mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Unknown placeholder in template: %s", var.c_str());
    }
}

