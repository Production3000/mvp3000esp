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


LimitTimer timer(50);

void setup() {
    // Add the custom module to the mvp framework
    mvp.addXmodule(&xmoduleLED);

    // Start mvp framework
    mvp.setup();

    xmoduleLED.setOnce(std::bind(&arraySetter, std::placeholders::_1, std::placeholders::_2));
    xmoduleLED.setOnDemandSetter(std::bind(&singleSetter, std::placeholders::_1));

    // Delay to see the initial LED colors
    delay(5000); 
}

void loop() {
    // Change LED to based on status/data/events/...
    if (timer.justFinished())
        xmoduleLED.demandLedUpdate();

    mvp.loop();
}

// Callbacks

void arraySetter(uint32_t* ledArray, uint8_t ledCount) {
    for (uint8_t i = 0; i < ledCount; i++) {
        ledArray[i] = Adafruit_NeoPixel::Color(127, i * (255/ledCount - 1), 255 - (i * (255/ledCount - 1)));
    }
}

uint32_t singleSetter(uint8_t led) {
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
