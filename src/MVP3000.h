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

    private:
        void checkStatus();

        uint32_t loopLast_ms = 0;
        void updateLoopDuration();

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

        // Modules
        static const uint8_t MAX_MODULES = 5;  // Maximum number of modules allowed
        uint8_t moduleCount = 0;
        Xmodule *xmodules[MAX_MODULES];

        void addXmodule(Xmodule *XmoduleSensor);

        // Loop duration statistics
        uint16_t loopDurationMean_ms = 0;
        uint16_t loopDurationMax_ms = 0;
        uint16_t loopDurationMin_ms = std::numeric_limits<uint16_t>::max();

        // Forward internal functions for simplicity
        void log(const char *message) { logger.write(CfgLogger::Level::USER, message); };

        uint32_t delayedRestart_ms = 0;
        void delayedRestart(uint32_t delay_ms = 25) { delayedRestart_ms = millis() + delay_ms; };
};

#endif
