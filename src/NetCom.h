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
    extern EspClass ESPX = ESP;
#else
    extern EspClassX ESPX;
#endif


struct CfgNetCom : public Cfg {

    // Modifiable settings saved to SPIFF

    uint16_t discoveryPort = 4211; // Search local network for MQTT broker
    uint16_t mqttPort = 1883; // 1883: unencrypted, unauthenticated
    String mqttForcedBroker = ""; // test.mosquitto.org
    String mqttTopicSuffix = "myesp";

    CfgNetCom() {
        cfgName = "cfgNetCom";
        addSetting("discoveryPort", &discoveryPort, [&](uint16_t _x) { if (_x < 1024) return false; else discoveryPort = _x; return true; }); // port above 1024
        addSetting("mqttPort", &mqttPort, [&](uint16_t _x) { if (_x < 1024) return false; else mqttPort = _x; return true; }); // port above 1024
        addSetting("mqttForcedBroker", &mqttForcedBroker, [&](String _x) { if ((_x.length() > 0) && (_x.length() < 6)) return false; else mqttForcedBroker = _x; return true; } ); // allow empty to remove
        addSetting("mqttTopicSuffix", &mqttTopicSuffix, [&](String _x) { if (_x.length() < 5) return false; else mqttTopicSuffix = _x; return true; }); // min 5 chars
    }
};


class NetCom {
    private:

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

        String controllerConnectedString() { return (mqttBrokerIp != INADDR_NONE) ? String(mqttBrokerIp[0]) + "." + mqttBrokerIp[1] + "." + mqttBrokerIp[2] + "." + mqttBrokerIp[3] : "not connected" ; }

        String mqttTopicPrefix = "MVP3000_" + String(ESPX.getChipId()) + "_";

        void mqttWrite(const char *message);

        bool editCfgNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg);
};

#endif