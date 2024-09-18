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

    // Fixed settings

    boolean isHardDisabled = false;

    // Modifiable settings saved to SPIFF

    boolean udpEnabled = true;
    uint16_t discoveryPort = 4211;

    CfgNetCom() : CfgJsonInterface("cfgNetCom") {
        addSetting<boolean>("udpEnabled", &udpEnabled, [](boolean _) { return true; });
        addSetting<uint16_t>("discoveryPort", &discoveryPort, [](uint16_t x) { return (x < 1024) ? false : true; }); // port above 1024
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
            ENABLED = 0,
            DISABLEDX = 1,
            HARDDISABLED = 1
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
        void udpSendMessage(const char *message, IPAddress remoteIp = INADDR_NONE);


        String webPageProcessor(uint8_t var);
        const char* webPage = R"===(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %1%</title>
<script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
<style>body { font-family: sans-serif; } table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h2>MVP3000 - Device ID %1%</h2> <h3 style='color: red;'>%0%</h3>
<p><a href='/'>Home</a></p>
<h3>UDP Auto-Discovery</h3> <ul>
    <li>Enable: <form action='/save' method='post'> <input name='udpEnabled' type='checkbox' %41% value='1'> <input name='udpEnabled' type='hidden' value='0'> <input type='submit' value='Save'> </form> </li>
    <li>Auto-discovery port: 1024-65535, default is 4211.<br> <form action='/save' method='post'> <input name='discoveryPort' value='%42%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li>
    <li>Discovery: %43% </li> </ul>
<p>&nbsp;</body></html>
)===";
};

#endif