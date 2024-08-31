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
#include <ArduinoMqttClient.h>
#include <millisDelay.h> // https://github.com/PowerBroker2/SafeString

#include "Config.h"
#include "ESPX.h"
#ifdef ESP8266
    extern EspClass ESPX;
#else
    extern EspClassX ESPX;
#endif
#include "NetWeb.h"


struct CfgNetCom : public CfgJsonInterface {

    // Fixed settings

    boolean mqttEnabled = true;

    // Modifiable settings saved to SPIFF

    uint16_t discoveryPort = 4211; // Search local network for MQTT broker
    uint16_t mqttPort = 1883; // 1883: unencrypted, unauthenticated
    String mqttForcedBroker = ""; // test.mosquitto.org
    String mqttTopicSuffix = "myesp";

    CfgNetCom() : CfgJsonInterface("cfgNetCom") {
        addSetting<uint16_t>("discoveryPort", &discoveryPort, [](uint16_t x) { return (x < 1024) ? false : true; }); // port above 1024
        addSetting<uint16_t>("mqttPort", &mqttPort, [](uint16_t x) { return (x < 1024) ? false : true; }); // port above 1024
        addSetting<String>("mqttForcedBroker", &mqttForcedBroker, [](String x) { return ((x.length() > 0) && (x.length() < 6)) ? false : true; } ); // allow empty to remove
        addSetting<String>("mqttTopicSuffix", &mqttTopicSuffix, [](String x) { return (x.length() < 5) ? false : true; }); // min 5 chars
    }
};


class NetCom {
    private:
        enum class COM_STATE_TYPE: uint8_t {
            CONNECTED = 0,
            WAITING = 1,
            DISABLEDX = 2
        };
        COM_STATE_TYPE comState = COM_STATE_TYPE::DISABLEDX;

        WiFiClient wifiClient;
        MqttClient mqttClient = NULL;

        IPAddress mqttBrokerIp = INADDR_NONE; // compare with == operator, there is

        uint16_t brokerInterval = 5000;
        millisDelay brokerDelay;

        void udpDiscoverMqtt();
        void udpReceiveMessage();
        void udpSendMessage(const char *message, IPAddress remoteIp = INADDR_NONE);

        void mqttConnect();

    public:

        WiFiUDP udp;

        CfgNetCom cfgNetCom;

        void setup();
        void loop();

        String controllerConnectedString() { return (comState == COM_STATE_TYPE::CONNECTED) ? "connected" : (comState == COM_STATE_TYPE::WAITING) ? "connecting" : "disabled" ; }

        String mqttTopicPrefix = "MVP3000_" + String(ESPX.getChipId()) + "_";

        void mqttWrite(const char *message);

    private:

        String webPageProcessor(const String& var);
        char* webPage = R"===(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %0%</title>
<script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
<style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h2>MVP3000 - Device ID %0%</h2>
<p><a href='/'>Home</a></p>
<h3>MQTT Communication</h3> <ul>
    <li>Status: %51% </li>
    <li>Auto-discovery port local broker: 1024-65535, default is 4211.<br> <form action='/save' method='post'> <input name='discoveryPort' value='%52%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li>
    <li>Forced external broker:<br> <form action='/save' method='post'> <input name='mqttForcedBroker' value='%53%'> <input type='submit' value='Save'> </form> </li>
    <li>MQTT port: default is 1883 (unsecure) <br> <form action='/save' method='post'> <input name='mqttPort' value='%54%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li>
    <li>Topic: <br> <form action='/save' method='post'> %55% <input name='mqttTopicSuffix' value='%56%' minlength='5'> <input type='submit' value='Save'> </form> </li> </ul>
<p>&nbsp;</body></html>
)===";
};

#endif