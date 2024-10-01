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

#ifndef MVP3000_XMODULELED_EFFECTS
#define MVP3000_XMODULELED_EFFECTS

#include <Arduino.h>

#include <Adafruit_NeoPixel.h>

typedef std::function<uint32_t(uint8_t, uint8_t)> FxSingleSetter;
typedef std::function<void(uint32_t*, uint8_t, uint8_t)> FxArraySetter;


struct XledFx {

    FxArraySetter getEffect(uint8_t effect) {
        if (effect == 1) {
            return std::bind(&XledFx::intEffect1, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        } else {
            return std::bind(&XledFx::intEffect2, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        }
    }

    void intEffect1(uint32_t* ledArray, uint8_t ledCount, uint8_t position) {
        uint8_t steps = 255;

        float_t phase = TWO_PI * position / steps;
        uint8_t r = ( sin(phase + 0) + 1 ) * 255 / 2;
        uint8_t g = ( sin(phase + TWO_PI/3) + 1 ) * 255 / 2;
        uint8_t b = ( sin(phase + TWO_PI/3*2) + 1 ) * 255 / 2;

        for (uint8_t i = 0; i < ledCount; i++) {
            ledArray[i] = Adafruit_NeoPixel::Color(r, g, b);
        }
    }

    void intEffect2(uint32_t* ledArray, uint8_t ledCount, uint8_t position) {
        uint8_t steps = 255;

        float_t phase = TWO_PI * position / steps;
        uint8_t r = ( sin(phase + 0) + 1 ) * 255 / 2;
        uint8_t g = 0;
        uint8_t b = 0;

        for (uint8_t i = 0; i < ledCount; i++) {
            ledArray[i] = Adafruit_NeoPixel::Color(r, g, b);
        }
    }

};

#endif