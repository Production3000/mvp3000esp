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


// https://arduino.stackexchange.com/questions/56517/formatting-strings-in-arduino-for-output
// https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html


// TODO simplify!!! https://randomnerdtutorials.com/esp32-web-server-spiffs-spi-flash-file-system/


void NetWeb::setup() {
    // Folders/requests
    server.on("/", std::bind(&NetWeb::serveRequestMain, this));
    server.on("/save", std::bind(&NetWeb::requestEditConfigValue, this));
    server.onNotFound(std::bind(&NetWeb::serveRequestMain, this));

    server.on("/measureOffset", std::bind(&NetWeb::measureOffset, this));                                                  
    server.on("/measureScaling", std::bind(&NetWeb::measureScaling, this));
    server.on("/resetOffset", std::bind(&NetWeb::resetOffset, this));
    server.on("/resetScaling", std::bind(&NetWeb::resetScaling, this));

    // Start server, independent of wifi status, main will only be called when connected
    server.begin();
}


void NetWeb::loop() {
    // Called from net.loop() only if network is up
    
    // Handle page requests
    server.handleClient();
}



// void NetWeb::registerPageElement(String moduleName, TPageElementFunction func) {

//     server.on("/measureOffset", std::bind(&NetWeb::measureOffset, this));    

//     if (pageElementCount < MAX_PAGE_ELEMENTS) {
//         pageElements[pageElementCount++] = func;
//     } else {
//         mvp.logger.write(CfgLogger::Level::ERROR, "Web page element array is full.");
//     }
// }


// void NetWeb::serveRequestPageElements() {
//     // Send data in chunks, but use pointer here so other functions do not need to know length
//     char* message = new char[WEB_CHUNK_LENGTH];

//     // Loop through all pageElements
//     for (uint8_t i = 0; i < pageElementCount; i++) {
//         // Loop through all strings of each pageElement
//         for (uint8_t j = 0; j < MAX_PE_STRING_COUNT; j++) {
//             pageElements[i](j, message, WEB_CHUNK_LENGTH);
//             // Break when returned string is empty
//             if (strlen(message) == 0)
//                 break;
//             server.sendContent(message);
//         }
//     }
// }



void NetWeb::serveRequestMain() {
    // Prepare and head
    sendStart();
    // Serve page content
    serveRequestMainHead();
    // serveRequestPageElements();
    // Close page
    sendClose();

    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Serving root to: %s",  server.client().remoteIP().toString().c_str());
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::sendStart() {
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

void NetWeb::sendClose() {
    // HTMK close page tags
    server.sendContent("</body></html>");
    // Close connection
    server.sendContent("");
    server.client().stop();
}

void NetWeb::sendFormatted(const char* messageFormatString, ...) {
    char message[WEB_CHUNK_LENGTH];
    va_list args;
    va_start(args, messageFormatString);
    vsnprintf(message, sizeof(message), messageFormatString, args);
    va_end(args);

    server.sendContent(message);
}


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::serveRequestMainHead() {
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
        <li>Reconnect tries: <br> <form action='/save' method='post'> <input name='clientConnectRetries' value='%d' min='1' max='255'> <input type='submit' value='Save'> </form> </li>",
        mvp.net.apSsid.c_str(), mvp.net.cfgNet.clientSsid.c_str(), mvp.net.cfgNet.clientPass.c_str(), mvp.net.cfgNet.clientConnectRetries);
    sendFormatted("\
        <li>Force client mode. WARNING: If credentials are wrong the device will be inaccessable via network, thus require re-flashing! \
         <form action='/save' method='post' onsubmit='return promptId(this);'> <input name='forceClientMode' type='checkbox' %s value='1'> <input name='forceClientMode' type='hidden' value='0'> <input name='deviceId' type='hidden'> <input type='submit' value='Save'> </form> </li> </ul>",
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
    mvp.sensorHandler.printWeb();


    // Maintenance
    sendFormatted("\
        <h3>Maintenance</h3> <ul> \
        <li> <form action='/save' method='post' onsubmit='return confirm(`Restart?`);'> <input name='restart' type='hidden'> <input type='submit' value='Restart' > </form> </li> \
        <li> <form action='/save' method='post' onsubmit='return promptId(this);'> <input name='resetdevice' type='hidden'> <input name='deviceId' type='hidden'> <input type='submit' value='Factory reset'> </form> </li> </ul>");

}




///////////////////////////////////////////////////////////////////////////////////



bool NetWeb::requestConfirmSensorId() {
    // Minimum two arguments: action and deviceId
    if (server.args() < 2)
        return false;

    // deviceId is last input element
    uint8_t lastInput = server.args() - 1;
    if ( (server.argName(lastInput) != "deviceId") || (!mvp.helper.isValidInteger(server.arg(lastInput))) || (server.arg(lastInput).toInt() != ESPX.getChipId()) )
        return false;

    return true;
}

void NetWeb::requestEditConfigValue() {
    if (server.args() < 1) {
        responseRedirect("Input error!");
        return;
    }

    bool success = false;
    switch (mvp.helper.hashStringDjb2(server.argName(0).c_str())) {

        // Network
        case mvp.helper.hashStringDjb2("forceClientMode"):
            if (!requestConfirmSensorId())
                break;
        case mvp.helper.hashStringDjb2("clientConnectRetries"):
        case mvp.helper.hashStringDjb2("newSsid"):
            success = mvp.net.editCfg(server.argName(0), server.arg(0), (server.args() == 2) ? server.arg(1) : ""); // pass second form input or empty string
            break;

        // MQTT Communication
        case mvp.helper.hashStringDjb2("discoveryPort"):
        case mvp.helper.hashStringDjb2("mqttForcedBroker"):
        case mvp.helper.hashStringDjb2("mqttPort"): 
        case mvp.helper.hashStringDjb2("mqttTopicSuffix"): 
            success = mvp.net.netCom.editCfg(server.argName(0), server.arg(0));
            break;

        // Maintenance
        case mvp.helper.hashStringDjb2("restart"): 
            responsePrepareRestart();
            ESP.restart();
            break;

        case mvp.helper.hashStringDjb2("resetdevice"):
            if (!requestConfirmSensorId())
                break;
            responsePrepareRestart();
            mvp.config.factoryResetDevice(); // calls ESP.restart();
            break;
    }

    // User info
    responseRedirect((success) ? "Settings saved!" : "Input error!");
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





void NetWeb::measureOffset() {
    mvp.sensorHandler.measureOffset();
    responseRedirect("Measuring offset, this may take a few seconds ...");
}

void NetWeb::measureScaling() { 
    // Generally check web input
    if ( (server.args() == 2) &&  (server.argName(0) == "valueNumber") && (server.argName(1) == "targetValue") ) {
        // Function checks bounds of valueNumber
        if (mvp.sensorHandler.measureScaling(server.arg(0).toInt(), server.arg(1).toInt())) {
            responseRedirect("Measuring scaling, this may take a few seconds ...");
            return;
        }
    }
    responseRedirect("Input error!");
}
void NetWeb::resetOffset() {
    mvp.sensorHandler.resetOffset();
    responseRedirect("Offset reset.");
}
void NetWeb::resetScaling() {
    mvp.sensorHandler.resetScaling();
    responseRedirect("Scaling reset.");
}