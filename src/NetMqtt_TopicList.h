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

#ifndef MVP3000_NETMQTT_TOPICLIST
#define MVP3000_NETMQTT_TOPICLIST

#include <Arduino.h>

#include <ArduinoMqttClient.h>
#ifdef ESP8266
    extern EspClass ESPX;
#else
    extern EspClassX ESPX;
#endif


typedef std::function<void(char*)> MqttDataCallback;


struct DataStructMqttTopic {
    String subtopic;
    MqttDataCallback dataCallback;

    MqttClient* mqttClient;

    DataStructMqttTopic() { }
    DataStructMqttTopic(String subtopic) : subtopic(subtopic) { } // For comparision only
    DataStructMqttTopic(String subtopic, MqttDataCallback dataCallback, MqttClient* mqttClient) : subtopic(subtopic), dataCallback(dataCallback), mqttClient(mqttClient) { }

    String getCtrlTopic() { return String(ESPX.getChipId()) + "_" + subtopic + "_ctrl"; }
    String getDataTopic() { return String(ESPX.getChipId()) + "_" + subtopic + "_data"; }

    std::function<void(const String &message)> getMqttPrint() { return std::bind(&DataStructMqttTopic::mqttPrint, this, std::placeholders::_1); }
    void mqttPrint(const String &message) {
        // Only write if connected
        if (mqttClient->connected()) {
            mqttClient->beginMessage(getDataTopic());
            mqttClient->print(message);
            mqttClient->endMessage();
        }
    }

    bool equals(DataStructMqttTopic* other) {
        if (other == nullptr)
            return false;
        // Compare the actionKey string
        return subtopic.equals(other->subtopic);
    }
};

struct LinkedListMqttTopic : LinkedListNEW3101<DataStructMqttTopic> {
    LinkedListMqttTopic(MqttClient* mqttClient) : mqttClient(mqttClient) { }

    MqttClient* mqttClient;

    boolean hasTopics() { return this->getSize(); }

    std::function<void(const String &message)> appendUnique(String subtopic, MqttDataCallback dataCallback = nullptr) {
        this->appendUniqueDataStruct(new DataStructMqttTopic(subtopic, dataCallback, mqttClient));

        return this->tail->dataStruct->getMqttPrint();
    }

    bool updateSetting(const String& key, const String& value) {
        boolean success = false;
        this->loop([&](DataStructMqttTopic* current, uint16_t i) {
            
            
        });
        return success;
    }

    boolean findAndExecute(String subtopic, char* data) {
        Node* node = this->findByContent(new DataStructMqttTopic(subtopic));
        if (node == nullptr) {
            return false;
        }
        if (node->dataStruct->dataCallback == nullptr) {
            return false;
        }
        node->dataStruct->dataCallback(data);
        return true;
    }

    void subscribeAll() {
        this->loop([&](DataStructMqttTopic* current, uint16_t i) {
            // Only subscribe if there is a callback
            if (current->dataCallback != nullptr) {
                mqttClient->subscribe(current->getCtrlTopic().c_str()); // String conversion
            }
        });
    }

    String getTopicStrings(uint8_t index) {                                             // TODO with bookmark LinkedListNEW3111
        String str;
        this->loop([&](DataStructMqttTopic* current, uint16_t i) {
            if (i == index) {
                str = current->getDataTopic() + ( (current->dataCallback == nullptr) ? "" : " | " + current->getCtrlTopic() );
                return;
            }
        });
        return str;
    }
};

#endif