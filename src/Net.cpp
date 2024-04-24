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

#include "Net.h"

#include "MVP3000.h"
extern MVP3000 mvp;


void Net::setup(CfgNet _cfgNet) {
    cfgNet = _cfgNet;

    // Load save credentials
    if (mvp.config.cfgReadPrepare("cfgNet")) {
        // Make sure saved ssid and pass are both readable before overwriting defaults
        String tmp;
        if (mvp.config.cfgReadGetValue("clientSsid", tmp) && mvp.config.cfgReadGetValue("clientPass", tmp)) {
            mvp.config.cfgReadGetValue("clientSsid", cfgNet.clientSsid);
            mvp.config.cfgReadGetValue("clientPass", cfgNet.clientPass);
            mvp.config.cfgReadGetValue("clientConnectRetries", cfgNet.clientConnectRetries);
            mvp.config.cfgReadGetValue("forceClientMode", cfgNet.forceClientMode);
            mvp.logger.write(CfgLogger::Level::INFO, "NetCfg loaded.");
        }
    }
    mvp.config.cfgReadClose();

    // Start wifi
    startWifi();

    // Init modules
    netWeb.setup();
    netCom.setup(cfgNet.cfgNetCom);
}
 
void Net::loop() {
    // Nothing to do without network
    if ((status != Status::CLIENT) && (status != Status::AP))
        return;
    
    // Captive portal only for AP
    if (status == Status::AP)
        dnsServer.processNextRequest();

    // Webpage, communication
    netWeb.loop();
    if (status == Status::CLIENT) 
        netCom.loop();
}

bool Net::editClientConnection(String newSsid, String newPass) {

    bool success = false;
    success = cfgNet.setWifiCredentials(newSsid, newPass);

    if (success) {
        clientConnectFails = 0;
        clientConnectSuccess = false;
        mvp.logger.write(CfgLogger::Level::INFO, "SSID and pass updated.");
        startWifi();
    }

    return success;
}

bool Net::editCfg(String varName, String newValueA, String newValueB) {

    bool success = false;
    switch (mvp.helper.hashStringDjb2(varName.c_str())) {
        case mvp.helper.hashStringDjb2("newSsid"):
            success = editClientConnection(newValueA, newValueB);
            break;
        case mvp.helper.hashStringDjb2("clientConnectRetries"):
            success = cfgNet.setClientConnectRetries(newValueA.toInt());
            break;
        case mvp.helper.hashStringDjb2("forceClientMode"):
            success = cfgNet.setForceClientMode((newValueA.toInt() == 0) ? false : true);
            break;
    }

    if (success) {
        // save cfg
        mvp.config.cfgWritePrepare();
        mvp.config.cfgWriteAddValue("clientSsid", cfgNet.clientSsid);
        mvp.config.cfgWriteAddValue("clientPass", cfgNet.clientPass);
        mvp.config.cfgWriteAddValue("clientConnectRetries", cfgNet.clientConnectRetries);
        mvp.config.cfgWriteAddValue("forceClientMode", cfgNet.forceClientMode);
        mvp.config.cfgWriteClose("cfgNet");
        mvp.logger.write(CfgLogger::Level::INFO, "cfgNet updated.");
    }
    return success;
}



///////////////////////////////////////////////////////////////////////////////////

void Net::startWifi() {
    status = Status::CONNECTING;
    // Start as client or force AP if ssid/pass not IEEE conform
    if ((cfgNet.clientSsid.length() > 0) && (cfgNet.clientPass.length() >= 8))
        startClient();
    else
        startAp();
}

void Net::startAp() {
    // Start AP with no password
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(apSsid)) {
        status = Status::NONE;
        mvp.logger.write(CfgLogger::Level::ERROR, "Error starting AP.");
        return;
    }

    // Start captive portal
    if (!dnsServer.start(53, "*", WiFi.softAPIP())) {
        status = Status::NONE;
        mvp.logger.write(CfgLogger::Level::ERROR, "Error starting captive portal.");
        return;
    }

    // Set system status
    status = Status::AP;
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "AP started, %s, %s", apSsid.c_str(), WiFi.softAPIP().toString().c_str());
}


void Net::startClient() {
    WiFi.mode(WIFI_STA);

// ESP32/ESP8266 have different  Wifi events                                                                                                              // TODO !!! test
#ifdef ESP8266
    WiFi.onStationModeDisconnected(std::bind(&Net::WiFiStationDisconnected, this)); // disconnectedEventHandler = 
    WiFi.onStationModeGotIP(std::bind(&Net::WiFiGotIP, this));  // gotIpEventHandler = 
#else
    WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info) { WiFiStationDisconnected(); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info) { WiFiGotIP(); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
#endif

    connectClient();
}

void Net::connectClient() {
    WiFi.begin(cfgNet.clientSsid, cfgNet.clientPass);
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Connecting to (SSID/pass): %s %s", cfgNet.clientSsid.c_str(), cfgNet.clientPass.c_str());
}

void Net::WiFiGotIP() {
    status = Status::CLIENT;
    clientConnectFails = 0;
    // Reconnect endlessly to a previously sucessfully connected network (until reboot)
    clientConnectSuccess = true;
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Connection established: %s", WiFi.localIP().toString().c_str() );
}

void Net::WiFiStationDisconnected() {
    status = Status::CONNECTING;
    if (clientConnectSuccess || cfgNet.forceClientMode) {
        mvp.logger.write(CfgLogger::Level::WARNING, "Network disconnected.");
        connectClient();
    } else if (++clientConnectFails < cfgNet.clientConnectRetries) {
        connectClient();
    }
    else {
        mvp.logger.write(CfgLogger::Level::INFO, "Client connect limit reached.");
        startAp();
    }
}
