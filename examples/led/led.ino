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

// LED PIN
#define LED_PIN D8
// #define LED_PIN D6 // 'upper' J4 on the PCB
// #define LED_PIN D5 // 'lower' J5 on the PCB

// Declare our NeoPixel strip

uint8_t ledCount = 12;

#include "XmoduleLED.h"

XmoduleLED xmoduleLED(LED_PIN, ledCount);

LimitTimer timer(50);

void setup() {
    // Add the custom module to the mvp framework
    mvp.addXmodule(&xmoduleLED);

    // Start mvp framework
    mvp.setup();

    // xmoduleLED.setEffect(1);
    // xmoduleLED.setOnDemandSetter(std::bind(&onDemandSetter, std::placeholders::_1));
    xmoduleLED.setEffectCallback(std::bind(&effectCallback, std::placeholders::_1, std::placeholders::_2));
}

void loop() {
    // For the onDemand option, eg data changes the LED color
    // if (timer.justFinished())
    //     xmoduleLED.setLed();

    mvp.loop();
}


uint32_t onDemandSetter(uint8_t led) {
    return Adafruit_NeoPixel::Color(random(255), random(255), random(255));
}

uint32_t effectCallback(uint8_t led, uint8_t position) {
    return Adafruit_NeoPixel::Color(position, 255-position, 127);
}