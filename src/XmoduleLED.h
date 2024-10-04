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

#include <MVP3000.h>
extern MVP3000 mvp;


#include <Adafruit_NeoPixel.h>

#include "XmoduleLED_effects.h"


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


typedef std::function<uint32_t()> CallbackSyncSetter;
typedef std::function<uint32_t(uint8_t)> CallbackSeparateSetter;
typedef std::function<void(uint32_t*, uint8_t)> CallbackArraySetter;


struct CfgXmoduleLED : public CfgJsonInterface {

    uint8_t ledPin;
    uint8_t fxRefreshRate_Hz = 40; // 40 Hz -> 25 ms

    // Modifiable settings saved to SPIFFS
    uint8_t ledCount = 1;
    uint8_t globalBrightness = 75;

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



        void setLed(CallbackSyncSetter setOnceSyncSetter); // single color, why ??? here this is stupid
        void setLed(CallbackSeparateSetter setOnceSeparateSetter);
        void setLed(CallbackArraySetter setOnceArraySetter);

        /**
         * @brief Demand an update of the LED strip. This is necessary if the LED display depends on the status of the script.
         */
        void demandLedUpdate();

        /**
         * 
         */
        void setOnDemandSetter(CallbackArraySetter onDemandArraySetter) { setOnDemandCallback(nullptr, nullptr, onDemandArraySetter); }


        void setOnDemandSetter(CallbackSeparateSetter onDemandSeparateSetter) { setOnDemandCallback(nullptr, onDemandSeparateSetter, nullptr); }


        void setOnDemandSetter(CallbackSyncSetter onDemandSyncSetter) { setOnDemandCallback(onDemandSyncSetter, nullptr, nullptr); }


        /**
         * @brief Display a pre-defined effect.
         * 
         * @param effect The effect to set.
         */
        void setEffect(uint8_t effect);

        /**
         * @brief Set a custom effect. Each LED is set individually.
         * 
         * @param fxCallback The setter function defining the current color of each LED.
         * @param duration_ms The duration of the effect in milliseconds.
         * @param onlyOnNewCycle (optional) If true, the callback is only executed on start of a new cycle.
         */
        void setEffectSetter(FxSeparateSetter fxCallback, uint16_t duration_ms, boolean onlyOnNewCycle = false) {  setEffect(fxCallback, nullptr, duration_ms, onlyOnNewCycle); }

        /**
         * @brief Set a custom effect. All LED are synchronized to display the same color.
         * 
         * @param fxCallback The setter function defining the current color of all LED.
         * @param duration_ms The duration of the effect in milliseconds.
         * @param onlyOnNewCycle (optional) If true, the callback is only executed on start of a new cycle.
         */
        void setEffectSetter(FxSyncSetter fxCallback, uint16_t duration_ms, boolean onlyOnNewCycle = false) { setEffect(nullptr, fxCallback, duration_ms, onlyOnNewCycle); }

    public:

        void setup() override;
        void loop() override;

    private:

        enum class XLED_STATE: uint8_t {
            EFFECT = 0,
            ONDEMAND = 1,
        };
        XLED_STATE xledState = XLED_STATE::ONDEMAND;

        CfgXmoduleLED cfgXmoduleLED;

        Adafruit_NeoPixel* pixels;

        XledFx xledFx;

        // PixelGroup* pixelGroup;

        uint8_t adcPin;
        uint16_t adcBits;
        int16_t analogReading = -1;
        LimitTimer brightnessTimer = LimitTimer(250);
        void measureBrightness();

        CallbackSyncSetter onDemandSyncSetter;
        CallbackSeparateSetter onDemandSeparateSetter;
        CallbackArraySetter onDemandArraySetter;
        void setOnDemandCallback(CallbackSyncSetter syncSetter, CallbackSeparateSetter separateSetter, CallbackArraySetter arraySetter);

        LimitTimer fxTimer = LimitTimer(1000/cfgXmoduleLED.fxRefreshRate_Hz);
        FxContainer fxContainer;
        void setEffect(FxSeparateSetter separateSetter, FxSyncSetter syncSetter, uint16_t duration_ms, boolean onlyOnNewCycle);
        void executeEffect();

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