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

    // Webpage, controller communication, data transfer
    netWeb.loop();
    netCom.loop();
}

void Net::editClientConnection(String newSsid, String newPass) {
    // Any input not conforming IEEE is ignored later on
    clientConnectFails = 0;
    cfgNet.clientConnectForever = false;

    cfgNet.clientSsid = newSsid;
    cfgNet.clientPass = newPass;

    mvp.config.cfgWritePrepare();
    mvp.config.cfgWriteAddValue("clientSsid", newSsid);
    mvp.config.cfgWriteAddValue("clientPass", newPass);
    mvp.config.cfgWriteClose("cfgNet");

    mvp.logger.write(CfgLogger::Level::INFO, "SSID and pass updated.");

    startWifi();
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
    if (!WiFi.softAP(cfgNet.apSsid)) {
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
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "AP started, %s, %s", cfgNet.apSsid.c_str(), WiFi.softAPIP().toString().c_str());
}


void Net::startClient() {
    WiFi.mode(WIFI_STA);

    // Wifi events                                                                                                              // TODO !!! ESP32/8266
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
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Connecting to (SSID/pass): %s %s", cfgNet.clientSsid, cfgNet.clientPass);
}

void Net::WiFiGotIP() {
    status = Status::CLIENT;
    clientConnectFails = 0;
    // Reconnect endlessly to a previously sucessfully connected network (until reboot)
    cfgNet.clientConnectForever = true;
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Connection established: %s", const_cast<char*>(WiFi.localIP().toString().c_str() ));
}

void Net::WiFiStationDisconnected() {
    status = Status::CONNECTING;
    if (cfgNet.clientConnectForever) {
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
