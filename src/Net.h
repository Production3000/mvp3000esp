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

#ifndef MVP3000_NET
#define MVP3000_NET

#include <Arduino.h>
#ifdef ESP8266
    #include <ESP8266WiFi.h>
#else
    #include <WiFi.h>
#endif
#include <DNSServer.h> // for captive portal

#include "Config.h"
#include "NetCom.h"
#include "NetWeb.h"
#include "ESPX.h"
#ifdef ESP8266
    extern EspClass ESPX;
#else
    extern EspClassX ESPX;
#endif


struct CfgNet : public CfgJsonInterface  {

    // Modifiable settings saved to SPIFF

    String clientSsid = ""; // IEEE 1-32 chars
    String clientPass = ""; // IEEE 8-63 chars
    uint16_t clientConnectRetries = 3;

    // DANGER, forceClientMode could make the device inaccessible for the user without flashing it
    // Usefull when network settings are correct for sure but likely offline during power on
    //  false on power on: try network settings clientConnectRetries times
    //      on success set clientConnectSuccess to true
    //      on fail fallback to AP
    //  false with clientConnectSuccess true on connection lost: try endlessly to previously verified network settings
    //  true on power on: try network settings endlessly
    boolean forceClientMode = false;

    CfgNet() : CfgJsonInterface("cfgNet") {
        addSetting<uint16_t>("clientConnectRetries", &clientConnectRetries, [](uint16_t x) { return (x > 100) ? false : true; }); // Limit to 100, any more is 'forever'
        addSetting<String>("clientSsid", &clientSsid, [](String x) { return true; } ); // Check is in extra function
        addSetting<String>("clientPass", &clientPass, [](String x) { return true; }); // Check is in extra function
        addSetting<boolean>("forceClientMode", &forceClientMode, [](boolean _) { return true; });
    }
};


class Net {
    private:
        DNSServer dnsServer;

        // Counter for client connect fails
        uint8_t clientConnectFails = 0;
        boolean clientConnectSuccess = false;
    
        void connectClient();
        void WiFiGotIP();
        void WiFiStationDisconnected();

        void startWifi();
        void startAp();
        void startClient();

        uint32_t delayedRestartWifi_ms = 0;
        void delayedRestartWifi(uint32_t delay_ms = 50) { delayedRestartWifi_ms = millis() + delay_ms; };

// ESP8266 needs definition of Wifi events
#ifdef ESP8266
        WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
#endif

    public:
        enum class NET_STATE_TYPE: uint8_t {
            CLIENT = 0,
            CONNECTING = 1,
            AP = 2,
            DISABLEDX = 3,
            ERROR = 4
        };
        NET_STATE_TYPE netState = NET_STATE_TYPE::DISABLEDX;

        CfgNet cfgNet;

        NetWeb netWeb;
        NetCom netCom;

        String apSsid = "device" + String(ESPX.getChipId());

        void setup();
        void loop();

        bool editClientConnection(String newSsid, String newPass);
        void cleanCfgKeepClientInfo();

    private:

        String webPageProcessor(const String& var);
        char* webPage = R"===(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %0%</title>
    <script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
    <style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h2>MVP3000 - Device ID %0%</h2>
    <p><a href='/'>Home</a></p>
    <h3>Network</h3> <ul>
        <li>Fallback AP SSID: '%31%'</li>
        <li>Network credentials: leave SSID empty to remove, any changes are applied at once.<br> <form action='/start' method='post' onsubmit='return confirm(`Change network?`);'> <input name='setwifi' type='hidden'> SSID <input name='newSsid' value='%32%'> Passphrase <input type='password' name='newPass' value='%33%'> <input type='submit' value='Save'> </form> </li>
        <li>Reconnect tries: <br> <form action='/save' method='post'> <input name='clientConnectRetries' type='number' value='%34%' min='1' max='255'> <input type='submit' value='Save'> </form> </li>
        <li>Force client mode. WARNING: If the credentials are wrong, the device will be inaccessible via network, thus require re-flashing!
         <form action='/checksave' method='post' onsubmit='return promptId(this);'> <input name='forceClientMode' type='checkbox' %35% value='1'> <input name='forceClientMode' type='hidden' value='0'> <input name='deviceId' type='hidden'> <input type='submit' value='Save'> </form> </li> </ul>
<p>&nbsp;</body></html>
)===";
};

#endif