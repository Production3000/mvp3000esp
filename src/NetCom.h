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

#ifndef MVP3000_NETCOM
#define MVP3000_NETCOM

#include <Arduino.h>

#ifdef ESP8266
    #include <ESP8266WiFi.h>
#else
    #include <WiFi.h>
    #include <IPAddress.h>
#endif
#include <WiFiUdp.h>

#include "Config.h"

#include "_Helper_LimitTimer.h"


struct CfgNetCom : public CfgJsonInterface {

    boolean isHardDisabled = false;

    // Modifiable settings saved to SPIFF

    uint16_t discoveryPort = 4211;

    CfgNetCom() : CfgJsonInterface("cfgNetCom") {
        addSetting<uint16_t>("discoveryPort", &discoveryPort, [&](const String& s) { uint16_t n = s.toInt(); if (n < 1024) return false; discoveryPort = n; return true; } ); // Port above 1024
    }
};


class NetCom {

    public:

        void setup();
        void loop();

        IPAddress checkSkill(const String& requestedSkill);

        void hardDisable() { cfgNetCom.isHardDisabled = true; }
        boolean isHardDisabled() { return cfgNetCom.isHardDisabled; }

    private:

        enum class UDP_STATE: uint8_t {
            HARDDISABLED = 0, // DISABLED is reserved
            ENABLED = 1,
        };
        UDP_STATE udpState;

        CfgNetCom cfgNetCom;
        void saveCfgCallback();

        WiFiUDP udp;

        IPAddress serverIp = INADDR_NONE;
        String serverSkills = "";

        uint16_t discoveryInterval = 10000; // 10 seconds
        uint64_t lastDiscovery = 0;
        LimitTimer discoveryTimer = LimitTimer(discoveryInterval);

        void sendDiscovery();

        void udpReceiveMessage();
        void udpSendMessage(const char* message, IPAddress remoteIp = INADDR_NONE);

    public:

        String templateProcessor(uint16_t var);
        PGM_P getHtml();

};

#endif