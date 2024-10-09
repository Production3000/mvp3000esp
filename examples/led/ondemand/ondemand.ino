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

#include <MVP3000.h>
extern MVP3000 mvp;

#include "XmoduleLED.h"
#define LED_PIN D8
// #define LED_PIN D6 // 'upper' J4 on the PCB
// #define LED_PIN D5 // 'lower' J5 on the PCB
uint8_t ledCount = 12;

XmoduleLED xmoduleLED(LED_PIN, ledCount);

LimitTimer timer(100);

void setup() {
    // Add the custom module to the mvp framework
    mvp.addXmodule(&xmoduleLED);
    // All pixels in sync alternating color depending on uptime.
    xmoduleLED.setSyncColor(Adafruit_NeoPixel::Color(255, 0, 0));

    // Start mvp framework
    mvp.setup();
}


void loop() {
    // Change LED to based on status/data/events/...
    if (timer.justFinished()){
        xmoduleLED.setSyncColor(evaluateData());
    }

    mvp.loop();
}


uint32_t evaluateData() {
    switch ((millis() / 1000) % 3) {
        case 0:
            return Adafruit_NeoPixel::Color(255, 0, 0);
        case 1:
            return Adafruit_NeoPixel::Color(0, 255, 0);
        case 2:
            return Adafruit_NeoPixel::Color(0, 0, 255);
        default:
            return 0;
    }
}
