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

#include "Led.h"

#include "MVP3000.h"
extern MVP3000 mvp;

void Led::setup() {

    if (!cfgLed.enabled) {
        mvp.logger.write(CfgLogger::Level::INFO, "Led disabled.");
        return;
    }

    pinMode(cfgLed.pin, OUTPUT);
    checkChangeStatus();
}

void Led::loop() {
    if (!cfgLed.enabled)
        return;

    // Check if system status changed and update timing if needed
    checkChangeStatus();

    // Delay finished
    if (ledTimer.justFinished()) {
        toggle();
    }
}

void Led::checkChangeStatus() {
    LED_TIMING_TYPE targetTiming;

    if (mvp.state == MVP3000::STATE_TYPE::ERROR) { // Blink fast for error
        targetTiming = LED_TIMING_TYPE::FAST;
    } else {
        switch (mvp.net.netState) { // Otherwise indicate Wifi status
            case Net::NET_STATE_TYPE::AP:
                targetTiming = LED_TIMING_TYPE::SLOW;
                break;
            case Net::NET_STATE_TYPE::CLIENT:
                targetTiming = LED_TIMING_TYPE::ON;
                break;
            case Net::NET_STATE_TYPE::CONNECTING:
                targetTiming = LED_TIMING_TYPE::MEDIUM;
                break;
            case Net::NET_STATE_TYPE::ERROR:
                targetTiming = LED_TIMING_TYPE::FAST;
                break;
        }
    }

    // Nothing to do if not changed
    if (targetTiming == ledTiming)
        return;

    switch (targetTiming) {
        case LED_TIMING_TYPE::OFF:
            off();
            ledTimer.stop();
            break;
        case LED_TIMING_TYPE::ON:
            on();
            ledTimer.stop();
            break;
        default:
            // Restart with new delay. The timing during this transition is of course weird.
            ledTimer.restart((uint32_t)targetTiming);
            break;
    }

    // Remember current status
    ledTiming = targetTiming;

    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Led timing changed to: %d ms", targetTiming);
}

void Led::on() {
    digitalWrite(cfgLed.pin, ONSTATE);
    currentState = true;
}

void Led::off() {
    digitalWrite(cfgLed.pin, !ONSTATE);
    currentState = false;
}

void Led::toggle() {
    (currentState) ? off() : on();
}
