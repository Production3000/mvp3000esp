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

///////////////////////////////////////////////////////////////////////////////////

/*

    Server sends a DIscover SENSor to broadcast, ESP responds with REsponse SENSor
        DISENS RESENS
    ESP sends a DIscover SERVer to broadcast, server responds with REsponse SERVer
        DISERV RESERV

*/

///////////////////////////////////////////////////////////////////////////////////


#include "NetMqtt.h"

#include "MVP3000.h"
extern MVP3000 mvp;


void NetMqtt::setup() {
    // Read config and register with web interface
    mvp.config.readCfg(cfgNetMqtt);

    if (!cfgNetMqtt.mqttEnabled)
        return;

    // Redefine needed with network, otherwise mqttClient.connected() crashes
    mqttClient = MqttClient(wifiClient);

    // Define web page
    mvp.net.netWeb.registerPage("/netmqtt", webPage ,  std::bind(&NetMqtt::webPageProcessor, this, std::placeholders::_1)); 
    // Register config
    mvp.net.netWeb.registerCfg(&cfgNetMqtt);

    // For some reason the mqttClient.onMessage() method does not work when the function is in a class ...
    // mqttClient.onMessage(handleMessage); // argument of type "void (NetMqtt::*)(int messageSize)" is incompatible with parameter of type "void (*)(int)"
    // mqttClient.onMessage([] (int messageSize) { handleMessage; }); // invalid use of non-static member function 'void NetMqtt::handleMessage(int)'
    // mqttClient.onMessage([&] (int messageSize) { handleMessage; }); // no suitable conversion function from "lambda [](int messageSize)->void" to "void (*)(int)" exists
    // mqttClient.onMessage(std::bind(&NetMqtt::handleMessage, this, std::placeholders::_1)); // no suitable conversion function from "std::_Bind_helper<false, void (NetMqtt::*)(int messageSize), NetMqtt *, const std::_Placeholder<1> &>::type" (aka "std::_Bind<std::__remove_cv_t<void (NetMqtt::*)(int messageSize)> (std::__remove_cv_t<NetMqtt *>, std::__remove_cv_t<const std::_Placeholder<1>>)>") to "void (*)(int)" exists
};

void NetMqtt::loop() {
    // Called from net.loop() only if wifi is up and in client mode

    // int messageSize = 0;
    switch (mqttState) {
        case MQTT_STATE::DISCONNECTED:
            // Check state change
            if (mqttClient.connected()) {
                mqttState = MQTT_STATE::CONNECTED;
                mvp.logger.write(CfgLogger::Level::INFO, "Connected to MQTT broker.");

                mqttTopicList.subscribeAll();
                break;
            }

            // Try to connect to broker
            mqttConnect();
            break;

        case MQTT_STATE::CONNECTED:
            // Check state change
            if (!mqttClient.connected()) {
                mqttState = MQTT_STATE::DISCONNECTED;
                mvp.logger.write(CfgLogger::Level::WARNING, "Disconnected from MQTT broker.");
                break;
            }

            // When mqttClient.onMessage() would work we could just call mqttClient.poll(); here
            // Check if a MQTT message was received, parseMessage calls poll() itself
            int messageSize = mqttClient.parseMessage(); 
            if (messageSize > 0) {
                handleMessage(messageSize);
            }
            break;

    }
}


///////////////////////////////////////////////////////////////////////////////////

std::function<void(const String &message)> NetMqtt::registerMqtt(String topic, std::function<void(char*)> dataCallback) {
    // Store topic and callback for registering with MQTT, return the function to write to this topic
    return mqttTopicList.add(topic, dataCallback);
}

void NetMqtt::mqttConnect() {
    // Only work to do if interval not yet started or just finished
    if (connectDelay.isRunning() && !connectDelay.justFinished())                                                                               // TODO there should be a upper limit of iterations/connect tries
        return;
    // (Re)start interval
    connectDelay.start(connectInterval);

    if (cfgNetMqtt.mqttForcedBroker.length() > 0) {
        // Connect to forced broker
        mqttClient.connect(cfgNetMqtt.mqttForcedBroker.c_str(), cfgNetMqtt.mqttPort);
        mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Connecting to MQTT broker: %s", cfgNetMqtt.mqttForcedBroker.c_str());
    } else if (localBrokerIp != INADDR_NONE) {
        // Connect to discovered broker
        // The library is broken for ESP8266, it does not accept the IPAddress-type when a port is given
        mqttClient.connect(localBrokerIp.toString().c_str(), cfgNetMqtt.mqttPort);
        mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Connecting to MQTT broker: %s", localBrokerIp.toString().c_str());                    // TODO count tries, remove / give up after 3 tries
    } else {
        // Auto-discover local broker IP only if no forced broker
        localBrokerIp = mvp.net.netCom.checkSkill("MQTT");
    }
}

void NetMqtt::handleMessage(int messageSize) {
    // Check if message is a duplicate, requires QoS 1+ and needs to be implemented by the sender and the receiver
    if (mqttClient.messageDup())
        return; // Handling of duplicates not implemented

    String topic = mqttClient.messageTopic();
    
    // Copy message to buffer, needs to be done after reading the topic as it clears the message-ready flag
    uint8_t buf[messageSize + 1];
    mqttClient.read(buf, messageSize + 1);
    buf[messageSize] = '\0';

    // Find the topic in the list and execute callback
    if (!mqttTopicList.findAndExecute(topic, (char *)buf))
        mvp.logger.writeFormatted(CfgLogger::Level::CONTROL, "MQTT control with unknown topic '%s'", topic.c_str());
}


///////////////////////////////////////////////////////////////////////////////////

String NetMqtt::webPageProcessor(const String& var) { 
    if (!mvp.helper.isValidInteger(var)) {
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid placeholder in template: %s", var.c_str());
        return var;
    }

    switch (var.toInt()) {
        case 0:
            return String(ESPX.getChipId());

        case 51:
            return (mqttState == MQTT_STATE::CONNECTED) ? "connected" : "disconnected" ;
        case 53:
            return cfgNetMqtt.mqttForcedBroker.c_str();
        case 54:
            return String(cfgNetMqtt.mqttPort);
        case 55:
            return String("MVP3000_") + String(ESPX.getChipId()) + "_";                                                                  // TODO not correct                                  
        case 56:
            return cfgNetMqtt.mqttTopicSuffix.c_str();

        default:
            break;
    }
    mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid placeholder in template: %s", var.c_str());
    return var;
}