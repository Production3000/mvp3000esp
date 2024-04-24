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
    // using mfn = void(*)(uint16_t _);
    // struct Setting{
    //     String key;
    //     int value;
    //     mfn fn;
    // };
    // Setting settings[4] =  {
    //     {"discoveryPort", discoveryPort, [](uint16_t _dP) { Serial.println("No Fn"); }},
    //     {"mqttPort", mqttPort, [](uint16_t _mP) { Serial.println("No Fn"); }},
    //     // {"mqttForcedBroker", mqttForcedBroker},
    //     // {"mqttTopicSuffix", mqttTopicSuffix}
    // };

    // String getkeyof(Setting& f) { return f.key; }
    // uint16_t getvalueof(Setting& f) { return f.value; }
    // mfn getfunctionof(Setting& f) { return f.fn; }

    uint16_t discoveryPort = 4211; // Search local network for MQTT broker

    uint16_t mqttPort = 1883; // 1883: unencrypted, unauthenticated
    String mqttForcedBroker = ""; // test.mosquitto.org
    String mqttTopicSuffix = "myesp";

    bool setDiscoveryPort(uint16_t _discoveryPort) {
        if (_discoveryPort < 1024)
            return false;
        discoveryPort = _discoveryPort;
        return true;
    };

    bool setMqttPort(uint16_t _mqttPort) {
        if (_mqttPort < 1024)
            return false;
        mqttPort = _mqttPort;
        return true;
    };

    bool setMqttForcedBroker(String _mqttForcedBroker) {
        if ((_mqttForcedBroker.length() > 0) && (_mqttForcedBroker.length() < 6)) // allow empty to remove
            return false;
        mqttForcedBroker = _mqttForcedBroker;
        return true;
    };

    bool setMqttTopicSuffix(String _mqttTopicSuffix) {
        if (_mqttTopicSuffix.length() < 5)
            return false;
        mqttTopicSuffix = _mqttTopicSuffix;
        return true;
    };
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

        void setup(CfgNetCom _cfgNetCom);
        void loop();

        String controllerConnectedString() { return (mqttBrokerIp != INADDR_NONE) ? String(mqttBrokerIp[0]) + "." + mqttBrokerIp[1] + "." + mqttBrokerIp[2] + "." + mqttBrokerIp[3] : "not connected" ; }

        String mqttTopicPrefix = "MVP3000_" + String(ESPX.getChipId()) + "_";

        void mqttWrite(const char *message);


        bool editCfg(String varName, String newValue);
};

#endif