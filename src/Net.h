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

#ifndef MVP3000_NET
#define MVP3000_NET

#include <Arduino.h>
#ifdef ESP8266
    #include <ESP8266WiFi.h>
#else
    #include <WiFi.h>
#endif
#include <DNSServer.h> // for captive portal

#include "Config.h"
#include "NetCom.h"
#include "NetMqtt.h"
#include "NetTime.h"
#include "NetWeb.h"


struct CfgNet : public CfgJsonInterface  {

    // Modifiable settings saved to SPIFF

    String clientSsid = ""; // IEEE 1-32 chars
    String clientPass = ""; // IEEE 8-63 chars
    uint8_t clientConnectRetries = 3;

    // DANGER, forceClientMode could make the device inaccessible for the user without flashing it
    // Usefull when network settings are correct for sure but likely offline during power on
    //  false on power on: try network settings clientConnectRetries times
    //      on success set clientConnectSuccess to true
    //      on fail fallback to AP
    //  false with clientConnectSuccess true on connection lost: try endlessly to previously verified network settings
    //  true on power on: try network settings endlessly
    boolean forceClientMode = false;

    CfgNet() : CfgJsonInterface("cfgNet") {
        addSetting<uint8_t>("clientConnectRetries", &clientConnectRetries, [&](const String& s) { uint8_t n = s.toInt(); if (n > 100) return false; clientConnectRetries = n; return true; } ); // Limit to 100, any more is 'forever'
        addSetting<String>("clientSsid", &clientSsid, [&](const String& s) { clientSsid = s; return true; } ); // Check is in extra function
        addSetting<String>("clientPass", &clientPass, [&](const String& s) { clientPass = s; return true; } ); // Check is in extra function
        addSetting<boolean>("forceClientMode", &forceClientMode, [&](const String& s) { forceClientMode = s.toInt(); return true; } );
    }
};


class Net {

    public:
        enum class NET_STATE_TYPE: uint8_t {
            CLIENT = 0,
            CONNECTING = 1,
            AP = 2,
            DISABLEDX = 3,
            ERROR = 4
        };
        NET_STATE_TYPE netState = NET_STATE_TYPE::DISABLEDX;

        CfgNet cfgNet;

        NetCom netCom;
        NetMqtt netMqtt;
        NetTime netTime;
        NetWeb netWeb;

        String apSsid = "device" + String(_helper.ESPX->getChipId());

        IPAddress myIp = INADDR_NONE;

        void setup();
        void loop();

        bool editClientConnection(const String& newSsid, const String& newPass);
        void cleanCfgKeepClientInfo();

        boolean connectedAsClient() { return (netState == NET_STATE_TYPE::CLIENT); }
        boolean connectedAsClientOrAP() { return ((netState == NET_STATE_TYPE::CLIENT) || (netState == NET_STATE_TYPE::AP)); }

    private:
        DNSServer dnsServer;

        // Counter for client connect fails
        uint8_t clientConnectFails = 0;
        boolean clientConnectSuccess = false;

        void connectClient();
        void WiFiGotIP();
        void WiFiStationDisconnected();

        void startWifi();
        void startAp();
        void startClient();

        uint32_t delayedRestartWifi_ms = 0; // TODO replace with LimitTimer
        void delayedRestartWifi(uint32_t delay_ms = 50) { delayedRestartWifi_ms = millis() + delay_ms; };

// ESP8266 needs definition of Wifi events
#ifdef ESP8266
        WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
#endif

    public:

        String templateProcessor(uint16_t var);

};

#endif