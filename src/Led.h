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

// Turns LED on/off or blinks depending on system status:
//  - blink fast for error
//  - otherwise indicate Wifi status
// Defaults to the built-in LED, which is connected to D4/IO-02. If this pin is
// used elsewhere, disable the LED output to not mess things up.

#ifndef MVP3000_LED
#define MVP3000_LED

#include <Arduino.h>

#include "_Helper_LimitTimer.h"


struct CfgLed {
    byte pin = BUILTIN_LED;
    boolean enabled = true;
};


class Led {

    public:
        void setup();
        void loop();

    private:
        enum class LED_TIMING_TYPE: int16_t {
            OFF = -1, // Power on: NULL --> off
            ON = 0, // Wifi connected and no other system errors
            FAST = 100, // Critical system error
            MEDIUM = 500, // Connecting
            SLOW = 2000, // Init/AP state after RESET button press
        };
        LED_TIMING_TYPE ledTiming = LED_TIMING_TYPE::OFF;

// ESP32/ESP8266 have inverted high/low for on/off
#ifdef ESP32
        uint8_t ONSTATE = HIGH;
#else
        uint8_t ONSTATE = LOW;
#endif

        boolean currentState = false;

        CfgLed cfgLed;

        LimitTimer ledTimer = LimitTimer((uint8_t)LED_TIMING_TYPE::OFF);

        void checkChangeStatus();

        void on();
        void off();
        void toggle();
};

#endif