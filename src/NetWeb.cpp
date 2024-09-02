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

    // Register home page
    registerPage("/", webPage, std::bind(&NetWeb::webPageProcessor, this, std::placeholders::_1));

    // Register actions
    registerAction("restart", [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
        mvp.delayedRestart(25); // Restarts after 25 ms
        return true;
    });
    registerAction("reset", [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
        mvp.config.asyncFactoryResetDevice((args == 3) && (argKey(2) == "keepwifi")); // If keepwifi is checked it is present in the args, otherwise not
        return true;
    }, "Factory reset initiated, this takes some 10 s ...");
    
    // Start server, independent of wifi status
    server.begin();
}

void NetWeb::loop() {
    // There is actually nothing to do here, the async server is running in the background
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::registerAction(String action, std::function<bool(int, std::function<String(int)>, std::function<String(int)>)> actionFkt) {
    webActionList.add(action, WebActionList::ResponseType::RESTART, actionFkt, "");
}

void NetWeb::registerAction(String action, std::function<bool(int, std::function<String(int)>, std::function<String(int)>)> actionFkt, String successMessage) {
    webActionList.add(action, WebActionList::ResponseType::MESSAGE, actionFkt, successMessage);
};

void NetWeb::registerCfg(CfgJsonInterface *Cfg) {
    webCfgList.add(Cfg);
}

void NetWeb::registerPage(String uri, const char* html, AwsTemplateProcessor processor, String type) {
    if (!webPageColl.add(uri, html, processor, type))
        mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Too many pages registered, max %d", WebPageColl::nodesSize);
}

void NetWeb::registerPage(String uri, AwsResponseFiller responseFiller, String type) {
    if (!webPageColl.add(uri, responseFiller, type))
        mvp.logger.writeFormatted(CfgLogger::Level::ERROR, "Too many pages registered, max %d", WebPageColl::nodesSize);
}

std::function<void(const String &message)> NetWeb::registerWebSocket(String uri, std::function<void(char*)> dataCallback) {
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

    // Loops through all actions and executes it if found
    WebActionList::Node* result = webActionList.loopActions(request->params(), [&](int i) { return request->getParam(i)->name(); }, [&](int i) { return request->getParam(i)->value(); });

    // Not found or failed
    if (result == nullptr) {
        responseRedirect(request, "Invalid input or action not found!");
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid action/input from: %s", request->client()->remoteIP().toString().c_str());
        return;
    }

    // Report success or restart
    switch (result->successResponse) {
        case WebActionList::ResponseType::MESSAGE:
            responseRedirect(request, result->successMessage.c_str());
            break;
        case WebActionList::ResponseType::RESTART:
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
            if ( (mvp.helper.isValidInteger(request->getParam("deviceId", true)->value())) && (request->getParam("deviceId", true)->value().toInt() == ESPX.getChipId()) ) {
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
    // Message to serve on next page load                              // TODO there should be a timeout to get rid of this message, like quarter a second max as webload ist fast                  
    postMessage = message;

    // Redirect to avoid post reload, 303 temporary
                                                                        // For modules this does not redirect to the module but to home
    request->redirect("/");                                            // TODO redirect for modules after save/action
}

void NetWeb::responseMetaRefresh(AsyncWebServerRequest *request) {
    // http-equiv seems to not show up in history
    request->send(200, "text/html", "<!DOCTYPE html> <head> <meta http-equiv='refresh' content='4;url=/'> </head> <body> <h3 style='color: red;'>Restarting ...</h3> </body> </html>");
}


///////////////////////////////////////////////////////////////////////////////////

String NetWeb::webPageProcessor(const String& var) {
    if (!mvp.helper.isValidInteger(var)) {
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid placeholder in template: %s", var.c_str());
        return var;
    }

    String str; // Needs to be defined outside of switch
    switch (var.toInt()) {
        case 0:
            return String(ESPX.getChipId());

        case 1:
            str = postMessage;
            postMessage = ""; // Clear message for next load
            return str;
        case 2:
            return String(__DATE__) + " " + String(__TIME__);
        case 3:
            return String(ESP.getFreeHeap());
        case 4:
            return String(ESPX.getHeapFragmentation());
        case 5:
            return String(mvp.helper.upTime());
        case 6:
            return String(ESPX.getResetReason().c_str());
        case 7:
            return String(ESP.getCpuFreqMHz());
        case 8:
            return String(mvp.loopDurationMean_ms) + " / " + String(mvp.loopDurationMin_ms) + " / " + String(mvp.loopDurationMax_ms);

        case 11:
            if (mvp.net.netCom.cfgNetCom.mqttEnabled) {
                return "<a href='/netcom'>MQTT communication</a></li>";
            } else {
                return "MQTT communication (disabled)</li>";
            }

        case 21:
            if (mvp.moduleCount == 0)
                return "<li>None</li>";
            for (uint8_t i = 0; i < mvp.moduleCount; i++) {
                char message[128];
                if ((mvp.xmodules[i]->uri).length() > 0) {
                    snprintf(message, sizeof(message), "<li><a href='%s'>%s</a></li>", mvp.xmodules[i]->uri.c_str(), mvp.xmodules[i]->description.c_str());
                } else {
                    snprintf(message, sizeof(message), "<li>%s</li>", mvp.xmodules[i]->description.c_str());
                }
                str += message;
            }
            return str;

        default:
            break;
    }
    mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid placeholder in template: %s", var.c_str());
    return var;
}
