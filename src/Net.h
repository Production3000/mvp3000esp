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
    String clientSsid = ""; // IEEE 1-32 chars
    String clientPass = ""; // IEEE 8-63 chars

    uint8_t clientConnectRetries = 3;
    // DANGER, clientConnectForever could make the device inaccessable for the user
    // Is used to reconnect endlessly to a previously sucessfully connected network (until reboot)
    boolean clientConnectForever = false;

    String apSsid = "device" + String(ESPX.getChipId());

    CfgNetCom cfgNetCom;

    // Init defaults
    CfgNet() { };
    // Init with custom client connect retries
    CfgNet(uint8_t _clientConnectRetries) : clientConnectRetries(_clientConnectRetries) { };
};


class Net {
    private:
        DNSServer dnsServer;

        // Counter for client conenct fails
        uint8_t clientConnectFails = 0;
    
        // #ifdef ESP8266
        //     WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
        // #else
        //     WiFiEventFuncCb gotIpEventHandler, disconnectedEventHandler;
        // #endif

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

        void setup(CfgNet _cfgNet);

        void loop();

        void editClientConnection(String newSsid, String newPass);

        boolean connectedAsClient() { return (status == Status::CLIENT); }

};

#endif