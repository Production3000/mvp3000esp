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

#ifndef MVP3000_mvp
#define MVP3000_mvp

#include <Arduino.h>

#include "_Helper.h"

#include "Logger.h"
#include "Led.h"
#include "Config.h"
#include "Net.h"

#include "Xmodule.h"
#include "XmoduleSensor.h"


class MVP3000 {

    public:

        /**
         * @brief Add a Xmodule to the MVP3000 system.
         * 
         * @param XmoduleSensor Pointer to the module to add.
         */
        void addXmodule(Xmodule *XmoduleSensor);

        /**
         * @brief Log a message at 'user' level.
         * 
         * @param message The message to log.
         */
        void log(const char *message) { logger.write(CfgLogger::Level::USER, message); };

        /**
         * @brief Enable or disable ANSI encoding of serial output.
         * 
         * @param enable True to enable, false to disable.
         */
        void logAnsiColor(boolean enable) { logger.ansiColor(enable); };

        /**
         * @brief Completely disable the UDP discovery service.
         */
        void udpHardDisable() { net.netCom.hardDisable(); };

    public:

        enum class STATE_TYPE: uint8_t {
            GOOD = 0, // Everything is fine
            INIT = 1, // Standard state when booting up until wifi up
            ERROR = 2 // Critical error
        };
        STATE_TYPE state = STATE_TYPE::INIT;

        Config config;
        Helper helper;
        Led led;
        Logger logger;
        Net net;

        void setup();
        void loop();

        void delayedRestart(uint32_t delay_ms = 25) { delayedRestart_ms = millis() + delay_ms; };

    private:

        // Modules
        static const uint8_t MAX_MODULES = 5;  // Maximum number of modules allowed
        uint8_t moduleCount = 0;
        Xmodule *xmodules[MAX_MODULES];

        void checkStatus();

        uint32_t delayedRestart_ms = 0;

        uint32_t loopLast_ms = 0;
        uint16_t loopDurationMean_ms = 0;
        uint16_t loopDurationMax_ms = std::numeric_limits<uint16_t>::min();
        uint16_t loopDurationMin_ms = std::numeric_limits<uint16_t>::max();
        void updateLoopDuration();


        String webPageProcessor(uint8_t var);
        char* webPage = R"===(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %1%</title>
    <script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
    <style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h2>MVP3000 - Device ID %1%</h2> <h3 style='color: red;'>%0%</h3>
    <h3>System</h3> <ul>
        <li>ID: %1%</li>
        <li>Build: %11%</li>
        <li>Memory: %12%&percnt; fragmented</li>
        <li>Uptime: %13%</li>
        <li>Last restart reason: %14%</li>
        <li>CPU frequency: %15% MHz</li>
        <li>Main loop duration: %16% ms (mean/min/max)</li>
        <li>Log websocket: ws://%2%/wslog </br>
            <textarea readonly rows='3' cols='80'>%17%</textarea> </li> </ul>
    <h3>Components</h3> <ul>
        <li><a href='/net'>Network</a></li>
        <li> %18% </li>
        <li><a href='/netmqtt'>MQTT</a></li> </ul>
    <h3>Modules</h3> <ul>
        %20% </ul>
    <h3>Maintenance</h3> <ul>
        <li> <form action='/start' method='post' onsubmit='return confirm(`Restart?`);'> <input name='restart' type='hidden'> <input type='submit' value='Restart' > </form> </li>
        <li> <form action='/checkstart' method='post' onsubmit='return promptId(this);'> <input name='reset' type='hidden'> <input name='deviceId' type='hidden'> <input type='submit' value='Factory reset'> <input type='checkbox' name='keepwifi' checked value='1'> keep Wifi </form> </li> </ul>
<p>&nbsp;</body></html>
)===";

};

#endif
