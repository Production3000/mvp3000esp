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

#include "_Helper.h"
extern _Helper _helper;


typedef std::function<void(char*)> MqttCtrlCallback;


struct DataStructMqttTopic {
    String baseTopic;
    MqttCtrlCallback ctrlCallback;

    MqttClient* mqttClient;

    DataStructMqttTopic() { }
    DataStructMqttTopic(const String& baseTopic) : baseTopic(baseTopic) { } // For comparision only
    DataStructMqttTopic(const String& baseTopic, MqttCtrlCallback ctrlCallback, MqttClient* mqttClient) : baseTopic(baseTopic), ctrlCallback(ctrlCallback), mqttClient(mqttClient) { }

    String getCtrlTopic() { String str; str += _helper.ESPX->getChipId(); str += "_"; str += baseTopic; str += "_ctrl";  return str; }
    String getDataTopic() { String str; str += _helper.ESPX->getChipId(); str += "_"; str += baseTopic; str += "_data";  return str; }

    std::function<void(const String& message)> getMqttPrint() { return std::bind(&DataStructMqttTopic::mqttPrint, this, std::placeholders::_1); }
    void mqttPrint(const String& message) {
        // Only write if connected
        if (mqttClient->connected()) {
            mqttClient->beginMessage(getDataTopic());
            mqttClient->print(message);
            mqttClient->endMessage();
        }
    }
};

struct LinkedListMqttTopic : LinkedList3111<DataStructMqttTopic> {

    std::function<void(const String& message)> appendUnique(MqttClient* mqttClient, const String& baseTopic, MqttCtrlCallback ctrlCallback = nullptr) {
        this->appendUniqueDataStruct(new DataStructMqttTopic(baseTopic, ctrlCallback, mqttClient));
        return this->tail->dataStruct->getMqttPrint();
    }

    DataStructMqttTopic* findTopic(const String& baseTopic) {
        return this->findByContentData(new DataStructMqttTopic(baseTopic));
    }

    boolean compareContent(DataStructMqttTopic* dataStruct, DataStructMqttTopic* other) override {
        return dataStruct->baseTopic.equals(other->baseTopic);
    }

    boolean hasTopics() { return this->getSize(); }
};

#endif