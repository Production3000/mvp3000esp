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


#include <Adafruit_NeoPixel.h> // Better, get effects in other way



// char bitarray[4]; // since 4*8 this array actually contains 32 bits

// char getBit(int index) {
//     return (bitarray[index/8] >> 7-(index & 0x7)) & 0x1;
// }

// void setBit(int index, int value) {
//     bitarray[index/8] = bitarray[index/8] | (value & 0x1) << 7-(index & 0x7);
// }


struct CfgXmoduleLED : public CfgJsonInterface {

    uint8_t ledPin;

    // Modifiable settings saved to SPIFFS
    uint8_t ledCount = 1;
    uint8_t brightness = 25;
    uint16_t duration = 10000;

    // The config name is used as SPIFFS file name
    CfgXmoduleLED() : CfgJsonInterface("XmoduleLED") {
        // Initialize settings for load/save to SPIFFS:
        addSetting<uint8_t>("ledCount", &ledCount, [&](const String& s) { ledCount = s.toInt(); return true; } );
        addSetting<uint8_t>("brightness", &brightness, [&](const String& s) { brightness = s.toInt(); return true; } );
        addSetting<uint16_t>("duration", &duration, [&](const String& s) { duration = s.toInt(); return true; } );
    }
};


class XmoduleLED : public _Xmodule {

    public:

        enum class XLED_STATE: uint8_t {
            INTEFFECT = 0,
            EXTEFFECT = 1,
            ONDEMAND = 2,
        };
        XLED_STATE xledState = XLED_STATE::ONDEMAND;

        XmoduleLED() : _Xmodule("LED-Xmodule", "/led") { }
        XmoduleLED(uint8_t ledPin, uint8_t ledCount) : _Xmodule("LED-Xmodule", "/led") {
            cfgXmoduleLED.ledPin = ledPin;
            cfgXmoduleLED.ledCount = ledCount;
        }



        void setup() override;
        void loop() override;

        void setLed();
        void externalEffect();

        std::function<uint32_t(uint8_t)> onDemandSetter;
        std::function<uint32_t(uint8_t, uint8_t)> effectCallback;
        boolean effect = false;

        void setOnDemandSetter(std::function<uint32_t(uint8_t)> onDemandSetter) {
            xledState = XLED_STATE::ONDEMAND;
            this->onDemandSetter = onDemandSetter;
        }
        void setEffectCallback(std::function<uint32_t(uint8_t, uint8_t)> effectCallback) {
            xledState = XLED_STATE::EXTEFFECT;
            this->effectCallback = effectCallback;
        }
        void setEffect(uint8_t effect) {
            xledState = XLED_STATE::INTEFFECT;
            this->effect = true;
        }

    private:

        CfgXmoduleLED cfgXmoduleLED;

        Adafruit_NeoPixel* pixels;

        void runFx();
        uint8_t position = 0;
        LimitTimer fxTimer = LimitTimer(50);

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