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

    // Redefine needed with network, otherwise mqttClient.connected() crashes
    mqttClient = MqttClient(wifiClient);

    // Start UDP even if external forcedBroker is set, to allow reverse-discovery of this ESP device
    udp.begin(cfgNetCom.discoveryPort);


    // Define web page
    webPageNetCom = new NetWeb::WebPage("/netcom", R"===(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %0%</title>
    <script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
    <style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h1>MVP3000 - Device ID %0%</h1>
    <p><a href='/'>Home</a></p>
    <h3>MQTT Communication</h3> <ul>
        <li>Status: %51% </li>
        <li>Auto-discovery port local broker: 1024-65535, default is 4211.<br> <form action='/save' method='post'> <input name='discoveryPort' value='%52%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li>
        <li>Forced external broker:<br> <form action='/save' method='post'> <input name='mqttForcedBroker' value='%53%'> <input type='submit' value='Save'> </form> </li>
        <li>MQTT port: default is 1883 (unsecure) <br> <form action='/save' method='post'> <input name='mqttPort' value='%54%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li>
        <li>Topic: <br> <form action='/save' method='post'> %55% <input name='mqttTopicSuffix' value='%56%' minlength='5'> <input type='submit' value='Save'> </form> </li> </ul>
<p>&nbsp;</body></html>
    )===" ,[&](const String& var) -> String {
        if (!mvp.helper.isValidInteger(var)) {
            mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid placeholder in template: %s", var.c_str());
            return var;
        }

        switch (var.toInt()) {
            case 0:
                return String(ESPX.getChipId());

            case 51:
                return controllerConnectedString().c_str();
            case 52:
                return String(cfgNetCom.discoveryPort);
            case 53:
                return cfgNetCom.mqttForcedBroker.c_str();
            case 54:
                return String(cfgNetCom.mqttPort);
            case 55:
                return mqttTopicPrefix.c_str();
            case 56:
                return cfgNetCom.mqttTopicSuffix.c_str();

            default:
                break;
        }
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Invalid placeholder in template: %s", var.c_str());
        return var;
    });
    // Register web page
    mvp.net.netWeb.registerPage(*webPageNetCom);

    // Register config
    mvp.net.netWeb.registerCfg(&cfgNetCom);
};

void NetCom::loop() {
    // Called from net.loop() only if wifi is up and in client mode

    switch (comState) {
        case COM_STATE_TYPE::CONNECTED:
            // Check if state is still connected
            if (mqttClient.connected()) {
                // Check for UDP packet, in that case handle it
                if (udp.parsePacket())
                    udpReceiveMessage();
            } else {
                comState = COM_STATE_TYPE::WAITING;
                mvp.logger.write(CfgLogger::Level::WARNING, "Disconnected from MQTT broker.");
            }
            break;

        case COM_STATE_TYPE::WAITING:
            if (!mqttClient.connected()) {
                // Only work to do if interval not yet started or just finished
                if (!brokerDelay.isRunning() || brokerDelay.justFinished()) {
                    // Connect to forced broker or discover broker on local network
                    if ((mqttBrokerIp != INADDR_NONE) || (cfgNetCom.mqttForcedBroker.length() > 0)) {
                        // Connect to broker
                        mqttConnect();
                    } else {
                        // Auto-discover local broker IP only if no forced broker
                        udpDiscoverMqtt();
                    }
                    // (Re)start interval
                    brokerDelay.start(brokerInterval);
                }
            } else {
                comState = COM_STATE_TYPE::CONNECTED;
                brokerDelay.stop();
                mvp.logger.write(CfgLogger::Level::INFO, "Connected to MQTT broker.");
            }
            break;
        
        case COM_STATE_TYPE::DISABLEDX: // Nothing to do
            break;

        default:
            break;
    }
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


void NetCom::mqttConnect() {
    if ((cfgNetCom.mqttForcedBroker.length() > 0))
        // Connect to forced broker
        mqttClient.connect(cfgNetCom.mqttForcedBroker.c_str(), cfgNetCom.mqttPort);
    else
        // Connect to discovered broker
        // The library is broken for ESP8266, it does not accept the IPAddress-type when a port is given
        mqttClient.connect(mqttBrokerIp.toString().c_str(), cfgNetCom.mqttPort);

    mvp.logger.write(CfgLogger::Level::INFO, "Connect request sent to broker.");
}

void NetCom::mqttWrite(const char *message) {
    // Check if MQTT is connected
    if (comState != COM_STATE_TYPE::CONNECTED)
        return;

    // Send message
    mqttClient.beginMessage(mqttTopicPrefix + cfgNetCom.mqttTopicSuffix);
    mqttClient.print(message);
    mqttClient.endMessage();
}
