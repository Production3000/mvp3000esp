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
#include "ESPX.h"
#ifdef ESP8266
    extern EspClass ESPX;
#else
    extern EspClassX ESPX;
#endif

#include "NetMqtt_TopicList.h"
#include "_HelNEWper.h"


struct CfgNetMqtt : public CfgJsonInterface {

    // Modifiable settings saved to SPIFF

    boolean mqttEnabled = true;
    uint16_t mqttPort = 1883; // 1883: unencrypted, unauthenticated
    String mqttForcedBroker = ""; // test.mosquitto.org

    CfgNetMqtt() : CfgJsonInterface("cfgNetMqtt") {
        addSetting<boolean>("mqttEnabled", &mqttEnabled, [](boolean _) { return true; });
        addSetting<uint16_t>("mqttPort", &mqttPort, [](uint16_t x) { return (x < 1024) ? false : true; }); // port above 1024
        addSetting<String>("mqttForcedBroker", &mqttForcedBroker, [](String x) { return ((x.length() > 0) && (x.length() < 6)) ? false : true; } ); // allow empty to remove
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
         * @param dataCallback The function to call when data is received on the topic suffixed with _ctrl. Omit to not subscribe to the topic.
         * @return Returns the function to write data to MQTT.
         */
        std::function<void(const String &message)> registerMqtt(String topic, MqttDataCallback dataCallback = nullptr);

    private:

        enum class MQTT_STATE: uint8_t {
            CONNECTED = 0,
            CONNECTING = 1,
            DISCONNECTED = 2,
            NOBROKER = 3,
            DISABLEDX = 4
        };
        MQTT_STATE mqttState;

        MqttTopicList mqttTopicList = MqttTopicList(&mqttClient);

        CfgNetMqtt cfgNetMqtt;

        WiFiClient wifiClient;

        MqttClient mqttClient = NULL;

        IPAddress localBrokerIp = INADDR_NONE; // compare with == operator, there is

        uint16_t connectInterval = 5000;
        uint8_t connectTries = 3;
        LimitTimer connectTimer = LimitTimer(connectInterval, connectTries);

        void saveCfgCallback();
        void setMqttState();

        void connectMqtt();

        void handleMessage(int messageSize);


        String webPageProcessor(uint8_t var);
        char const* webPage = R"===(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %1%</title>
<script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
<style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h2>MVP3000 - Device ID %1%</h2> <h3 style='color: red;'>%0%</h3>
<p><a href='/'>Home</a></p>
<h3>MQTT Communication</h3> <ul>
    <li>Enable: <form action='/save' method='post'> <input name='mqttEnabled' type='checkbox' %51% value='1'> <input name='mqttEnabled' type='hidden' value='0'> <input type='submit' value='Save'> </form> </li>
    <li>Status: %52% </li>
    <li>Local broker: %53% </li>
    <li>Forced external broker:<br> <form action='/save' method='post'> <input name='mqttForcedBroker' value='%54%'> <input type='submit' value='Save'> </form> </li>
    <li>MQTT port: default is 1883 (unsecure) <br> <form action='/save' method='post'> <input name='mqttPort' value='%55%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li>
    <li>Topics: <ul> %60% </ul> </li> </ul>
<p>&nbsp;</body></html>
)===";

};

#endif