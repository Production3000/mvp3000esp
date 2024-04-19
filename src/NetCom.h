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

#include "ESPX.h"
#ifdef ESP8266
    extern EspClass ESPX = ESP;
#else
    extern EspClassX ESPX;
#endif

struct CfgNetCom {

    uint16_t discoveryPort = 4211; // Search local network for MQTT broker
    uint16_t mqttPort = 1883; // 1883: unencrypted, unauthenticated

    String mqttForcedBroker = ""; // test.mosquitto.org
    String mqttTopicSuffix = "mysensor";

    String mqttTopic() { 
        char topic[64];
        snprintf(topic, 64, "MVP3000_%d_%s", ESPX.getChipId(), mqttTopicSuffix);
        return String(topic);
    };
};


class NetCom {
    private:

        WiFiClient wifiClient;
        MqttClient mqttClient = NULL;

        IPAddress mqttBrokerIp = INADDR_NONE;

        uint16_t brokerInterval = 5000;
        millisDelay brokerDelay;

        void udpDiscoverMqtt();
        void udpReceiveMessage();
        void udpSendMessage(const char *message, IPAddress remoteIp = INADDR_NONE);

        void mqttConnect();

    public:

        WiFiUDP udp;

        CfgNetCom cfgNetCom;

        void setup(CfgNetCom _cfgNetCom);
        void loop();

        String controllerConnectedString() { return (mqttBrokerIp != INADDR_NONE) ? String(mqttBrokerIp[0]) + "." + mqttBrokerIp[1] + "." + mqttBrokerIp[2] + "." + mqttBrokerIp[3] : "not connected" ; }

        void mqttWrite(const char *message);
};

#endif