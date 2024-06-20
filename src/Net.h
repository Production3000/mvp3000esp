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
#include "NetWeb.h"
#include "ESPX.h"
#ifdef ESP8266
    extern EspClass ESPX;
#else
    extern EspClassX ESPX;
#endif


struct CfgNet : public Cfg  {

    // Modifiable settings saved to SPIFF

    String clientSsid = ""; // IEEE 1-32 chars
    String clientPass = ""; // IEEE 8-63 chars
    uint16_t clientConnectRetries = 3;

    // DANGER, forceClientMode could make the device inaccessible for the user without flashing it
    // Usefull when network settings are correct for sure but likely offline during power on
    //  false on power on: try network settings clientConnectRetries times
    //      on success set clientConnectSuccess to true
    //      on fail fallback to AP
    //  false with clientConnectSuccess true on connection lost: try endlessly to previously verified network settings
    //  true on power on: try network settings endlessly
    boolean forceClientMode = false;

    CfgNet() {
        cfgName = "cfgNet";
        // Saved settings
        addSetting("clientConnectRetries", &clientConnectRetries, [&](uint16_t _x) { if (_x > 100) return false; else clientConnectRetries = _x; return true; }); // Limit to 100, any more is 'forever'
        // addSetting("mqttPort", &mqttPort, [&](uint16_t _x) { if (_x < 1) return false; else mqttPort = _x; return true; }); // port above 1024
        addSetting("clientSsid", &clientSsid, [&](String _x) { clientSsid = _x; return true; } ); // Check is in extra function
        addSetting("clientPass", &clientPass, [&](String _x) { clientPass = _x; return true; }); // Check is in extra function
        addSetting("forceClientMode", &forceClientMode, [&](boolean _x) { forceClientMode = _x; return true; });
    }

    bool setWifiCredentials(String newSsid, String newPass) {
        // Remove network
        if (newSsid.length() == 0) {
            clientSsid = "";
            clientPass = "";
            return true;
        }
        // Edit network
        if ((newSsid.length() >= 1) && (newSsid.length() <= 32) && (newPass.length() >= 8) && (newPass.length() <= 63)) {
            clientSsid = newSsid;
            clientPass = newPass;
            return true;
        }

        return false;
    };
};


class Net {
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

// ESP32/ESP8266 have different Wifi events
#ifdef ESP8266
        WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
#endif

    public:
        enum class NET_STATE_TYPE: uint8_t {
            CLIENT = 0,
            CONNECTING = 1,
            AP = 2,
            DISABLED = 3,
            ERROR = 4
        };
        NET_STATE_TYPE netState = NET_STATE_TYPE::DISABLED;


        CfgNet cfgNet;

        NetWeb netWeb;
        NetCom netCom;

        // enum Status: uint8_t {
        //     NONE = 0,
        //     CLIENT = 1,
        //     CONNECTING = 2,
        //     AP = 3,
        // };
        // Status status = Status::CONNECTING;

        String apSsid = "device" + String(ESPX.getChipId());

        void setup();

        void loop();

        bool editClientConnection(String newSsid, String newPass);
        bool editCfgNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg);
};

#endif