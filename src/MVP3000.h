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

#include "Helper.h"

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

        Config config;
        Helper helper;
        Led led;
        Logger logger;
        Net net;

        // Modules
        static const uint8_t MAX_MODULES = 5;  // Maximum number of modules allowed
        uint8_t moduleCount = 0;
        Xmodule *xmodules[MAX_MODULES];

        enum class Status: uint8_t {
            GOOD = 0, // Everything is fine
            INIT = 1, // Standard state when booting up until wifi up
            ERROR = 2 // Critical error
        };
        Status status = Status::INIT;

        uint16_t loopDurationMean_ms = 0;
        uint16_t loopDurationMax_ms = 0;
        uint16_t loopDurationMin_ms = std::numeric_limits<uint16_t>::max();

        void setup();
        void loop();

        void addXmodule(Xmodule *XmoduleSensor);

        // Forward internal functions for simplicity
        void log(const char *message) { logger.write(CfgLogger::Level::USER, message); };

};

#endif
