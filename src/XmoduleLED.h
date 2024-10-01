
#ifndef MVP3000_XMODULELED
#define MVP3000_XMODULELED

#include <Arduino.h>

#include <MVP3000.h>
extern MVP3000 mvp;


#include <WS2812FX.h> // TODO remove, use Adafruit_NeoPixel
#include <Adafruit_NeoPixel.h> // Better, get effects in other way


struct CfgXmoduleLED : public CfgJsonInterface {

    // Modifiable settings saved to SPIFFS
    uint8_t brightness = 25;
    uint16_t duration = 10000;
    uint8_t fxMode = 11;

    // The config name is used as SPIFFS file name
    CfgXmoduleLED() : CfgJsonInterface("XmoduleLED") {
        // Initialize settings for load/save to SPIFFS:
        addSetting<uint8_t>("brightness", &brightness, [&](const String& s) { brightness = s.toInt(); return true; } );
        addSetting<uint16_t>("duration", &duration, [&](const String& s) { duration = s.toInt(); return true; } );
        addSetting<uint8_t>("fxMode", &fxMode, [&](const String& s) { fxMode = s.toInt(); return true; } );
    }
};


class XmoduleLED : public _Xmodule {

    public:
        XmoduleLED() : _Xmodule("LED-Xmodule", "/led") { }
        XmoduleLED(uint16_t led_count, uint8_t led_pin) : _Xmodule("LED-Xmodule", "/led"), led_count(led_count), led_pin(led_pin) { }

        void setup() override;
        void loop() override;

    private:

        CfgXmoduleLED cfgXmoduleLED;

        WS2812FX* ws2812fx;

        uint16_t led_count;
        uint8_t led_pin;

        uint8_t currentFxMode = 255;

        void setLed();

        void saveCfgCallback();

        String webPageProcessor(uint8_t var);
        char const* webPage = R"===(%0%
<p><a href='/'>Home</a></p>
<h3>%100%</h3>
<h3>Settings</h3> <ul>
    <li>Brigtness:<br> <form action='/save' method='post'> <input name='brightness' value='%101%' type='number' min='0' max='255'> <input type='submit' value='Save'> </form> </li>
    <li>Duration [ms]:<br> <form action='/save' method='post'> <input name='duration' value='%102%' type='number' min='0' max='65535'> <input type='submit' value='Save'> </form> </li>
    <li>Effect:<br> <form action='/save' method='post'> <select name='fxmode'> %110% </select> <input type='submit' value='Save'> </form> </li> </ul>
<h3>Action</h3> <ul>
    <li>Perform some action:<br> <form action='/start' method='post'> <input name='someAction' type='hidden'> <input type='submit' value='Action'> </form> </li> </ul>   
%9%)===";

};


#endif