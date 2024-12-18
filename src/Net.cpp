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


void Net::setup() {
    // Read config
    mvp.config.readCfg(cfgNet);

    // Start wifi
    startWifi();

    // Init web interface, UDP discovery and MQTT (in that order)
    netWeb.setup();
    netCom.setup();
    netMqtt.setup();
    netTime.setup();

    // Register config
    netWeb.registerCfg(&cfgNet);

    // Register actions
    netWeb.registerAction("setwifi", [&](int args, WebArgKeyValue argKey, WebArgKeyValue argValue) {
        // argValue(0) is the action name
        if (args != 3)
            return false;
        if ((argKey(1) != "newSsid") && (argKey(2) != "newPass"))
            return false;
        return editClientConnection(argValue(1), argValue(2));
    }, "Connecting to network ...");

}

void Net::loop() {
    // Check if delayed restart was set
    if (delayedRestartWifi_ms > 0) {
        if (millis() > delayedRestartWifi_ms) {
            delayedRestartWifi_ms = 0; // Clear flag
            startWifi();
        }
    }

    // Unwanted connection states need to be filltered in the respective loop() functions
    netMqtt.loop();
    netCom.loop();
    netWeb.loop();
    netTime.loop();

    // Captive portal only for AP
    if (netState == NET_STATE_TYPE::AP) {
        dnsServer.processNextRequest();
    }
}


///////////////////////////////////////////////////////////////////////////////////

bool Net::editClientConnection(const String& newSsid, const String& newPass) {
    // SSID and pass are both either empty or IEEE conform
    if (((newSsid.length() == 0) && (newPass.length() == 0)) || ((newSsid.length() >= 1) && (newSsid.length() <= 32) && (newPass.length() >= 8) && (newPass.length() <= 63))) {
        cfgNet.clientSsid = newSsid;
        cfgNet.clientPass = newPass;
        // Save cfg
        mvp.config.writeCfg(cfgNet);
        // Reset connection state
        clientConnectFails = 0;
        clientConnectSuccess = false;
        // Restart wifi with new settings but leave time for web response to be sent
        delayedRestartWifi();
        mvp.logger.write(CfgLogger::Level::INFO, "SSID and pass updated.");
        return true;
    }
    return false;
}

void Net::cleanCfgKeepClientInfo() {
    // For some reason this does not work:
    //  CfgNet cfgNetClean = *new CfgNet();
    //  cfgNetClean.clientSsid = cfgNet.clientSsid;
    //  cfgNetClean.clientPass = cfgNet.clientPass;
    //  mvp.config.writeCfg(cfgNetClean);
    // Alternative but not nice:
    cfgNet.clientConnectRetries = 3;
    cfgNet.forceClientMode = false;
    mvp.config.writeCfg(cfgNet);
}

///////////////////////////////////////////////////////////////////////////////////

void Net::startWifi() {
    netState = NET_STATE_TYPE::CONNECTING;
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
        netState = NET_STATE_TYPE::ERROR;
        mvp.logger.write(CfgLogger::Level::ERROR, "Error starting AP.");
        return;
    }

    // Start captive portal
    if (!dnsServer.start(53, "*", WiFi.softAPIP())) {
        netState = NET_STATE_TYPE::ERROR;
        mvp.logger.write(CfgLogger::Level::ERROR, "Error starting captive portal.");
        return;
    }

    netState = NET_STATE_TYPE::AP;
    myIp = WiFi.softAPIP();
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "AP started: %s, %s", apSsid.c_str(), WiFi.softAPIP().toString().c_str());
}


void Net::startClient() {
    WiFi.mode(WIFI_STA);

// ESP32/ESP8266 have different Wifi events
#ifdef ESP8266
    disconnectedEventHandler = WiFi.onStationModeDisconnected(std::bind(&Net::WiFiStationDisconnected, this));
    gotIpEventHandler = WiFi.onStationModeGotIP(std::bind(&Net::WiFiGotIP, this));
#else
    WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info) { WiFiStationDisconnected(); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info) { WiFiGotIP(); }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
#endif

    connectClient();
}

void Net::connectClient() {
    WiFi.begin(cfgNet.clientSsid, cfgNet.clientPass);
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Connecting to (SSID/pass): %s/%s", cfgNet.clientSsid.c_str(), cfgNet.clientPass.c_str());
}

void Net::WiFiGotIP() {
    // Filter double-call, maybe because of auto-reconnect within the ESP
    if (netState == NET_STATE_TYPE::CLIENT)
        return;
    netState = NET_STATE_TYPE::CLIENT;

    myIp = WiFi.localIP();

    // Reconnect endlessly to a previously successfully connected network (until reboot)
    clientConnectSuccess = true;
    clientConnectFails = 0;

    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Connection established: %s", WiFi.localIP().toString().c_str() );
}

void Net::WiFiStationDisconnected() {
    myIp = INADDR_NONE;
    netState = NET_STATE_TYPE::CONNECTING;
    if (clientConnectSuccess || cfgNet.forceClientMode) {
        mvp.logger.write(CfgLogger::Level::INFO, "Network disconnected.");
        connectClient();
    } else if (++clientConnectFails < cfgNet.clientConnectRetries) {
        connectClient();
    } else {
        mvp.logger.write(CfgLogger::Level::INFO, "Client connect limit reached.");
        startAp();
    }
}

String Net::templateProcessor(uint16_t var) {
    switch (var) {
        case 41:
            return apSsid.c_str();
        case 42:
            return cfgNet.clientSsid.c_str();
        case 43:
            return cfgNet.clientPass.c_str();
        case 44:
            return String(cfgNet.clientConnectRetries);
        case 45:
            return (cfgNet.forceClientMode == true) ? "checked" : "";
        default:
            return "";
    }
 }
