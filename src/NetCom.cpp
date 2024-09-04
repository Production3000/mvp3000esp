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


#include "NetCom.h"

#include "MVP3000.h"
extern MVP3000 mvp;


void NetCom::setup() {
    // Read config and register with web interface
    mvp.config.readCfg(cfgNetCom);

    if (!cfgNetCom.mqttEnabled)
        return;

    comState = COM_STATE_TYPE::WAITING;

    // Start UDP even if external forcedBroker is set, to allow reverse-discovery of this ESP device
    udp.begin(cfgNetCom.discoveryPort);


    // Define web page
    mvp.net.netWeb.registerPage("/netcom", webPage ,  std::bind(&NetCom::webPageProcessor, this, std::placeholders::_1)); 
    // Register config
    mvp.net.netWeb.registerCfg(&cfgNetCom);

    // For some reason the mqttClient.onMessage() method does not work when the function is in a class ...
    // mqttClient.onMessage(onMqttMessage); // argument of type "void (NetCom::*)(int messageSize)" is incompatible with parameter of type "void (*)(int)"
    // mqttClient.onMessage([] (int messageSize) { onMqttMessage; }); // invalid use of non-static member function 'void NetCom::onMqttMessage(int)'
    // mqttClient.onMessage([&] (int messageSize) { onMqttMessage; }); // no suitable conversion function from "lambda [](int messageSize)->void" to "void (*)(int)" exists
    // mqttClient.onMessage(std::bind(&NetCom::onMqttMessage, this, std::placeholders::_1)); // no suitable conversion function from "std::_Bind_helper<false, void (NetCom::*)(int messageSize), NetCom *, const std::_Placeholder<1> &>::type" (aka "std::_Bind<std::__remove_cv_t<void (NetCom::*)(int messageSize)> (std::__remove_cv_t<NetCom *>, std::__remove_cv_t<const std::_Placeholder<1>>)>") to "void (*)(int)" exists
};

void NetCom::loop() {
    // Called from net.loop() only if wifi is up and in client mode

    // switch (comState) {
    //     case COM_STATE_TYPE::CONNECTED:
    //         // Check if state is still connected
    //         if (mqttClient.connected()) {
    //             // Check for UDP packet, in that case handle it
    //             if (udp.parsePacket())
    //                 udpReceiveMessage();

    //             // Handle MQTT messages, keep-alive, etc. 
    //             // mqttClient.poll();
    //             // Check if a MQTT message was received
    //             // Would be nicer with the mqttClient.onMessage() method, but that seems to not work in a class
    //             int messageSize = mqttClient.parseMessage();
    //             if (messageSize > 0) { // parseMessage already calls poll()
    //                 onMqttMessage(messageSize);
    //             }


    //         } else {
    //             comState = COM_STATE_TYPE::WAITING;
    //             mvp.logger.write(CfgLogger::Level::WARNING, "Disconnected from MQTT broker.");
    //         }
    //         break;

    //     case COM_STATE_TYPE::WAITING:
    //         if (!mqttClient.connected()) {
    //             // Only work to do if interval not yet started or just finished
    //             if (!brokerDelay.isRunning() || brokerDelay.justFinished()) {
    //                 // Connect to forced broker or discover broker on local network
    //                 if ((mqttBrokerIp != INADDR_NONE) || (cfgNetCom.mqttForcedBroker.length() > 0)) {
    //                     // Connect to broker
    //                     mqttConnect();
    //                 } else {
    //                     // Auto-discover local broker IP only if no forced broker
    //                     udpDiscoverMqtt();
    //                 }
    //                 // (Re)start interval
    //                 brokerDelay.start(brokerInterval);
    //             }
    //         } else {
    //             comState = COM_STATE_TYPE::CONNECTED;
    //             brokerDelay.stop();
    //             mvp.logger.write(CfgLogger::Level::INFO, "Connected to MQTT broker.");

    //             mqttTopicList.subscribeAll();

    //         }
    //         break;
        
    //     case COM_STATE_TYPE::DISABLEDX: // Nothing to do
    //         break;

    //     default:
    //         break;
    // }
}



void NetCom::udpDiscoverMqtt() {
    // Send DIscover SERVer to broadcast
    udpSendMessage("DISERV", WiFi.broadcastIP()); 

    mvp.logger.write(CfgLogger::Level::INFO, "Discovery request sent.");
}

void NetCom::udpReceiveMessage() {
    char packetBuffer[8];
    uint8_t len = udp.read(packetBuffer, 8);

    // Length is 4, 6, 6+4, 6+4+2+2 characters
    if (len != 6)
        return;

    // Terminate char string
    packetBuffer[len] = '\0';

    switch (mvp.helper.hashStringDjb2(packetBuffer)) {
        case mvp.helper.hashStringDjb2("RESERV"): // RESERV Discovery response from server received
            // Remember controller IP, stop interval, registering with broker is done in next loop
            mqttBrokerIp = udp.remoteIP();
            brokerDelay.stop();
            mvp.logger.write(CfgLogger::Level::INFO, "Discovery response received.");
            break;
        case mvp.helper.hashStringDjb2("DISENS"): // DISENS Discovery request received, send discovery response
            udpSendMessage("RESENS", udp.remoteIP());
            mvp.logger.write(CfgLogger::Level::INFO, "Discovery request received.");
            break;
    }
}

void NetCom::udpSendMessage(const char *message, IPAddress remoteIp) {
    // Test this using netcat: nc -ul [laptopIP] [port]

    // Standard call is without remoteIp and defaults to controller IP
    if (remoteIp == INADDR_NONE) {
        // Check if controller is available
        if (mqttBrokerIp == INADDR_NONE) {
            mvp.logger.write(CfgLogger::Level::WARNING, "UDP not sent, no remote/controller IP.");
            return;
        }
        remoteIp = mqttBrokerIp;
    }

    // Return for empty message/datastring
    if (strlen(message) == 0) {
        mvp.logger.write(CfgLogger::Level::INFO, "UDP not sent, message empty.");
        return;
    }

    // Send UDP packet
    if (!udp.beginPacket(remoteIp, cfgNetCom.discoveryPort))
        mvp.logger.write(CfgLogger::Level::WARNING, "UDP not sent, send error.");
    for (uint16_t i = 0; i < strlen(message); i++) {
        udp.write((uint8_t)message[i]);
    }
    if (!udp.endPacket())
        mvp.logger.write(CfgLogger::Level::WARNING, "UDP not sent, send error.");   
}



String NetCom::webPageProcessor(const String& var) { 
    if (!mvp.helper.isValidInteger(var)) {
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid placeholder in template: %s", var.c_str());
        return var;
    }

    switch (var.toInt()) {
        case 0:
            return String(ESPX.getChipId());

        case 52:
            return String(cfgNetCom.discoveryPort);

        default:
            break;
    }
    mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid placeholder in template: %s", var.c_str());
    return var;
}