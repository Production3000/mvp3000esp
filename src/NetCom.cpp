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


void NetCom::setup(CfgNetCom _cfgNetCom) {
    cfgNetCom = _cfgNetCom;

    // Load save credentials
    if (mvp.config.cfgReadPrepare("cfgNetCom")) {
        // Make sure saved ssid and pass are both readable before overwriting defaults
        String tmp;
        if (mvp.config.cfgReadGetValue("mqttForcedBroker", tmp)) {
            mvp.config.cfgReadGetValue("mqttForcedBroker", cfgNetCom.mqttForcedBroker);
            mvp.config.cfgReadGetValue("mqttTopicSuffix", cfgNetCom.mqttTopicSuffix);
            mvp.config.cfgReadGetValue("discoveryPort", cfgNetCom.discoveryPort);
            mvp.config.cfgReadGetValue("mqttPort", cfgNetCom.mqttPort);
            mvp.logger.write(CfgLogger::Level::INFO, "NetComCfg loaded.");
        }
    }
    mvp.config.cfgReadClose();

    // Redefine needed with network, otherweise mqttClient.connected() crashes
    mqttClient = MqttClient(wifiClient);
    // Start UDP independent of forcedBroker, to allow reverse-discovery of this ESP device
    udp.begin(cfgNetCom.discoveryPort);
};

void NetCom::loop() {
    // Called from net.loop() only if network is up and in client mode
    if (!mvp.net.connectedAsClient())
        return;

    // Check for UDP packet, handle it
    if (udp.parsePacket())
        udpReceiveMessage();

    if (!mqttClient.connected()) {
        // Interval never started or just finished
        if (!brokerDelay.isRunning() || brokerDelay.justFinished()) {
            // Start interval
            brokerDelay.start(brokerInterval);

            if ((mqttBrokerIp != INADDR_NONE) || (cfgNetCom.mqttForcedBroker.length() > 0)) {
                // Connect to broker
                mqttConnect();
            } else {
                // Autodiscover local broker IP only if no forced broker
                udpDiscoverMqtt();
            }
        }
    }
}

bool NetCom::editCfg(String varName, String newValue) {

    bool success = false;
    switch (mvp.helper.hashStringDjb2(varName.c_str())) {
        case mvp.helper.hashStringDjb2("discoveryPort"):
            success = cfgNetCom.setDiscoveryPort(newValue.toInt());
            break;
        case mvp.helper.hashStringDjb2("mqttForcedBroker"): 
            success = cfgNetCom.setMqttForcedBroker(newValue);
            break;
        case mvp.helper.hashStringDjb2("mqttPort"): 
            success = cfgNetCom.setMqttPort(newValue.toInt());
            break;
        case mvp.helper.hashStringDjb2("mqttTopicSuffix"):
            success = cfgNetCom.setMqttTopicSuffix(newValue);
            break;
    }

    if (success) {
        // save cfg
        mvp.config.cfgWritePrepare();
        mvp.config.cfgWriteAddValue("discoveryPort", cfgNetCom.discoveryPort);
        mvp.config.cfgWriteAddValue("mqttForcedBroker", cfgNetCom.mqttForcedBroker);
        mvp.config.cfgWriteAddValue("mqttPort", cfgNetCom.mqttPort);
        mvp.config.cfgWriteAddValue("mqttTopicSuffix", cfgNetCom.mqttTopicSuffix);
        mvp.config.cfgWriteClose("cfgNetCom");
        mvp.logger.write(CfgLogger::Level::INFO, "cfgNetCom updated.");
    }
    return success;
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
            mvp.logger.write(CfgLogger::Level::INFO, "Discovery response recieved.");
            break;
        case mvp.helper.hashStringDjb2("DISENS"): // DISENS Discovery request received, send discovery response
            udpSendMessage("RESENS", udp.remoteIP());
            mvp.logger.write(CfgLogger::Level::INFO, "Discovery request recieved.");
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
        mvp.logger.write(CfgLogger::Level::WARNING, "UDP not sent, send error.");                              // TODO: Check ESP32/8266
    for (uint16_t i = 0; i < strlen(message); i++) {
        udp.write((uint8_t)message[i]);
    }
    if (!udp.endPacket())
        mvp.logger.write(CfgLogger::Level::WARNING, "UDP not sent, send error.");   
}


void NetCom::mqttConnect() {
    mvp.logger.write(CfgLogger::Level::INFO, "sending to to broker.");
    if ((cfgNetCom.mqttForcedBroker.length() > 0))
        // Connect to forced broker
        mqttClient.connect(cfgNetCom.mqttForcedBroker.c_str(), cfgNetCom.mqttPort);
    else
        // Connect to discovered broker
        mqttClient.connect(mqttBrokerIp, cfgNetCom.mqttPort); 

    mvp.logger.write(CfgLogger::Level::INFO, "Connect request sent to broker.");
}

void NetCom::mqttWrite(const char *message) {
    // Need to be online and connected
    if (!mvp.net.connectedAsClient() || !mqttClient.connected()) 
        return;

    // Send message
    mqttClient.beginMessage(mqttTopicPrefix + cfgNetCom.mqttTopicSuffix);
    mqttClient.print(message);
    mqttClient.endMessage();
}
