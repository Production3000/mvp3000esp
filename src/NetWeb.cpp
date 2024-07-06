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
    // Initialize cfgList
    webCfgList = WebCfgList([&](CfgJsonInterface &cfg) { mvp.config.writeCfg(cfg); });

    // Initialize actionList
    webActionList.add("restart", WebActionList::ResponseType::RESTART, [&](int args, std::function<String(int)> argName, std::function<String(int)> argValue) {
        return true;
    });
    webActionList.add("resetdevice", WebActionList::ResponseType::RESTART, [&](int args, std::function<String(int)> argName, std::function<String(int)> argValue) {
        mvp.config.factoryResetDevice(); // also calls restart, but whatever
        return true;
    });

    // Folders/requests
    server.on("/", HTTP_ANY, std::bind(&NetWeb::serveRequest, this, std::placeholders::_1));
    server.on("/save", std::bind(&NetWeb::editCfg, this, std::placeholders::_1));
    server.on("/checksave", std::bind(&NetWeb::editCfg, this, std::placeholders::_1));
    server.on("/start", std::bind(&NetWeb::startAction, this, std::placeholders::_1));
    server.on("/checkstart", std::bind(&NetWeb::startAction, this, std::placeholders::_1));
    server.onNotFound(std::bind(&NetWeb::serveRequestCaptureAll, this, std::placeholders::_1));

    // Start server, independent of wifi status
    server.begin();
}

void NetWeb::loop() {
    // Called from net.loop() only if network is up
    // There is actually nothing to do here, the async server is running in the background
}


///////////////////////////////////////////////////////////////////////////////////


//         static String processor(const String& var);
// String NetWeb::processor(const String& var) {
    // switch (var.toInt()) {
    //     case 1:
    //         return ESPX.getChipId();
    //     case 2:
    //         return postMessage;
    //     default:
    //         break;
    // }
//     return String();
// }

void NetWeb::serveRequestCaptureAll(AsyncWebServerRequest *request) {
    if (mvp.helper.isValidInteger(request->url().substring(1))) {
        Serial.println("module");
        serveRequest(request);
        // Serve module content
        // uint8_t moduleId = request->url().toInt();
        // if (moduleId < mvp.moduleCount) {
        //     if (mvp.xmodules[moduleId]->enableContentModuleNetWeb) {
        //         mvp.xmodules[moduleId]->contentModuleNetWeb(request);
        //         return;
        //     }
        // }
    } else {
        // Serve main page
        serveRequest(request);
    }
}

void NetWeb::serveRequest(AsyncWebServerRequest *request) {
    response = request->beginChunkedResponse("text/html", [&](uint8_t *buffer, size_t maxLen, size_t index)-> size_t {
        // Put the substring from index_html into the buffer, starting at index until index + maxLen
        size_t len = strlen(index_html);
        if (index + maxLen > len) {
            maxLen = len - index;
        }
        memcpy(buffer, index_html + index, maxLen);
        return maxLen;
    } , [&](const String& var) -> String {
        String modules;
        switch (var.toInt()) {
            case 1:
                return String(ESPX.getChipId());
            case 2:
                return postMessage;
            case 3:
                return String(__DATE__) + " " + String(__TIME__);
            case 4:
                return String(ESP.getFreeHeap());
            case 5:
                return String(ESPX.getHeapFragmentation());
            case 6:
                return String(mvp.helper.upTime());
            case 7:
                return String(ESPX.getResetReason().c_str());
            case 8:
                return String(ESP.getCpuFreqMHz());
            case 9:
                return String(mvp.loopDurationMean_ms) + " / " + String(mvp.loopDurationMin_ms) + " / " + String(mvp.loopDurationMax_ms);
            
            case 31:
                return mvp.net.apSsid.c_str();
            case 32:
                return mvp.net.cfgNet.clientSsid.c_str();
            case 33:
                return mvp.net.cfgNet.clientPass.c_str();
            case 34:
                return String(mvp.net.cfgNet.clientConnectRetries);
            case 35:
                return (mvp.net.cfgNet.forceClientMode == true) ? "checked" : "";

            case 51:
                return mvp.net.netCom.controllerConnectedString().c_str();
            case 52:
                return String(mvp.net.netCom.cfgNetCom.discoveryPort);
            case 53:
                return mvp.net.netCom.cfgNetCom.mqttForcedBroker.c_str();
            case 54:
                return String(mvp.net.netCom.cfgNetCom.mqttPort);
            case 55:
                return mvp.net.netCom.mqttTopicPrefix.c_str();
            case 56:
                return mvp.net.netCom.cfgNetCom.mqttTopicSuffix.c_str();

            case 99:
                for (uint8_t i = 0; i < mvp.moduleCount; i++) {                                        // TODO make this string thing much better this is so annoying in c++
                    if (mvp.xmodules[i]->enableContentModuleNetWeb)
                        modules += "<li><a href='/" + String(i) + "'>" + mvp.xmodules[i]->description + "</a></li>";
                    else
                        modules += "<li>" + mvp.xmodules[i]->description + "</li>";
                }
                if (mvp.moduleCount == 0)
                    modules += "<li>None</li>";
                return modules;

            default:
                break;
        }
        return String();
    });

    request->send(response);

    // Clear message for next load
    postMessage = "";
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

    // Report success and 
    switch (result->successResonse)   {
        case WebActionList::ResponseType::MESSAGE:
            responseRedirect(request, result->successMessage.c_str());
            break;
        case WebActionList::ResponseType::RESTART:
            responsePrepareRestart(request); // Restarts after 25 ms, page reloads after 4 s
            break;
        
        default:
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
        if (request->hasParam("deviceId")) {
            if ( (mvp.helper.isValidInteger(request->getParam("deviceId")->value())) && (request->getParam("deviceId")->value().toInt() == ESPX.getChipId()) )
                return true;
        } else {
            // Failed confirmation check
            mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Invalid deviceId input from: %s",  request->client()->remoteIP().toString().c_str());
            return false;
        }
    }
    return true;
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::responseRedirect(const char* message) {                                         // TODO DELETE
    // // Message to serve on next page load
    // postMessage = message;

    // // Redirect to avoid post reload, 303 temporary
    // // For modules this does not redirect to the module but to home
    // server.sendHeader("Location", "/");
    // server.send(303);
}

void NetWeb::responseRedirect(AsyncWebServerRequest *request, const char *message) {
    // Message to serve on next page load                              // TODO there should be a timeout to get rid of this message, like quarter a second max as webload ist fast                  
    postMessage = message;

    // Redirect to avoid post reload, 303 temporary
                                                                        // For modules this does not redirect to the module but to home
    request->redirect("/");                                            // TODO redirect for modules after save/action
}

void NetWeb::responsePrepareRestart(AsyncWebServerRequest *request) {
    // http-equiv seems to not show up in history
    request->send(200, "text/html", "<!DOCTYPE html> <head> <meta http-equiv='refresh' content='4;url=/'> </head> <body> <h3 style='color: red;'>Restarting ...</h3> </body> </html>");
    // Initiate delayed restart
    mvp.delayedRestart(25);
}
