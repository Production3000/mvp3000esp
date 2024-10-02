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


void setup() {
    // Add the custom module to the mvp framework
    mvp.addXmodule(&xmoduleLED);

    // Start mvp framework
    mvp.setup();

    // Internal effect
    // xmoduleLED.setEffect(1);
    // xmoduleLED.setEffect(2);

    // All pixels in sync, called every 1000ms
    xmoduleLED.setEffectSetter(fxSyncSetter, 1000, true);

    // Each pixel with individual color, called with 40Hz, repeating/reset after 2000ms 
    // xmoduleLED.setEffectSetter(fxSingleSetter, 2000);
}

void loop() {
    mvp.loop();
}

uint32_t fxSyncSetter(uint16_t timingPosition) {
    return Adafruit_NeoPixel::Color(random(255), random(255), random(255));
}

uint32_t fxSingleSetter(uint8_t led, uint16_t timingPosition) {
    uint8_t shift = timingPosition / 256;
    return Adafruit_NeoPixel::Color(shift, 255 - shift, 255 * led / (ledCount - 1) );
}