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

#ifndef MVP3000_XMODULELED
#define MVP3000_XMODULELED

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "_Xmodule.h"
#include "_Helper_LimitTimer.h"

#include "XmoduleLED_XledFx.h"
#include "XmoduleLED_webpage.h"


// Stacked RGB color as in the Adafruit library, copy here to provide to user script
#define ColorRGB(r, g, b) ((uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (uint32_t)(b)) 


struct CfgXmoduleLED : public CfgJsonInterface {

    uint8_t ledPin;
    uint8_t refreshRateFx_Hz = 40; // 40 Hz -> 25 ms
    uint8_t refreshRateStatic_s = 1; // s

    // Modifiable settings saved to SPIFFS
    uint8_t ledCount = 1;
    uint8_t globalBrightness = 150;

    // The config name is used as SPIFFS file name
    CfgXmoduleLED() : CfgJsonInterface("XmoduleLED") {
        // Initialize settings for load/save to SPIFFS:
        addSetting<uint8_t>("ledCount", &ledCount, [&](const String& s) { ledCount = s.toInt(); return true; } );
        addSetting<uint8_t>("globalBrightness", &globalBrightness, [&](const String& s) { globalBrightness = s.toInt(); return true; } );
    }
};



/**
 * @brief Xmodule for controlling a LED strip.
 * 
 * @param ledPin The pin the LED strip is connected to.
 * @param ledCount The number of LED in the strip.
 */
class XmoduleLED : public _Xmodule {

    public:
        XmoduleLED(uint8_t ledPin, uint8_t ledCount) : _Xmodule("LED-Xmodule", "/led") {
            cfgXmoduleLED.ledPin = ledPin;
            cfgXmoduleLED.ledCount = ledCount;

            delete [] currentColors;
            currentColors = new uint32_t[cfgXmoduleLED.ledCount];
            delete [] currentBrightness;
            currentBrightness = new uint8_t[cfgXmoduleLED.ledCount];

            for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
                currentColors[i] = 0;
                currentBrightness[i] = 255;
            }
        }


        /**
         * @brief Select a predefined brightness effect for the LED strip.
         * 
         * @param duration_ms Duration of the effect in milliseconds.
         * @param effect The pre-defined effect to use.
         */
        void setBrightnessEffect(uint16_t duration_ms, XledFx::BRIGHTNESSFX effect);

        /**
         * @brief Set a custom brightness effect for the LED strip.
         * 
         * @param duration_ms Duration of the effect in milliseconds.
         * @param useFrames If true, the effect is calculated for each subframe (40 Hz). If false, the effect is calculated only for the first frame of each cycle.
         * @param runEndless If true, the effect runs endlessly. If false, the effect stops after a single cycle.
         * @param brightnessSetter The function to calculate the brightness for each LED.
         */
        void setBrightnessEffect(uint16_t duration_ms, boolean useFrames, boolean runEndless, FxBrightnessSetter brightnessSetter);

        /**
         * @brief Set the brightness of each LED individually.
         * 
         * @param brightness An array of brightness values between 0 and 255.
         */
        void setFixedBrightnessIndividual(uint8_t* brightness);

        /**
         * @brief Set the brightness of all LEDs to the same value.
         * 
         * @param brightness Brightness value between 0 and 255.
         */
        void setFixedBrightnessSync(uint8_t brightness);

        /**
         * @brief Select a predefined color effect for the LED strip.
         * 
         * @param duration_ms Duration of the effect in milliseconds.
         * @param effect The pre-defined effect to use.
         */
        void setColorEffect(uint16_t duration_ms, XledFx::COLORFX effect);

        /**
         * @brief Select a predefined color effect for the LED strip and override the runEndless setting.
         * 
         * @param duration_ms Duration of the effect in milliseconds.
         * @param runEndless Overrides the effect default. If true, the effect runs endlessly. If false, the effect stops after a single cycle.
         * @param effect The pre-defined effect to use.
         */
        void setColorEffect(uint16_t duration_ms, boolean runEndless, XledFx::COLORFX effect);

        /**
         * @brief Set a custom color effect for the LED strip.
         * 
         * @param duration_ms Duration of the effect in milliseconds.
         * @param useFrames If true, the effect is calculated for each frame. If false, the effect is calculated only for the first frame of each cycle.
         * @param runEndless If true, the effect runs endlessly. If false, the effect stops after a single cycle.
         * @param colorWheel If true, the maximum 'position' of the color effect does not reach 255/65535. If false, the last 'position' is 255/65535.
         * @param colorSetter The function to calculate the color for each LED.
         */
        void setColorEffect(uint16_t duration_ms, boolean useFrames, boolean runEndless, boolean colorWheel, FxColorSetter colorSetter);

        /**
         * @brief Set the color of each LED individually.
         * 
         * @param colors An array of color values.
         */
        void setFixedColorIndividual(uint32_t* colors);

        /**
         * @brief Set the color of all LEDs to the same value.
         * 
         * @param color The color value.
         */
        void setFixedColorSync(uint32_t color);

        /**
         * @brief Use a photoresistor to automatically adapt the global brightness of the LED strip. This overrides the global brightness setting.
         * 
         * @param analogPin The analog pin to read the ambient light from.
         * @param analogBits (optional) The resolution of the ADC. If 0, the resolution of internal ADC is used: 10 bits for ESP8266, 12 bits for ESP32.
         */
        void adaptiveGlobalBrightness(uint8_t analogPin, uint8_t analogBits = 0);

        /**
         * @brief Set the global brightness of the LED strip. This turns adaptive global brightness off.
         * 
         * @param brightness Brightness value between 0 and 255.
         */
        void fixedGlobalBrightness(uint8_t globalBrightness);


    public:

        void setup() override;
        void loop() override;

    private:

        enum XLED_STATE: uint8_t {
            NOFX = 0,
            FXCOLOR = 1,
            FXBRIGHT = 2,
            FXFULL = 3,
        };
        XLED_STATE xledState = XLED_STATE::NOFX;
        void appendXledState(XLED_STATE state);
        void removeXledState(XLED_STATE state);

        CfgXmoduleLED cfgXmoduleLED;

        Adafruit_NeoPixel* pixels;

        XledFx xledFx;

        LimitTimer frameTimer = LimitTimer(cfgXmoduleLED.refreshRateStatic_s * 1000);
        void resetTimer();

        uint32_t* currentColors;
        uint8_t* currentBrightness;

        XledFx::FxCalculator brightnessFxCalculator;
        XledFx::FxCalculator colorFxCalculator;

        void drawLed();

        uint8_t adcPin;
        uint16_t adcBits;
        int16_t analogReading = -1;
        LimitTimer brightnessTimer = LimitTimer(250);
        void measureBrightness();


        void saveCfgCallback();

        String webPageProcessor(uint8_t var);
        PGM_P getWebPage() override { return htmlXmoduleLed; }

};

#endif