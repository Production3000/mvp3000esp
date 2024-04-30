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
    server.on("/save", std::bind(&NetWeb::serveForm, this));
    server.on("/savecheck", std::bind(&NetWeb::serveFormCheckId, this));
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
        mvp.xmodules[server.arg(0).toInt()]->netWebContentModule();
    } else {
        contentHome();
    }

    // Close page
    contentClose();

    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Serving page to: %s",  server.client().remoteIP().toString().c_str());
}

void NetWeb::serveForm() {
    if (server.args() < 1) {
        responseRedirect("Input error!");
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid form input from: %s",  server.client().remoteIP().toString().c_str());
        return;
    }

    auto lambdaKey = [&](int i) { return server.argName(i); };
    auto lambdaValue = [&](int i) { return server.arg(i); };

    // MVP3000 settings
    bool success = mvp.net.editCfgNetWeb(server.args(), lambdaKey, lambdaValue);
    if (!success)
        success = mvp.net.netCom.editCfgNetWeb(server.args(), lambdaKey, lambdaValue);

    if (!success)
        switch (mvp.helper.hashStringDjb2(server.argName(0).c_str())) {                                          // TODO separate actions

            // Maintenance actions
            case mvp.helper.hashStringDjb2("restart"): 
                responsePrepareRestart();
                ESP.restart();
                break;

            case mvp.helper.hashStringDjb2("resetdevice"):
                responsePrepareRestart();
                mvp.config.factoryResetDevice(); // calls ESP.restart();
                break;
        }

    if (success) {
        responseRedirect("Settings saved!");
        return;
    }

    // Loop through modules while success is not true
    for (uint8_t i = 0; i < mvp.moduleCount; i++) {       
        // Use lambdas to read args
        success = mvp.xmodules[i]->editCfgNetWeb(server.args(), lambdaKey, lambdaValue);
        if (success)
            return;
    }

    // All failed
    responseRedirect("Input error!");
    mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid form input from: %s",  server.client().remoteIP().toString().c_str());
}



void NetWeb::serveFormCheckId() {
    // Minimum two arguments: action and deviceId
    if (server.args() < 2) {
        responseRedirect("Id check failed.");
        return;
    }
    // deviceId is last input element
    uint8_t lastInput = server.args() - 1;
    if ( (server.argName(lastInput) != "deviceId") || (!mvp.helper.isValidInteger(server.arg(lastInput))) || (server.arg(lastInput).toInt() != ESPX.getChipId()) ) {
        responseRedirect("Id check failed.");
        return;
    }

    serveForm();
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::contentStart() {
    // Open connection
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);

    // Actual content is sent in chunks
    server.send(200, "text/html", "");

    // HTML head, styles, title, start body and postMessage                      
    sendFormatted("<!DOCTYPE html> <html lang='en'> <head> <title>Production3000 - ID %d</title> \
    <script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script> \
    <style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head> \
    <body> <h1>Production3000 - Sensor %d</h1> <h3 style='color: red;'>%s</h3>", ESPX.getChipId(), ESPX.getChipId(), postMessage);
    
    // Clear message for next load
    postMessage = "";
}

void NetWeb::contentClose() {
    // HTMK close page tags
    server.sendContent("</body></html>");
    // Close connection
    server.sendContent("");
    server.client().stop();
}

void NetWeb::contentHome() {
    // System                                                                                   // TODO display last warnings/errors
    sendFormatted("\
        <h3>System</h3> <ul> \
        <li>ID: %d</li> \
        <li>Build: %s %s</li> \
        <li>Memory: %d/%d free, fragmentation %d%%</li> \
        <li>Uptime: %s</li> \
        <li>Last restart reason: %s</li> \
        <li>Main loop duration: %d ms</li> </ul>",
        ESPX.getChipId(), __DATE__,__TIME__, ESP.getFreeHeap(), ESP.getHeapSize(), ESPX.getHeapFragmentation(), mvp.helper.upTime(), ESPX.getResetReason().c_str(), mvp.loopDuration_ms);   

    // Network
    sendFormatted("\
        <h3>Network</h3> <ul> \
        <li>Fallback AP SSID: '%s'</li> \
        <li>Network credentials: leave SSID empty to remove, any changes are applied at once.<br> <form action='/save' method='post'> SSID <input name='newSsid' value='%s'> Passphrase <input type='password' name='newPass' value='%s'> <input type='submit' value='Save'> </form> </li> \
        <li>Reconnect tries: <br> <form action='/save' method='post'> <input name='clientConnectRetries' type='number' value='%d' min='1' max='255'> <input type='submit' value='Save'> </form> </li>",
        mvp.net.apSsid.c_str(), mvp.net.cfgNet.clientSsid.c_str(), mvp.net.cfgNet.clientPass.c_str(), mvp.net.cfgNet.clientConnectRetries);
    sendFormatted("\
        <li>Force client mode. WARNING: If credentials are wrong the device will be inaccessable via network, thus require re-flashing! \
         <form action='/savecheck' method='post' onsubmit='return promptId(this);'> <input name='forceClientMode' type='checkbox' %s value='1'> <input name='forceClientMode' type='hidden' value='0'> <input name='deviceId' type='hidden'> <input type='submit' value='Save'> </form> </li> </ul>",
        (mvp.net.cfgNet.forceClientMode == true) ? "checked" : "" );

    // MQTT communication
    sendFormatted("\
        <h3>MQTT Communication</h3> <ul> \
        <li>Broker: %s </li> \
        <li>Auto-discovery port local broker: 1024-65535, default is 4211.<br> <form action='/save' method='post'> <input name='discoveryPort' value='%d' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li> \
        <li>Forced external broker:<br> <form action='/save' method='post'> <input name='mqttForcedBroker' value='%s'> <input type='submit' value='Save'> </form> </li> \
        <li>MQTT port: default is 1883 (unsecure) <br> <form action='/save' method='post'> <input name='mqttPort' value='%d' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li> \
        <li>Topic: <br> <form action='/save' method='post'> %s <input name='mqttTopicSuffix' value='%s' minlength='5'> <input type='submit' value='Save'> </form> </li> </ul>",
        mvp.net.netCom.controllerConnectedString(), mvp.net.netCom.cfgNetCom.discoveryPort, mvp.net.netCom.cfgNetCom.mqttForcedBroker, mvp.net.netCom.cfgNetCom.mqttPort, mvp.net.netCom.mqttTopicPrefix.c_str(), mvp.net.netCom.cfgNetCom.mqttTopicSuffix);


    // Modules
    sendFormatted("<h3>Modules</h3> <ul>");
    for (uint8_t i = 0; i < mvp.moduleCount; i++) {
        sendFormatted("<li><a href='?m=%d'>%s</a></li>", i, mvp.xmodules[i]->description.c_str());
    }
    sendFormatted("</ul>");

    // Maintenance
    sendFormatted("\
        <h3>Maintenance</h3> <ul> \
        <li> <form action='/save' method='post' onsubmit='return confirm(`Restart?`);'> <input name='restart' type='hidden'> <input type='submit' value='Restart' > </form> </li> \
        <li> <form action='/savecheck' method='post' onsubmit='return promptId(this);'> <input name='resetdevice' type='hidden'> <input name='deviceId' type='hidden'> <input type='submit' value='Factory reset'> </form> </li> </ul>");

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
    server.sendHeader("Location", "/");
    server.send(303);
}

void NetWeb::responsePrepareRestart() {
    // http-equiv seems to not show up in history
    server.send(200, "text/html", "<!DOCTYPE html> <head> <meta http-equiv='refresh' content='4;url=/'> </head> <body> <h3 style='color: red;'>Restarting ...</h3> </body> </html>");
    // Wait for redirect to be actually sent
    delay(25);
}
