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

void Led::setup(CfgLed _cfgLed) {
    cfgLed = _cfgLed;

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
    if (ledDelay.justFinished()) {
        toggle();
        ledDelay.repeat();
    }
}

void Led::checkChangeStatus() {
    Timing targetTiming;

    if (mvp.status == MVP3000::Status::ERROR) { // Blink fast for error
        targetTiming = Timing::FAST;
    } else {
        switch (mvp.net.status) { // Otherwise indicate Wifi status
            case Net::Status::AP:
                targetTiming = Timing::SLOW;
                break;
            case Net::Status::CLIENT:
                targetTiming = Timing::ON;
                break;
            case Net::Status::CONNECTING:
                targetTiming = Timing::MEDIUM;
                break;
            case Net::Status::NONE:
                targetTiming = Timing::FAST;
                break;
        }
    }

    // Nothing to do if not changed
    if (targetTiming == currentTiming)
        return;

    switch (targetTiming) {
        case Timing::OFF:
            off();
            ledDelay.stop();
            break;
        case Timing::ON:
            on();
            ledDelay.stop();
            break;
        default:
            // Best fit for transition, toggle LED if:
            //  target delay is shorter than old delay and has already passed
            //  target delay is longer than old delay and half has already passed
            if ( (ledDelay.getStartTime() > (uint16_t)targetTiming) || ( (ledDelay.getStartTime() > (uint16_t)targetTiming / 2) && ((uint16_t)targetTiming < (uint16_t)currentTiming) ) )
                toggle();

            // Restart with new delay
            ledDelay.stop();
            ledDelay.start((uint32_t)targetTiming);
            break;
    }

    // Remember current status
    currentTiming = targetTiming;

    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Led timing changed to: %d ms", targetTiming);
}

void Led::on() {
    digitalWrite(cfgLed.pin, LOW);
    this->state = true;
}

void Led::off() {
    digitalWrite(cfgLed.pin, HIGH);
    this->state = false;
}

void Led::toggle() {
    if (this->state)
        off();
    else
        on();
}
