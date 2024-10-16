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

#include "Logger.h"
#include "Led.h"
#include "Config.h"
#include "Net.h"

#include "_Xmodule.h"
#include "XmoduleSensor.h"


#include <stdarg.h>


class MVP3000 {

    public:

        /**
         * @brief Add a Xmodule to the MVP3000 system.
         *
         * @param XmoduleSensor Pointer to the module to add.
         */
        void addXmodule(_Xmodule *xmodule);


        /**
         * @brief Log a message at 'user' level.
         *
         * @param message The message to log.
         */
        void log(const String& message) { logger.write(CfgLogger::Level::USER, message); };

        /**
         * @brief Log a formatted message at 'user' level.
         *
         * @param message The message to log with arguments following.
         */
        void logFormatted(const String& message, ...) {
            va_list args;
            va_start(args, message);
            // NOTE: va_end(args) is called in printFormatted to allow direct return
            logger.writeFormatted(CfgLogger::Level::USER, message, args);
        };

        /**
         * @brief Enable or disable ANSI encoding of serial output.
         *
         * @param enable True to enable, false to disable.
         */
        void logDisableAnsiColor() { logger.disableAnsiColor(); };

        /** 
         * @brief Set the log level to 'info', 'data', 'control', 'user', 'warning' or 'error'.
         */
        void logSetLevel(CfgLogger::Level level) { logger.setLevel(level); };

        /**
         * @brief Enable/disable the output targets of logging message. Console and web interface is enabled by default, websocket is disabled.
         *
         * @param target The target to change.
         * @param enable True to enable, false to disable.
         */
        void logSetTarget(CfgLogger::OutputTarget target, boolean enable) { logger.setTarget(target, enable); };

        
        /**
         * @brief Completely disable the MQTT discovery service.
         */
        void mqttHardDisable() { net.netMqtt.hardDisable(); };

        /**
         * @brief Completely disable the UDP discovery service.
         */
        void udpHardDisable() { net.netCom.hardDisable(); };

        /**
         * @brief Completely disable the UDP discovery service.
         */
        void wsHardDisable() { net.netWeb.webSockets.hardDisable(); };

        /**
         * @brief Set an alternate page as root and move the main MVP3000 page to a sub-uri.
         * 
         * @param alternateResponseFiller The function to fill the alternate page.
         * @param alternateTemplateProcessor (optional) The function to process the alternate page template.
         * @param mvpUri (optional) The new URI of the main MVP3000 page. Default is "/mvp3000".
         */
        void setAlternateRoot(AwsResponseFiller alternateResponseFiller, std::function<String (uint16_t)> alternateTemplateProcessor = nullptr, const String& mvpUri = "/mvp3000") { net.netWeb.setAlternateRoot(alternateResponseFiller, alternateTemplateProcessor, mvpUri); }

    public:

        enum class STATE_TYPE: uint8_t {
            GOOD = 0, // Everything is fine
            INIT = 1, // Standard state when booting up until wifi up
            ERROR = 2 // Critical error
        };
        STATE_TYPE state = STATE_TYPE::INIT;

        Config config;
        // Helper helper;
        Led led;
        Logger logger;
        Net net;

        void setup();
        void loop();

        void delayedRestart(uint32_t delay_ms = 25) { delayedRestart_ms = millis() + delay_ms; };


        // Modules
        static const uint8_t MAX_MODULES = 5;  // Maximum number of modules allowed
        uint8_t moduleCount = 0;
        _Xmodule* xmodules[MAX_MODULES];

    private:

        void checkStatus();

        uint32_t delayedRestart_ms = 0;

        uint32_t loopLast_ms = 0;
        uint16_t loopDurationMean_ms = 0;
        uint16_t loopDurationMax_ms = std::numeric_limits<uint16_t>::min();
        uint16_t loopDurationMin_ms = std::numeric_limits<uint16_t>::max();
        void updateLoopDuration();

    public:

        String templateProcessor(uint16_t var);
        uint8_t webPageProcessorCount;

};

#endif
