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


typedef std::function<uint32_t(uint8_t, uint16_t)> FxSingleSetter; // Each LED with own color
typedef std::function<uint32_t(uint16_t)> FxSyncSetter; // All LED in syncronuous mode

struct FxContainer {
    FxSingleSetter singleSetter;
    FxSyncSetter syncSetter;
    uint16_t timingPosition = 0;
    uint16_t duration_ms;
    boolean onlyOnNewCycle;

    FxContainer() {}
    FxContainer(FxSingleSetter singleSetter, FxSyncSetter syncSetter, uint16_t duration_ms, boolean onlyOnNewCycle = false) : singleSetter(singleSetter), syncSetter(syncSetter), duration_ms(duration_ms), onlyOnNewCycle(onlyOnNewCycle) {}
};


struct XledFx {

    FxContainer getFxContainer(uint8_t effect) {
        if (effect == 1) {
            return FxContainer(nullptr, std::bind(&XledFx::fxSync, this, std::placeholders::_1), fxDurationFast_ms);
        } else {
            return FxContainer(nullptr, std::bind(&XledFx::fxSync, this, std::placeholders::_1), fxDurationSlow_ms);
        }
    }

    uint16_t fxDurationFast_ms = 500;
    uint16_t fxDurationSlow_ms = 5000;
    uint32_t fxSync(uint16_t position) {
        float_t phase = TWO_PI * position / std::numeric_limits<uint16_t>::max();
        uint8_t r = ( sin(phase + 0) + 1 ) * 255 / 2;
        uint8_t g = ( sin(phase + TWO_PI/3) + 1 ) * 255 / 2;
        uint8_t b = ( sin(phase + TWO_PI/3*2) + 1 ) * 255 / 2;
        return Adafruit_NeoPixel::Color(r, g, b);
    }
};

#endif