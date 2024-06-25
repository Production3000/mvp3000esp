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
    // Folders/requests
    server.on("/", std::bind(&NetWeb::serveRequest, this));
    server.on("/save", std::bind(&NetWeb::editCfg, this, false));
    server.on("/savecheck", std::bind(&NetWeb::editCfg, this, true));
    server.on("/start", std::bind(&NetWeb::startAction, this, false));
    server.on("/startcheck", std::bind(&NetWeb::startAction, this, true));
    server.onNotFound(std::bind(&NetWeb::serveRequest, this));

    // Start server, independent of wifi status, main will only be called when connected
    server.begin();
}

void NetWeb::loop() {
    // Called from net.loop() only if network is up

    // Handle page requests
    server.handleClient();
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::serveRequest() {
    // Prepare and head
    contentStart();

    // Serve main/module content
    if ((server.args() == 1) && (server.argName(0) == "m") && (mvp.helper.isValidInteger(server.arg(0))) && (server.arg(0).toInt() < mvp.moduleCount)) {
        mvp.xmodules[server.arg(0).toInt()]->contentModuleNetWeb();
    } else {
        contentHome();
    }

    // Close page
    contentClose();

    // Client IP is obviously empty after stop()
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Serving page to: %s",  server.client().remoteIP().toString().c_str());

    // Stop connection, not sure if needed
    server.client().stop();
}

void NetWeb::contentStart() {
    // Clear message for next load
    postMessage = "";

    // Open connection
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);

    // Actual content is sent in chunks
    server.send(200, "text/html", "");

    // HTML head, styles, title, start body and postMessage
    sendFormatted("<!DOCTYPE html> <html lang='en'> <head> <title>MVP3000 - Device ID %d</title> \
    <script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script> \
    <style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head> \
    <body> <h1>MVP3000 - Device ID %d</h1> <h3 style='color: red;'>%s</h3>", ESPX.getChipId(), ESPX.getChipId(), postMessage);
}

void NetWeb::contentClose() {
    // HTMK close page tags
    server.sendContent("<p>&nbsp;</body></html>");

    // Close connection
    server.sendContent("");
}

void NetWeb::contentHome() {
    // System
    sendFormatted("\
        <h3>System</h3> <ul> \
        <li>ID: %d</li> \
        <li>Build: %s %s</li> \
        <li>Memory: %d, fragmentation %d%%</li> \
        <li>Uptime: %s</li> \
        <li>Last restart reason: %s</li> \
        <li>CPU frequency: %d MHz</li> \
        <li>Main loop duration: %d / %d / %d ms (mean/min/max)</li> </ul>",
        ESPX.getChipId(), __DATE__,__TIME__, ESP.getFreeHeap(), ESPX.getHeapFragmentation(), mvp.helper.upTime(), ESPX.getResetReason().c_str(), ESP.getCpuFreqMHz(), mvp.loopDurationMean_ms, mvp.loopDurationMin_ms, mvp.loopDurationMax_ms);

    // Network
    sendFormatted("\
        <h3>Network</h3> <ul> \
        <li>Fallback AP SSID: '%s'</li> \
        <li>Network credentials: leave SSID empty to remove, any changes are applied at once.<br> <form action='/save' method='post'> SSID <input name='newSsid' value='%s'> Passphrase <input type='password' name='newPass' value='%s'> <input type='submit' value='Save'> </form> </li> \
        <li>Reconnect tries: <br> <form action='/save' method='post'> <input name='clientConnectRetries' type='number' value='%d' min='1' max='255'> <input type='submit' value='Save'> </form> </li>",
        mvp.net.apSsid.c_str(), mvp.net.cfgNet.clientSsid.c_str(), mvp.net.cfgNet.clientPass.c_str(), mvp.net.cfgNet.clientConnectRetries);
    sendFormatted("\
        <li>Force client mode. WARNING: If credentials are wrong the device will be inaccessible via network, thus require re-flashing! \
         <form action='/savecheck' method='post' onsubmit='return promptId(this);'> <input name='forceClientMode' type='checkbox' %s value='1'> <input name='forceClientMode' type='hidden' value='0'> <input name='deviceId' type='hidden'> <input type='submit' value='Save'> </form> </li> </ul>",
        (mvp.net.cfgNet.forceClientMode == true) ? "checked" : "" );

    // MQTT communication
    sendFormatted("\
        <h3>MQTT Communication</h3> <ul> \
        <li>Status: %s </li> \
        <li>Auto-discovery port local broker: 1024-65535, default is 4211.<br> <form action='/save' method='post'> <input name='discoveryPort' value='%d' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li> \
        <li>Forced external broker:<br> <form action='/save' method='post'> <input name='mqttForcedBroker' value='%s'> <input type='submit' value='Save'> </form> </li> \
        <li>MQTT port: default is 1883 (unsecure) <br> <form action='/save' method='post'> <input name='mqttPort' value='%d' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li> \
        <li>Topic: <br> <form action='/save' method='post'> %s <input name='mqttTopicSuffix' value='%s' minlength='5'> <input type='submit' value='Save'> </form> </li> </ul>",
        mvp.net.netCom.controllerConnectedString().c_str(), mvp.net.netCom.cfgNetCom.discoveryPort, mvp.net.netCom.cfgNetCom.mqttForcedBroker.c_str(), mvp.net.netCom.cfgNetCom.mqttPort, mvp.net.netCom.mqttTopicPrefix.c_str(), mvp.net.netCom.cfgNetCom.mqttTopicSuffix);


    // Modules list
    sendFormatted("<h3>Modules</h3> <ul>");
    for (uint8_t i = 0; i < mvp.moduleCount; i++) {
        if (mvp.xmodules[i]->enableContentModuleNetWeb)
            sendFormatted("<li><a href='?m=%d'>%s</a></li>", i, mvp.xmodules[i]->description.c_str());
        else
            sendFormatted("<li>%s</li>", mvp.xmodules[i]->description.c_str());
    }
    sendFormatted("</ul>");

    // Maintenance
    sendFormatted("\
        <h3>Maintenance</h3> <ul> \
        <li> <form action='/start' method='post' onsubmit='return confirm(`Restart?`);'> <input name='restart' type='hidden'> <input type='submit' value='Restart' > </form> </li> \
        <li> <form action='/startcheck' method='post' onsubmit='return promptId(this);'> <input name='resetdevice' type='hidden'> <input name='deviceId' type='hidden'> <input type='submit' value='Factory reset'> </form> </li> </ul>");
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::editCfg(boolean checkId) {
    if (server.args() == 0) { // Likely a reload-from-locationbar error
        responseRedirect("Redirected ...");
        return;
    }

    if (checkId)
        if (!formInputCheckId())
            return;

    auto lambdaKey = [&](int i) { return server.argName(i); };
    auto lambdaValue = [&](int i) { return server.arg(i); };

    // MVP3000 settings
    bool success = mvp.net.editCfgNetWeb(server.args(), lambdaKey, lambdaValue);
    if (!success)
        success = mvp.net.netCom.editCfgNetWeb(server.args(), lambdaKey, lambdaValue);

    // Loop through modules while success is not true
    if (!success)
        for (uint8_t i = 0; i < mvp.moduleCount; i++) {
            // Use lambdas to read args
            success = mvp.xmodules[i]->editCfgNetWeb(server.args(), lambdaKey, lambdaValue);
            if (success)
                break;
        }

    // Response for display
    if (success) {
        responseRedirect("Settings saved!");
    } else {
        responseRedirect("Input error!");
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid form input from: %s",  server.client().remoteIP().toString().c_str());
    }
}

void NetWeb::startAction(boolean checkId) {
    if (server.args() == 0) { // Likely a reload-from-locationbar error
        responseRedirect("Redirected ...");
        return;
    }

    if (checkId)
        if (!formInputCheckId())
            return;

    // MVP3000 actions
    bool success = true;
    switch (mvp.helper.hashStringDjb2(server.argName(0).c_str())) {

        case mvp.helper.hashStringDjb2("restart"):
            responsePrepareRestart();
            ESP.restart();
            break;

        case mvp.helper.hashStringDjb2("resetdevice"): //
            responsePrepareRestart();
            mvp.config.factoryResetDevice(); // calls ESP.restart();
            break;

        default: // Keyword not found
            success = false;
    }

    // Loop through modules while success is not true
    if (!success) {
        auto lambdaKey = [&](int i) { return server.argName(i); };
        auto lambdaValue = [&](int i) { return server.arg(i); };
        for (uint8_t i = 0; i < mvp.moduleCount; i++) {
            // Use lambdas to read args
            success = mvp.xmodules[i]->startActionNetWeb(server.args(), lambdaKey, lambdaValue);
            if (success)
                break;
        }
    }

    // Response for display
    if (!success) {
        responseRedirect("Input error!");
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid form input from: %s",  server.client().remoteIP().toString().c_str());
    } // else in case of success response should be generated from within the action function
}


bool NetWeb::formInputCheckId() {
    // Minimum two arguments: action and deviceId
    if (server.args() >= 2) {
        // deviceId is last input element
        uint8_t lastInput = server.args() - 1;
        if ( (server.argName(lastInput) == "deviceId") && (mvp.helper.isValidInteger(server.arg(lastInput))) && (server.arg(lastInput).toInt() == ESPX.getChipId()) )
            return true;
    }

    responseRedirect("Id check failed.");
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Invalid deviceId input from: %s",  server.client().remoteIP().toString().c_str());
    return false;
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::sendFormatted(const char* messageFormatString, ...) {
    char message[WEB_CHUNK_LENGTH];
    va_list args;
    va_start(args, messageFormatString);
    vsnprintf(message, sizeof(message), messageFormatString, args);
    va_end(args);

    server.sendContent(message);
}

void NetWeb::responseRedirect(const char* message) {
    // Message to serve on next page load
    postMessage = message;
    // Redirect to avoid post reload, 303 temporary
    server.sendHeader("Location", ((server.args() == 1) && (server.argName(0) == "m")) ? "/?m=" + server.arg(0) : "/");
    server.send(303);
}

void NetWeb::responsePrepareRestart() {
    // http-equiv seems to not show up in history
    server.send(200, "text/html", "<!DOCTYPE html> <head> <meta http-equiv='refresh' content='4;url=/'> </head> <body> <h3 style='color: red;'>Restarting ...</h3> </body> </html>");
    // Wait for redirect to be actually sent
    delay(25);
}
