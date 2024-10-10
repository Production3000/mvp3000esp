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
#include <map>

#include <MVP3000.h>
extern MVP3000 mvp;



// struct PixelGroup {
//     PixelGroup(uint8_t ledCount, uint8_t start, uint8_t end) : ledCount(ledCount) {
//         bitarray[0] = 0;
//         bitarray[1] = 0;
//         bitarray[2] = 0;
//         bitarray[3] = 0;

//         for (uint8_t i = start; i <= end; i++) {
//             setBit(i, 1);
//         }
//     }

//     uint8_t ledCount;

//     char bitarray[4]; // since 4*8 this array actually contains 32 bits

//     char getBit(int index) {
//         return (bitarray[index/8] >> 7-(index & 0x7)) & 0x1;
//     }

//     void setBit(int index, int value) {
//         bitarray[index/8] = bitarray[index/8] | (value & 0x1) << 7-(index & 0x7);
//     }
// };


typedef std::function<uint32_t(uint8_t, uint8_t, uint16_t, uint32_t*)> FxColorSetter;
typedef std::function<uint8_t(uint8_t, uint8_t, uint16_t, uint8_t*)> FxBrightnessSetter;
typedef std::tuple<boolean, boolean, FxColorSetter> FxColorContainer;
typedef std::tuple<boolean, boolean, FxBrightnessSetter> FxContainer;

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
    TRANSITION = 0,
    RND_SYNC_LOUD = 2,
    RND_SYNC_PASTEL = 3,
    RND_SPARKLE_LOUD = 4,
    RND_SPARKLE_PASTEL = 5,
    RND_WALK_LOUD = 6,
    RND_WALK_PASTEL = 7,
    RAINBOW_SYNC = 8,
    RAINBOW_WAVE_FWD = 9,
    RAINBOW_WAVE_BWD = 10,
};


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

        void setRandomColor() { 
            removeXledState(XLED_STATE::FXCOLOR);
            for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
                currentColors[i] = Adafruit_NeoPixel::ColorHSV(random(65536), 255, 255);
            }
            resetTimer();
        }


        void setSeparateColor(uint32_t* colors) {         // NumberArray !! has loop
            removeXledState(XLED_STATE::FXCOLOR);
            for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
                currentColors[i] = colors[i];
            }
            resetTimer();
        }

        void setSeparateBrightness(uint8_t* brightness) {
            removeXledState(XLED_STATE::FXBRIGHT);
            for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; ++i) {
                currentBrightness[i] = brightness[i];
            }
            resetTimer();
        }

        void setSyncColor(uint32_t color) {
            removeXledState(XLED_STATE::FXCOLOR);
            std::fill_n(currentColors, cfgXmoduleLED.ledCount, color);
            resetTimer();
        }

        void setSyncBrightness(uint8_t brightness) {
            removeXledState(XLED_STATE::FXBRIGHT);
            std::fill_n(currentBrightness, cfgXmoduleLED.ledCount, brightness);
            resetTimer();
        }

        void resetTimer() {
            if (xledState == XLED_STATE::ONDEMAND) {
                updateTimer.restart(cfgXmoduleLED.refreshRateStatic_s * 1000);
            } else {
                updateTimer.restart(1000/cfgXmoduleLED.refreshRateFx_Hz);
            }
        }

        /**
         * @brief Use a photoresistor to automatically adapt the global brightness of the LED strip. This overrides the global brightness setting.
         * 
         * @param analogPin The analog pin to read the ambient light from.
         * @param analogBits (optional) The resolution of the ADC. If 0, the resolution of internal ADC of the ESP is used.
         */
        void adaptiveGlobalBrightness(uint8_t analogPin, uint8_t analogBits = 0);

        /**
         * @brief Set the global brightness of the LED strip. This turns adaptive global brightness off.
         * 
         * @param brightness Brightness value between 0 and 255.
         */
        void setGlobalBrightness(uint8_t globalBrightness);



        void setBrightnessEffect(uint16_t duration_ms, BRIGHTNESSFX effect);
        void setBrightnessEffect(uint16_t duration_ms, boolean onlyOnNewCycle, boolean runOnlyOnce, FxBrightnessSetter brightnessSetter);

        void setColorEffect(uint16_t duration_ms, COLORFX effect);
        void setColorEffect(uint16_t duration_ms, boolean onlyOnNewCycle, boolean runOnlyOnce, FxColorSetter colorSetter);


    public:

        void setup() override;
        void loop() override;

    private:

        enum XLED_STATE: uint8_t {
            ONDEMAND = 0,
            FXCOLOR = 1,
            FXBRIGHT = 2,
            FXFULL = 3,
        };
        XLED_STATE xledState = XLED_STATE::ONDEMAND;
        void appendXledState(XLED_STATE state) {
            if ((xledState == state) || (state == XLED_STATE::FXFULL)) // nothing to do
                return;
            xledState = static_cast<XLED_STATE>(xledState + state);
        }
        void removeXledState(XLED_STATE state) {
            if (xledState == XLED_STATE::ONDEMAND)
                return;
            if ((xledState == state) || (state == XLED_STATE::FXFULL))
                xledState = static_cast<XLED_STATE>(xledState - state);
        }

        CfgXmoduleLED cfgXmoduleLED;

        Adafruit_NeoPixel* pixels;


        uint32_t* currentColors;
        uint8_t* currentBrightness;

        FxColorSetter fxColorSetter = nullptr;
        FxBrightnessSetter fxBrightnessSetter = nullptr;

        LimitTimer updateTimer = LimitTimer(cfgXmoduleLED.refreshRateStatic_s * 1000);

        void drawLed();


        uint16_t fxBrightnessFrame = 0;
        uint16_t fxBrightnessDuration_ms;
        boolean fxBrightnessOnlyOnNewCycle;
        boolean fxBrightnessRunOnlyOnce;
        void calculateBrightnessEffect();

        uint16_t fxColorFrame = 0;
        uint16_t fxColorDuration_ms;
        boolean fxColorOnlyOnNewCycle;
        boolean fxColorRunOnlyOnce;
        void calculateColorEffect();



        // PixelGroup* pixelGroup;

        uint8_t adcPin;
        uint16_t adcBits;
        int16_t analogReading = -1;
        LimitTimer brightnessTimer = LimitTimer(250);
        void measureBrightness();


        void saveCfgCallback();

        String webPageProcessor(uint8_t var);
        const char*  getWebPage() override { return R"===(%0%
<p><a href='/'>Home</a></p>
<h3>%100%</h3>
<h3>Settings</h3> <ul>
    <li>Brigtness:<br> <form action='/save' method='post'> <input name='globalBrightness' value='%101%' type='number' min='0' max='255'> <input type='submit' value='Save'> </form> </li>
    <li>Duration [ms]:<br> <form action='/save' method='post'> <input name='duration' value='%102%' type='number' min='0' max='65535'> <input type='submit' value='Save'> </form> </li>
    <li>Effect:<br> <form action='/save' method='post'> <select name='fxmode'> %110% </select> <input type='submit' value='Save'> </form> </li> </ul>
<h3>Action</h3> <ul>
    <li>Perform some action:<br> <form action='/start' method='post'> <input name='someAction' type='hidden'> <input type='submit' value='Action'> </form> </li> </ul>   
%9%)==="; }

};


#endif