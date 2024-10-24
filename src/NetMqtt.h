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


typedef std::function<void(const String&)> NetworkCtrlCallback;


struct CfgNetMqtt : public CfgJsonInterface {

    boolean isHardDisabled = false;

    // Modifiable settings saved to SPIFF

    uint16_t mqttPort = 1883; // 1883: unencrypted, unauthenticated
    String mqttForcedBroker = ""; // test.mosquitto.org

    CfgNetMqtt() : CfgJsonInterface("cfgNetMqtt") {
        addSetting<uint16_t>("mqttPort", &mqttPort, [&](const String& s) { uint16_t n = s.toInt(); if (n < 1024) return false; mqttPort = n; return true; } ); // Port above 1024
        addSetting<String>("mqttForcedBroker", &mqttForcedBroker, [&](const String& s) { mqttForcedBroker = s; return true; } ); // Allow empty to remove
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
        void registerMqtt(const String& topic, NetworkCtrlCallback ctrlCallback = nullptr);

        void hardDisable() { cfgNetMqtt.isHardDisabled = true; }
        boolean isHardDisabled() { return cfgNetMqtt.isHardDisabled; }

        void printMqtt(const String& topic, const String& message);

    private:

        struct DataStructMqttTopic {
            String baseTopic;
            NetworkCtrlCallback ctrlCallback;

            DataStructMqttTopic() { }
            DataStructMqttTopic(const String& baseTopic) : baseTopic(baseTopic) { } // For comparision only
            DataStructMqttTopic(const String& baseTopic, NetworkCtrlCallback ctrlCallback) : baseTopic(baseTopic), ctrlCallback(ctrlCallback) { }

            String getCtrlTopic() { String str; str += _helper.ESPX->getChipId(); str += "_"; str += baseTopic; str += "_ctrl";  return str; }
            String getDataTopic() { String str; str += _helper.ESPX->getChipId(); str += "_"; str += baseTopic; str += "_data";  return str; }
        };

        struct LinkedListMqttTopic : LinkedList3111<DataStructMqttTopic> {
            
            void appendUnique(const String& baseTopic, NetworkCtrlCallback ctrlCallback = nullptr) {
                this->appendUniqueDataStruct(new DataStructMqttTopic(baseTopic, ctrlCallback));
            }

            DataStructMqttTopic* findTopic(const String& baseTopic) {
                return this->findByContentData(DataStructMqttTopic(baseTopic));
            }

            boolean compareContent(DataStructMqttTopic* dataStruct, DataStructMqttTopic* other) override {
                return dataStruct->baseTopic.equals(other->baseTopic);
            }
        };

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

        MqttClient mqttClient = nullptr;

        IPAddress localBrokerIp = INADDR_NONE; // compare with == operator, there is

        uint16_t connectInterval = 5000;
        uint8_t connectTries = 3;
        LimitTimer connectTimer = LimitTimer(connectInterval, connectTries);

        void lateSetup();

        void saveCfgCallback();

        void connectMqtt();

        void handleMessage();

    public:

        String templateProcessor(uint16_t var);
        PGM_P getHtml();

};

#endif