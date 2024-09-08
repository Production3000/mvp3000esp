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


typedef std::function<void(char*)> MqttDataCallback;

struct MqttTopicList {
    struct Node {
        String topic;
        MqttDataCallback dataCallback;

        Node* next;

    MqttClient* mqttClient;

        Node() { }
        Node(String topic, MqttDataCallback dataCallback, MqttClient* mqttClient) : topic(topic), dataCallback(dataCallback), mqttClient(mqttClient) { }

        String getCtrlTopic() { return topic + "_ctrl"; }
        String getDataTopic() { return topic + "_data"; }

        std::function<void(const String &message)> getMqttPrint() { return std::bind(&Node::mqttPrint, this, std::placeholders::_1); }
        void mqttPrint(const String &message) {
            // Only write if connected
            if (mqttClient->connected()) {
                mqttClient->beginMessage(getDataTopic());
                mqttClient->print(message);
                mqttClient->endMessage();
            }
        }
    };

    Node* head = nullptr;

    MqttClient* mqttClient;


    MqttTopicList(MqttClient* mqttClient) : mqttClient(mqttClient) { }


    std::function<void(const String &message)> add(String topic, MqttDataCallback dataCallback = nullptr) {
        Node* newNode = new Node(topic , dataCallback, mqttClient);
        newNode->next = head;
        head = newNode;

        return newNode->getMqttPrint();
    }

    boolean findAndExecute(String topic, char* data) {
        Node* current = head;
        while (current != nullptr) {
            if (current->topic.compareTo(topic)) {
                current->dataCallback(data);
                return true;
            }
            current = current->next;
        }
        return false;
    }

    boolean hasTopics() { return head != nullptr; }

    void subscribeAll() {
        Node* current = head;
        while (current != nullptr) {
            // Only subscribe if there is a callback
            if (current->dataCallback != nullptr) {
                mqttClient->subscribe(current->getCtrlTopic().c_str());
            }
            current = current->next;
        }
    }

    String getTopicStrings(uint8_t index) {
        Node* current = head;
        uint8_t counter = 0;
        while (current != nullptr) {
            if (counter++ == index) {
                return current->getDataTopic() + ( (current->dataCallback == nullptr) ? "" : " | " + current->getCtrlTopic() ) ;
            }
            current = current->next;
        }
        return "";
    }
};

#endif