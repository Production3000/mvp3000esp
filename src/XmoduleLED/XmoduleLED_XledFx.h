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

#ifndef XMODULELED_EFFECTS
#define XMODULELED_EFFECTS

#include <Arduino.h>

#include <Adafruit_NeoPixel.h>
#include <map>


// led, ledCount, timingPosition, currentColor/currentBrightness
typedef std::function<uint8_t(uint8_t, uint8_t, uint16_t, uint8_t**)> FxBrightnessSetter;
typedef std::function<uint32_t(uint8_t, uint8_t, uint16_t, uint32_t**)> FxColorSetter;

// useFrames, runEndless, setter function
typedef std::tuple<boolean, boolean, FxBrightnessSetter> FxBrightnessContainer;

// useFrames, runEndless, colorWheel, setter function
typedef std::tuple<boolean, boolean, boolean, FxColorSetter> FxColorContainer;

const char brfx_name_0[] PROGMEM = "Blink";
const char brfx_name_1[] PROGMEM = "Fade in";
const char brfx_name_2[] PROGMEM = "Fade out";
const char brfx_name_3[] PROGMEM = "Pulse full";
const char brfx_name_4[] PROGMEM = "Pulse upper half";
const char brfx_name_5[] PROGMEM = "Random all synced";
const char brfx_name_6[] PROGMEM = "Random individual sparkle";
const char brfx_name_7[] PROGMEM = "Random individual walk";
const char brfx_name_8[] PROGMEM = "Wave forward";
const char brfx_name_9[] PROGMEM = "Wave backward";

const char cfx_name_0[] PROGMEM = "Random all synced (loud)";
const char cfx_name_1[] PROGMEM = "Random all synced (pastel)";
const char cfx_name_2[] PROGMEM = "Random individual sparkle (loud)";
const char cfx_name_3[] PROGMEM = "Random individual sparkle (pastel)";
const char cfx_name_4[] PROGMEM = "Random individual walk (loud)";
const char cfx_name_5[] PROGMEM = "Random individual walk (pastel)";
const char cfx_name_6[] PROGMEM = "Rainbow all synced";
const char cfx_name_7[] PROGMEM = "Rainbow wave forward";
const char cfx_name_8[] PROGMEM = "Rainbow wave backward";


struct XledFx {
    enum BRIGHTNESSFX: uint8_t {
        BLINK = 0,
        FADE_IN = 1,
        FADE_OUT = 2,
        PULSE_FULL = 3,
        PULSE_HALF = 4,
        RND_SYNC = 5,
        RND_SPARKLE = 6,
        RND_WALK = 7,
        WAVE_FWD = 8,
        WAVE_BWD = 9,
    };

    enum COLORFX: uint8_t {
        RND_SYNC_LOUD = 0,
        RND_SYNC_PASTEL = 1,
        RND_SPARKLE_LOUD = 2,
        RND_SPARKLE_PASTEL = 3,
        RND_WALK_LOUD = 4,
        RND_WALK_PASTEL = 5,
        RAINBOW_SYNC = 6,
        RAINBOW_WAVE_FWD = 7,
        RAINBOW_WAVE_BWD = 8,
    };

    std::map<BRIGHTNESSFX, const __FlashStringHelper*> brightnessFxNames = {
        { BRIGHTNESSFX::BLINK, FPSTR(brfx_name_0) },
        { BRIGHTNESSFX::FADE_IN, FPSTR(brfx_name_1) },
        { BRIGHTNESSFX::FADE_OUT, FPSTR(brfx_name_2) },
        { BRIGHTNESSFX::PULSE_FULL, FPSTR(brfx_name_3) },
        { BRIGHTNESSFX::PULSE_HALF, FPSTR(brfx_name_4) },
        { BRIGHTNESSFX::RND_SYNC, FPSTR(brfx_name_5) },
        { BRIGHTNESSFX::RND_SPARKLE, FPSTR(brfx_name_6) },
        { BRIGHTNESSFX::RND_WALK, FPSTR(brfx_name_7) },
        { BRIGHTNESSFX::WAVE_FWD, FPSTR(brfx_name_8) },
        { BRIGHTNESSFX::WAVE_BWD, FPSTR(brfx_name_9) },
    };

    std::map<COLORFX, const __FlashStringHelper*> colorFxNames = {
        { COLORFX::RND_SYNC_LOUD, FPSTR(cfx_name_0) },
        { COLORFX::RND_SYNC_PASTEL, FPSTR(cfx_name_1) },
        { COLORFX::RND_SPARKLE_LOUD, FPSTR(cfx_name_2) },
        { COLORFX::RND_SPARKLE_PASTEL, FPSTR(cfx_name_3) },
        { COLORFX::RND_WALK_LOUD, FPSTR(cfx_name_4) },
        { COLORFX::RND_WALK_PASTEL, FPSTR(cfx_name_5) },
        { COLORFX::RAINBOW_SYNC, FPSTR(cfx_name_6) },
        { COLORFX::RAINBOW_WAVE_FWD, FPSTR(cfx_name_7) },
        { COLORFX::RAINBOW_WAVE_BWD, FPSTR(cfx_name_8) },
    };

    // useFrames, runEndless, setter function
    std::map<BRIGHTNESSFX, FxBrightnessContainer> brightnessFx = {
        { FADE_IN, std::make_tuple(true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t** currentBrightness) { return timingPosition / 256; }) },
        { FADE_OUT, std::make_tuple(true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t** currentBrightness) { return 255 - timingPosition / 256; }) },
        { BLINK, std::make_tuple(false, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t** currentBrightness) { return (timingPosition > 32767) ? 0 : 255; }) },
        { PULSE_FULL, std::make_tuple(true, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t** currentBrightness) { return 2 * (uint8_t)abs((0.5 + (int16_t)timingPosition) / 256); }) },
        { PULSE_HALF, std::make_tuple(true, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t** currentBrightness) { return 255 - (uint8_t)abs((0.5 + (int16_t)timingPosition) / 256); }) },
        { RND_SYNC, std::make_tuple(false, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t** currentBrightness) {
            return (led == 0) ? random(256) : (*currentBrightness)[0]; }) },
        { RND_SPARKLE, std::make_tuple(false, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t** currentBrightness) { return random(256); }) },
        { RND_WALK, std::make_tuple(false, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t** currentBrightness) {
            int16_t temp = (*currentBrightness)[led] + random(-4, 5);
            return constrain(temp, 0, 255); }) },
        { WAVE_FWD, std::make_tuple(true, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t** currentBrightness) { return nearbyintf( 255.0/2 * ( 1 + sin(2 * PI * led / (ledcount - 1) - 2 * PI * timingPosition / 65535) ) ); }) },
        { WAVE_BWD, std::make_tuple(true, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t** currentBrightness) { return nearbyintf( 255.0/2 * ( 1 + sin(2 * PI * led / (ledcount - 1) + 2 * PI * timingPosition / 65535) ) ); }) },
    };

    // useFrames, runEndless, colorWheel, setter function
    std::map<COLORFX, FxColorContainer> colorFx = {
        { RND_SYNC_LOUD, std::make_tuple(false, true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t** currentColor) { return (led == 0) ? Adafruit_NeoPixel::ColorHSV(random(65536)) : (*currentColor)[0]; }) },
        { RND_SYNC_PASTEL, std::make_tuple(false, true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t** currentColor) { return (led == 0) ? Adafruit_NeoPixel::Color(random(256), random(256), random(256)) : (*currentColor)[0]; }) },
        { RND_SPARKLE_LOUD, std::make_tuple(false, true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t** currentColor) { return Adafruit_NeoPixel::ColorHSV(random(65536)); }) },
        { RND_SPARKLE_PASTEL, std::make_tuple(false, true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t** currentColor) { return Adafruit_NeoPixel::Color(random(256), random(256), random(256)); }) },
        { RND_WALK_LOUD, std::make_tuple(false, true, false, [&](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t** currentColor) {
            uint16_t h;
            uint8_t s, v;
            rgb2hsv((*currentColor)[led], h, s, v);
            // -1/+1 * (255 + 0..255)
            return Adafruit_NeoPixel::ColorHSV(h + (2 * random(0,2) - 1) * (255 + random(256))); }) },
        { RND_WALK_PASTEL, std::make_tuple(false, true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t** currentColor) {
            int16_t r = (uint8_t)((*currentColor)[led] >> 16) + random(-4, 5);
            int16_t g = (uint8_t)((*currentColor)[led] >> 8) + random(-4, 5);
            int16_t b = (uint8_t)(*currentColor)[led] + random(-4, 5);
            return Adafruit_NeoPixel::Color(constrain(r, 0, 255), constrain(g, 0, 255), constrain(b, 0, 255)); }) },
        { RAINBOW_SYNC, std::make_tuple(true, true, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t** currentColor) { return Adafruit_NeoPixel::ColorHSV(timingPosition); }) },
        { RAINBOW_WAVE_FWD, std::make_tuple(true, true, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t** currentColor) { return Adafruit_NeoPixel::ColorHSV(65535 * led / (ledcount - 1) + timingPosition); }) },
        { RAINBOW_WAVE_BWD, std::make_tuple(true, true, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t** currentColor) { return Adafruit_NeoPixel::ColorHSV(65535 * led / (ledcount - 1) - timingPosition); }) },
    };



    struct FxCalculator {
        uint16_t frame = 0;

        uint16_t duration_ms;
        boolean runEndless;
        boolean useFrames;

        FxBrightnessSetter brightnessSetter = nullptr;

        uint32_t colorA = 0;
        uint32_t colorB = 0;
        boolean colorWheel = false;
        FxColorSetter colorSetter = nullptr;

        FxCalculator() {};
        FxCalculator(uint16_t duration_ms, boolean useFrames, boolean runEndless, FxBrightnessSetter brightnessSetter) : duration_ms(duration_ms), useFrames(useFrames), runEndless(runEndless), 
            brightnessSetter(brightnessSetter) {};
        FxCalculator(uint16_t duration_ms, boolean useFrames, boolean runEndless, boolean colorWheel, FxColorSetter colorSetter) : duration_ms(duration_ms), useFrames(useFrames), runEndless(runEndless),
            colorWheel(colorWheel), colorSetter(colorSetter) {};

        boolean calculate(uint8_t refreshRateFx_Hz, uint8_t** currentBrightness, uint8_t ledCount) {
            return calculate(refreshRateFx_Hz, currentBrightness, nullptr, ledCount);
        }
        boolean calculate(uint8_t refreshRateFx_Hz, uint32_t** currentColor, uint8_t ledCount) {
            return calculate(refreshRateFx_Hz, nullptr, currentColor, ledCount);
        }

        boolean calculate(uint8_t refreshRateFx_Hz, uint8_t** currentBrightness, uint32_t** currentColor, uint8_t ledCount) {
            // Omitting check to see if brightnessSetter/colorSetter is actually set

            // Limited resolution for short durations: 40 * 140 / 1000 = 5 --> 125 ms instead of the targeted 140 ms
            uint16_t frameCount = refreshRateFx_Hz * duration_ms / 1000 - 1;
            // Some color effects work best as a wheel (65535 = 0) and others need the full range (0 to 255)
            uint16_t dividingFrameCount = (colorWheel) ? frameCount + 1: frameCount;

            // Effects can either be many gradual steps per duration cycle (fade, wheel) or have a single change per duration cycle (blink, random color change).
            if (useFrames || (frame == 0)) {
                uint16_t timingPosition = 65535 * frame / dividingFrameCount;
                for (uint8_t i = 0; i < ledCount; i++) {
                    if (brightnessSetter != nullptr)
                        (*currentBrightness)[i] = brightnessSetter(i, ledCount, timingPosition, currentBrightness);
                    if (colorSetter != nullptr)
                        (*currentColor)[i] = colorSetter(i, ledCount, timingPosition, currentColor);
                }
            }

            frame++;
            if (frame >= frameCount) {
                frame = 0;
                if (!runEndless) {
                    return false;
                }
            }
            return true;
        }
    };


    // Get the HSV values from the stored RGB color
    void rgb2hsv(uint32_t currentColor, uint16_t& h, uint8_t& s, uint8_t& v) {
        uint8_t r = (uint8_t)(currentColor >> 16);
        uint8_t g = (uint8_t)(currentColor >> 8);
        uint8_t b = (uint8_t)currentColor;

        uint8_t rgbMax = (r > g) ? r : g;
        rgbMax = (rgbMax > b) ? rgbMax : b;

        if (rgbMax == 0) { // Black
            h = 0;
            s = 0;
            v = 0;
            return;
        }
        v = rgbMax;

        uint8_t rgbMin = (r < g) ? r : g;
        rgbMin = (rgbMin < b) ? rgbMin : b;
        float_t delta = rgbMax - rgbMin;
        if (delta < 1) { // == 0 Gray
            h = 0;
            s = 0;
            return;
        }
        s = 255 * (float_t)delta / v;

        if (rgbMax == r)
            h = nearbyintf(10922.5 * float_t(g - b) / delta); // Red centered at 65535/0 rollover
        else if (rgbMax == g)
            h = nearbyintf(21845.0 + (10922.5 * float_t(b - r) / delta));
        else
            h = nearbyintf(43690.0 + (10922.5 * float_t(r - g) / delta));
    };

};

#endif