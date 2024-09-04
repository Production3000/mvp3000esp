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
#include "ESPX.h"
#ifdef ESP8266
    extern EspClass ESPX;
#else
    extern EspClassX ESPX;
#endif


struct CfgNetCom : public CfgJsonInterface {

    // Fixed settings

    boolean udpEnabled = true;

    // Modifiable settings saved to SPIFF

    uint16_t discoveryPort = 4211; // Search local network for MQTT broker

    CfgNetCom() : CfgJsonInterface("cfgNetCom") {
        addSetting<uint16_t>("discoveryPort", &discoveryPort, [](uint16_t x) { return (x < 1024) ? false : true; }); // port above 1024
    }
};


class NetCom {

    public:

        void setup();
        void loop();

        IPAddress checkSkill(String requestedSkill);

    private:

        CfgNetCom cfgNetCom;

        WiFiUDP udp;

        IPAddress serverIp = INADDR_NONE; // compare with == operator, there is
        String serverSkills = "";

        uint64_t lastDiscovery = 0;
        uint16_t discoveryInterval = 5000; // 5 seconds
        void sendDiscovery();

        void udpReceiveMessage();
        void udpSendMessage(const char *message, IPAddress remoteIp = INADDR_NONE);


        String webPageProcessor(const String& var);
        char* webPage = R"===(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %0%</title>
<script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
<style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h2>MVP3000 - Device ID %0%</h2>
<p><a href='/'>Home</a></p>
<h3>MQTT Communication</h3> <ul>
    <li>Status: XXX </li>
    <li>Auto-discovery port local broker: 1024-65535, default is 4211.<br> <form action='/save' method='post'> <input name='discoveryPort' value='%52%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li> </ul>
<p>&nbsp;</body></html>
)===";
};

#endif