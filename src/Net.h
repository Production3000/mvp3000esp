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


#include "NetCom.h"
#include "NetWeb.h"

#include "ESPX.h"
#ifdef ESP8266
    extern EspClass ESPX = ESP;
#else
    extern EspClassX ESPX;
#endif


struct CfgNet {

    CfgNetCom cfgNetCom;

    String clientSsid = ""; // IEEE 1-32 chars
    String clientPass = ""; // IEEE 8-63 chars
    uint8_t clientConnectRetries = 3;

    // DANGER, forceClientMode could make the device inaccessable for the user without flashing it
    // Usefull when network settings are correct for sure but likely offline during power on
    //  false on power on: try network settings clientConnectRetries times
    //      on success set clientConnectSuccess to true
    //      on fail fallback to AP
    //  false with clientConnectSuccess true on connection lost: try endlessly to previously verified network settings
    //  true on power on: try network settings endlessly
    boolean forceClientMode = false;

    bool setWifiCredentials(String newSsid, String newPass) {
        clientSsid = newSsid;
        clientPass = newPass;
        return true;
    };

    bool setClientConnectRetries(uint8_t _clientConnectRetries) {
        if (_clientConnectRetries == 0)
            return false;
        clientConnectRetries = _clientConnectRetries;
        return true;
    };

    bool setForceClientMode(boolean _forceClientMode) {
        forceClientMode = _forceClientMode;
        return true;
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

    public:

        CfgNet cfgNet;

        NetWeb netWeb;
        NetCom netCom;

        enum Status: uint8_t {
            NONE = 0,
            CLIENT = 1,
            CONNECTING = 2,
            AP = 3,
        };
        Status status = Status::CONNECTING;

        String apSsid = "device" + String(ESPX.getChipId());

        void setup(CfgNet _cfgNet);

        void loop();

        bool editClientConnection(String newSsid, String newPass);
        bool editCfg(String varName, String newValue, String newValueX);

        boolean connectedAsClient() { return (status == Status::CLIENT); }

};

#endif