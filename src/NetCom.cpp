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

#include "_Helper.h"
extern _Helper _helper;


void NetCom::setup() {
    // This can be completely to allow external UDP uses, in a Xmodule or other. Saves minimal 200 kB memory.
    if (cfgNetCom.isHardDisabled) {
        udpState = UDP_STATE::HARDDISABLED;
        webPage = webPageHardDisabled; // Set web to display disabled html
        return;
    }
    udpState = UDP_STATE::ENABLED;

    // Read config and register
    mvp.config.readCfg(cfgNetCom);
    mvp.net.netWeb.registerCfg(&cfgNetCom, std::bind(&NetCom::saveCfgCallback, this));

    // Start UDP for discovery and reverse-discovery of this ESP device
    udp.begin(cfgNetCom.discoveryPort);
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Discovery started on port: %d", cfgNetCom.discoveryPort);
};

void NetCom::loop() {
    // Nothing to do if not ENABLED or not connected
    if ((udpState == UDP_STATE::HARDDISABLED) || !mvp.net.connectedAsClient())
        return;

    // Check for UDP packet, in that case handle it
    if (udp.parsePacket())
        udpReceiveMessage();

    // Discover server and regularly update
    sendDiscovery();
}


///////////////////////////////////////////////////////////////////////////////////

IPAddress NetCom::checkSkill(const String& requestedSkill) {
    // Check if the requested skill is in the string of skills
    if (serverSkills.indexOf(requestedSkill) >= 0)
        return serverIp;
    return INADDR_NONE;
}

void NetCom::sendDiscovery() {
    // Do not hammer the network, everything should be discovered on the first try anyway and there will not be much change afterwards
    if (!discoveryTimer.justFinished())
        return;

    // Clear previous discovery if it is to old
    if (millis() > lastDiscovery + 1.2 * discoveryInterval) {
        serverIp = INADDR_NONE;
        serverSkills = "";
    }

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

    // Do nothing if a DEVICE message is received, generally we could collect those other devices on the network
    // if (strncmp(packetBuffer, "DEVICE", 6) == 0) { return; }
    // Check for MVP3000, respond with DEVICE[ID]
    if (strncmp(packetBuffer, "MVP3000", 7) == 0) {
        udpSendMessage((String("DEVICE") + String(_helper.ESPX->getChipId())).c_str() , udp.remoteIP());
        mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Discovery response sent to: %s", udp.remoteIP().toString().c_str());
        return;
    }
    // Check for SERVER, store the IP and the SKILL string
    if (strncmp(packetBuffer, "SERVER", 6) == 0) {
        serverIp = udp.remoteIP();
        serverSkills = packetBuffer + 7;
        lastDiscovery = millis();
        mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Server response: %s from %s", serverSkills.c_str(), serverIp.toString().c_str());
        return;
    }
}

void NetCom::udpSendMessage(const char* message, IPAddress remoteIp) {
    // Test this using netcat: nc -ukl [port]

    if (udpState == UDP_STATE::HARDDISABLED)
        return;
        
    // Send UDP packet
    if (!udp.beginPacket(remoteIp, cfgNetCom.discoveryPort)) {
        mvp.logger.write(CfgLogger::Level::WARNING, "UDP not sent, send error.");
        return;
    }
    for (uint16_t i = 0; i < strlen(message); i++) {
        udp.write((uint8_t)message[i]);
    }
    if (!udp.endPacket()) {
        mvp.logger.write(CfgLogger::Level::WARNING, "UDP not completed, send error.");
    }
}


///////////////////////////////////////////////////////////////////////////////////

void NetCom::saveCfgCallback() {
    // HARDDISABLED: nothing to do (actually this request comes from an outdated HTML form)
    if (udpState == UDP_STATE::HARDDISABLED)
        return;

    // Stop and restart
    udp.stop();
    udp.begin(cfgNetCom.discoveryPort);
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Discovery started on port: %d", cfgNetCom.discoveryPort);
}


String NetCom::templateProcessor(uint8_t var) {
    switch (var) {
        case 52:
            return String(cfgNetCom.discoveryPort);
        case 53:
            return (serverIp == INADDR_NONE) ? "none" : _helper.printFormatted("%s: %s", serverIp.toString().c_str(), serverSkills.c_str());

        default:
            return "";
    }
}