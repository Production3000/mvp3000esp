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
    server.on("/resetdevice", std::bind(&NetWeb::requestResetDevice, this));
    server.on("/restart", std::bind(&NetWeb::requestRestart, this));
    // server.on("/save", std::bind(&NetWeb::requestEditConfigValue, this));
    server.on("/savewifi", std::bind(&NetWeb::requestEditWifi, this));
    server.onNotFound(std::bind(&NetWeb::serveRequestMain, this));


    server.on("/measureOffset", std::bind(&NetWeb::measureOffset, this));                                                  
    server.on("/measureScaling", std::bind(&NetWeb::measureScaling, this));
    server.on("/resetOffset", std::bind(&NetWeb::resetOffset, this));
    server.on("/resetScaling", std::bind(&NetWeb::resetScaling, this));

    
    // Start server, independent of wifi status, main will only be called when connected
    server.begin();
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



void NetWeb::loop() {
    // Only called if wifi is connected
    
    // Handle page requests
    server.handleClient();
}

// void NetWeb::registerPageElement(TPageElementFunction func) {
//     if (pageElementCount < MAX_PAGE_ELEMENTS) {
//         pageElements[pageElementCount++] = func;
//     } else {
//         mvp.logger.write(CfgLogger::Level::ERROR, "Web page element array is full.");
//     }
// }


void NetWeb::serveRequestMain() { // root and catch all
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Serving root to: %s",  server.client().remoteIP().toString());

    // Prepare and head
    sendStart();

    // Serve page content
    serveRequestMainHead();
    // serveRequestPageElements();

    sendClose();
}


void NetWeb::sendStart() {
    // Open connection
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);

    // Actual content is sent in chunks
    server.send(200, "text/html", "");

    // HTML head, styles, title, start body and postMessage
    sendFormatted("<!DOCTYPE html> <html lang='en'> <head> <title>Production3000 - ID %d</title> <style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } </style> </head> <body> <h1>Production3000 - Sensor %d</h1> <h3 style='color: red;'>%s</h3>", ESPX.getChipId(), ESPX.getChipId(), postMessage);
    
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



void NetWeb::serveRequestMainHead() {
    // System                                                                                   // TODO display total memory, last warnings/errors
    sendFormatted("\
        <h3>System</h3> <ul> <li>ID: %d</li> <li>Build: %s %s</li> <li>Memory: %d free, %d%% fragmentation</li> <li>Uptime: %s</li> <li>Last restart reason: %s</li> <li>Loop duration: %d ms</li> </ul>",
        ESPX.getChipId(), __DATE__,__TIME__, ESP.getFreeHeap(), ESPX.getHeapFragmentation(), mvp.helper.upTime(), ESPX.getResetReason().c_str(), mvp.loopDuration_ms);

    // Network and Communication
    sendFormatted("\
        <h3>Network</h3> <ul> <li>Fallback AP SSID: '%s'</li> <li>Connect to network: (leave SSID empty to remove, changes are applied at once)<p> <form action='/savewifi' method='post'> SSID <input name='newSsid' value='%s'> Passphrase <input type='password' name='newPass' value='%s'> <input type='submit' value='Save'> </form> </p> </li> </ul>",
        mvp.net.cfgNet.apSsid.c_str(), mvp.net.cfgNet.clientSsid, mvp.net.cfgNet.clientPass);

    // UDP com              // TODO make optional, not pressing, it is just one udp every few seconds
    sendFormatted("\
        <h3>MQTT Communication</h3> <ul> <li>enable/disable</li> <li> Controller: %s) </li> <li> UDP port (1024-65535, default: 4211) <p> <form action='/save' method='post'> <input name='discoveryPort' value='%d'> <input type='submit' value='Save'> </form> </li> </ul>",
        mvp.net.netCom.controllerConnectedString(), mvp.net.cfgNet.cfgNetCom.discoveryPort);


    mvp.sensorHandler.printWeb();


    // Maintenance
    sendFormatted("\
        <h3>Maintenance</h3> <ul> <li> <form action='/restart' method='post' onsubmit='return confirm(`Restart?`);'> <input type='submit' value='Restart' > </form> </li> <li> <form action='/resetdevice' method='post' onsubmit='document.getElementById(`idReset`).value = prompt(`Reset to FACTORY DEFAULTS! Confirm with device ID.`);'> <input id='idReset' name='sensorId' type='hidden' value=''> <input type='submit' value='Factory reset'> </form> </li> </ul>");

}

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


///////////////////////////////////////////////////////////////////////////////////

void NetWeb::requestEditWifi() {
    // Check input
    if ((server.args() != 2) || (server.argName(0) != "newSsid") || (server.argName(1) != "newPass")) {
        responseRedirect("Input error");
        return;
    }

    responseRedirect("Updating client connection ...");                                                     // TODO this is actually not displayed as the page is never served, maybe with AsyncServer?
    delay(20);

    mvp.net.editClientConnection(server.arg(0), server.arg(1));
}

void NetWeb::requestEditConfigValue() {
    // floatToIntExponent
    // if ( (server.args() >= 1) &&  (server.argName(0) == "floatToIntExponent") && mvp.sensorHandler.setSaveFloatToIntExponent(server.args(), [this](uint8_t i) { return server.arg(i); }) ) {
    //     responseRedirect("Settings saved! NOTE: sensor table changed.");
    //     return;
    // }

    // Setting, arg name checked in config
    // if ( (server.args() == 1) && mvp.config.setValue(server.argName(0), server.arg(0)) ) {
    //     responseRedirect("Settings saved!");
    //     return;
    // }

    // Input error
    responseRedirect("Not saved!");
}

void NetWeb::requestResetDevice() {
    if ( (server.argName(0) != "sensorId") || (!mvp.helper.isValidInteger(server.arg(0))) || (server.arg(0).toInt() != ESPX.getChipId()) ) {
        responseRedirect("Device id wrong, not reset!");
        return;
    }
    responsePrepareRestart();
    mvp.config.factoryResetDevice();
}

// void NetWeb::requestResetSensor() {
//     if ( (server.argName(0) != "sensorId") || (!mvp.helper.isValidInteger(server.arg(0))) || (server.arg(0).toInt() != ESPX.getChipId()) ) {
//         responseRedirect("Device id wrong, defaults not loaded!");
//         return;
//     }
//     responsePrepareRestart();
//     mvp.sensorHandler.resetSensor();
// }

void NetWeb::requestRestart() {
    responsePrepareRestart();
    ESP.restart();
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
