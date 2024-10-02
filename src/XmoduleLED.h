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


typedef std::function<uint32_t(uint8_t)> CallbackSingleSetter;
typedef std::function<void(uint32_t*, uint8_t)> CallbackSyncSetter;


struct CfgXmoduleLED : public CfgJsonInterface {

    uint8_t ledPin;
    uint8_t fxRefreshRate_Hz = 40; // 40 Hz -> 25 ms

    // Modifiable settings saved to SPIFFS
    uint8_t ledCount = 1;
    uint8_t brightness = 75;

    // The config name is used as SPIFFS file name
    CfgXmoduleLED() : CfgJsonInterface("XmoduleLED") {
        // Initialize settings for load/save to SPIFFS:
        addSetting<uint8_t>("ledCount", &ledCount, [&](const String& s) { ledCount = s.toInt(); return true; } );
        addSetting<uint8_t>("brightness", &brightness, [&](const String& s) { brightness = s.toInt(); return true; } );
    }
};


class XmoduleLED : public _Xmodule {

    public:

        XmoduleLED() : _Xmodule("LED-Xmodule", "/led") { }
        XmoduleLED(uint8_t ledPin, uint8_t ledCount) : _Xmodule("LED-Xmodule", "/led") {
            cfgXmoduleLED.ledPin = ledPin;
            cfgXmoduleLED.ledCount = ledCount;
        }

        void adaptiveBrightness(uint8_t analogPin) { this->analogPin = analogPin; }
        void setBrightness(uint8_t brightness);

        void setOnce(CallbackSingleSetter setOnceInfo);
        void setOnce(CallbackSyncSetter setOnceInfo);

        void demandLedUpdate();
        void setOnDemandSetter(CallbackSingleSetter onDemandSingleSetter) { setOnDemandCallback(onDemandSingleSetter, nullptr); }
        void setOnDemandSetter(CallbackSyncSetter onDemandSyncSetter) { setOnDemandCallback(nullptr, onDemandSyncSetter); }

        void setEffect(uint8_t effect);
        void setEffectSetter(FxSingleSetter fxCallback, uint16_t duration_ms, boolean onlyOnNewCycle = false) {  setEffect(fxCallback, nullptr, duration_ms, onlyOnNewCycle); }
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

   
        uint8_t analogPin;
        int16_t analogReadValue = -1;
        LimitTimer brightnessTimer = LimitTimer(250);
        void measureBrightness();

        CallbackSingleSetter onDemandSingleSetter;
        CallbackSyncSetter onDemandArraySetter;
        void setOnDemandCallback(CallbackSingleSetter onDemandSingleSetter = nullptr, CallbackSyncSetter onDemandArraySetter = nullptr);

        LimitTimer fxTimer = LimitTimer(1000/cfgXmoduleLED.fxRefreshRate_Hz);
        FxContainer fxContainer;
        void setEffect(FxSingleSetter singleSetter, FxSyncSetter syncSetter, uint16_t duration_ms, boolean onlyOnNewCycle);
        void executeEffect();

        void saveCfgCallback();

        String webPageProcessor(uint8_t var);
        const char*  getWebPage() override { return R"===(%0%
<p><a href='/'>Home</a></p>
<h3>%100%</h3>
<h3>Settings</h3> <ul>
    <li>Brigtness:<br> <form action='/save' method='post'> <input name='brightness' value='%101%' type='number' min='0' max='255'> <input type='submit' value='Save'> </form> </li>
    <li>Duration [ms]:<br> <form action='/save' method='post'> <input name='duration' value='%102%' type='number' min='0' max='65535'> <input type='submit' value='Save'> </form> </li>
    <li>Effect:<br> <form action='/save' method='post'> <select name='fxmode'> %110% </select> <input type='submit' value='Save'> </form> </li> </ul>
<h3>Action</h3> <ul>
    <li>Perform some action:<br> <form action='/start' method='post'> <input name='someAction' type='hidden'> <input type='submit' value='Action'> </form> </li> </ul>   
%9%)==="; }

};


#endif