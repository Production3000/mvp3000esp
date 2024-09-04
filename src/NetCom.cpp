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

Sends out MVP3000 to broadcast
Devices respond with DEVICE[ID]
Server responds with SERVER;SKILL;SKILL;SKILL

*/

///////////////////////////////////////////////////////////////////////////////////


#include "NetCom.h"

#include "MVP3000.h"
extern MVP3000 mvp;


void NetCom::setup() {
    // Read config and register with web interface
    mvp.config.readCfg(cfgNetCom);

    // This can be complately disabled to allow external UDP uses, in a Xmodule or other.
    if (!cfgNetCom.udpEnabled)
        return;

    // Start UDP even if external forcedBroker is set, to allow reverse-discovery of this ESP device
    udp.begin(cfgNetCom.discoveryPort);

    // Define web page
    mvp.net.netWeb.registerPage("/netcom", webPage ,  std::bind(&NetCom::webPageProcessor, this, std::placeholders::_1)); 
    // Register config
    mvp.net.netWeb.registerCfg(&cfgNetCom);

};

void NetCom::loop() {
    // Called from net.loop() only if wifi is up and in client mode

    if (!cfgNetCom.udpEnabled)
        return;

    // Check for UDP packet, in that case handle it
    if (udp.parsePacket())
        udpReceiveMessage();                                                                    // TODO there is never a discovery sent, so no skills are ever received
}


///////////////////////////////////////////////////////////////////////////////////

IPAddress NetCom::checkSkill(String requestedSkill) {
    if (serverIp == INADDR_NONE)
        return INADDR_NONE;

    // Check if the requested skill is in the string of skills
    if (serverSkills.indexOf(requestedSkill) == -1)
        return INADDR_NONE;

    return serverIp;
}

void NetCom::sendDiscovery() {
    udpSendMessage("MVP3000", WiFi.broadcastIP()); 
    mvp.logger.write(CfgLogger::Level::INFO, "Discovery request sent.");
}

void NetCom::udpReceiveMessage() {
    char packetBuffer[256];
    uint8_t len = udp.read(packetBuffer, 256);

    // Length is 7 or more caracters
    if (len < 7)
        return;

    // Terminate char string
    packetBuffer[len] = '\0';

    // Do nothing if a DEVICE message is received.
    // Check for MVP3000, respond with DEVICE[ID]
    if (strncmp(packetBuffer, "MVP3000", 7) == 0) {
        udpSendMessage((String("DEVICE") + String(ESPX.getChipId())).c_str() , udp.remoteIP());
        mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Discovery response sent to: %s", udp.remoteIP().toString().c_str());
        return;
    }
    // Check for SERVER, store the IP and the SKILL string
    if (strncmp(packetBuffer, "SERVER", 6) == 0) {
        serverIp = udp.remoteIP();
        serverSkills = packetBuffer + 7;
        mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Server response: %s from %s", serverSkills.c_str(), serverIp.toString().c_str());
        return;
    }
}

void NetCom::udpSendMessage(const char *message, IPAddress remoteIp) {
    // Test this using netcat: nc -ul [laptopIP] [port]

    // Standard call is without remoteIp and defaults to controller IP
    // if (remoteIp == INADDR_NONE) {
    //     // Check if controller is available
    //     if (serverIp == INADDR_NONE) {
    //         mvp.logger.write(CfgLogger::Level::WARNING, "UDP not sent, no remote/controller IP.");
    //         return;
    //     }
    //     remoteIp = serverIp;
    // }

    // Return for empty message/datastring
    // if (strlen(message) == 0) {
    //     mvp.logger.write(CfgLogger::Level::INFO, "UDP not sent, message empty.");
    //     return;
    // }

    // Send UDP packet
    if (!udp.beginPacket(remoteIp, cfgNetCom.discoveryPort))
        mvp.logger.write(CfgLogger::Level::WARNING, "UDP not sent, send error.");
    for (uint16_t i = 0; i < strlen(message); i++) {
        udp.write((uint8_t)message[i]);
    }
    if (!udp.endPacket())
        mvp.logger.write(CfgLogger::Level::WARNING, "UDP not sent, send error.");   
}


///////////////////////////////////////////////////////////////////////////////////

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