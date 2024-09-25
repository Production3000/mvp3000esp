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

#ifndef MVP3000_NETMQTT
#define MVP3000_NETMQTT

#include <Arduino.h>

#ifdef ESP8266
    #include <ESP8266WiFi.h>
#else
    #include <WiFi.h>
    #include <IPAddress.h>
#endif
#include <WiFiUdp.h>
#include <ArduinoMqttClient.h>

#include "Config.h"

#include "_Helper_LimitTimer.h"

#include "NetMqtt_TopicList.h"


struct CfgNetMqtt : public CfgJsonInterface {

    boolean isHardDisabled = false;

    // Modifiable settings saved to SPIFF

    uint16_t mqttPort = 1883; // 1883: unencrypted, unauthenticated
    String mqttForcedBroker = ""; // test.mosquitto.org

    CfgNetMqtt() : CfgJsonInterface("cfgNetMqtt") {
        addSetting<uint16_t>("mqttPort", &mqttPort, [](uint16_t x) { return (x < 1024) ? false : true; }); // port above 1024
        addSetting<String>("mqttForcedBroker", &mqttForcedBroker, [](const String& x) { return ((x.length() > 0) && (x.length() < 6)) ? false : true; } ); // allow empty to remove
    }
};


class NetMqtt {

    public:

        void setup();
        void loop();

        /**
         * @brief Register a topic for MQTT communication.
         *
         * @param topic The topic to register. It is prefixed with the device ID and suffixed with _data and _ctrl.
         * @param ctrlCallback The function to call when data is received on the topic suffixed with _ctrl. Omit to not subscribe to the topic.
         * @return Returns the function to write data to MQTT.
         */
        void registerMqtt(const String& topic, MqttCtrlCallback ctrlCallback = nullptr);

        void hardDisable() { cfgNetMqtt.isHardDisabled = true; }
        boolean isHardDisabled() { return cfgNetMqtt.isHardDisabled; }

        void printMqtt(const String& topic, const String& message);

    private:

        enum class MQTT_STATE: uint8_t {
            HARDDISABLED = 0,
            INIT = 1,
            NOTOPIC = 2,
            NOBROKER = 3,
            FAILED = 4,
            DISCONNECTED = 5,
            CONNECTING = 6,
            CONNECTED = 7,
        };
        MQTT_STATE mqttState;

        LinkedListMqttTopic linkedListMqttTopic; // Adaptive size

        CfgNetMqtt cfgNetMqtt;

        WiFiClient wifiClient;

        MqttClient mqttClient = NULL;

        IPAddress localBrokerIp = INADDR_NONE; // compare with == operator, there is

        uint16_t connectInterval = 5000;
        uint8_t connectTries = 3;
        LimitTimer connectTimer = LimitTimer(connectInterval, connectTries);

        void lateSetup();

        void saveCfgCallback();

        void connectMqtt();

        void handleMessage();

    public:

        String templateProcessor(uint8_t var);
        const char* webPage = R"===(
<h3>MQTT Communication</h3> <ul>
<li>Status: %62% </li>
<li>Local broker: %63% </li>
<li>Forced external broker:<br> <form action='/save' method='post'> <input name='mqttForcedBroker' value='%64%'> <input type='submit' value='Save'> </form> </li>
<li>MQTT port: default is 1883 (unsecure) <br> <form action='/save' method='post'> <input name='mqttPort' value='%65%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li>
<li>Topics: <ul> %70% </ul> </li> </ul>
)===";

        const char* webPageHardDisabled = "<h3>MQTT Communication (DISABLED)</h3>";
        const char* webPageNoTopics = "<h3>MQTT Communication (No Topics)</h3>";

};

#endif