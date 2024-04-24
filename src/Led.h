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
#include <millisDelay.h> // https://github.com/PowerBroker2/SafeString

// Replace millisDelay, there are no hardware timers on ESP8266
// Replace with own in helper                               // TODO


struct CfgLed {             
    byte pin = BUILTIN_LED;
    boolean enabled = true;
};


class Led {
    private:
        boolean state = false;

// ESP32/ESP8266 have inverted high/low for on/off
#ifdef ESP8266
        uint8_t ONSTATE = LOW;
        uint8_t OFFSTATE = HIGH;
#else
        uint8_t ONSTATE = HIGH;
        uint8_t OFFSTATE = LOW;
#endif

        CfgLed cfgLed;

        enum class Timing: int16_t {
            OFF = -1, // Power on: NULL --> off
            ON = 0, // Wifi connected and no other system errors
            FAST = 100, // Critical system error
            MEDIUM = 500, // Connecting
            SLOW = 2000, // Init/AP state after RESET button press
        };

        Timing currentTiming = Timing::OFF;

        millisDelay ledDelay;

        void checkChangeStatus();

        void on();
        void off();
        void toggle();

    public:
        void setup(CfgLed _cfgLed);
        void loop();
};

#endif