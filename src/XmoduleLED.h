
#ifndef MVP3000_XMODULELED
#define MVP3000_XMODULELED

#include <Arduino.h>

#include <MVP3000.h>
extern MVP3000 mvp;


#include <WS2812FX.h>


struct CfgXmoduleLED : public CfgJsonInterface {

    // Fixed settings, restored with reboot to value set at compile  
    uint16_t fixedNumber = 10101;

    // Modifiable settings saved to SPIFFS
    uint16_t brightness = 25;
    uint16_t duration = 10000;
    uint16_t fxMode = 11;

    // The config name is used as SPIFFS file name
    CfgXmoduleLED() : CfgJsonInterface("XmoduleLED") {
        // Initialize settings for load/save to SPIFFS:
        //  name of the variable, to allow input from a web-form
        //  reference pointer to actual variable
        //  function for range checks
        addSetting<uint16_t>("brightness", &brightness, [](uint8_t _x) { return true; });
        addSetting<uint16_t>("duration", &duration, [](uint16_t _x) { return true; }); // 
        addSetting<uint16_t>("fxmode", &fxMode, [](uint8_t _x) { return (_x < 80) ? true : false; });
    }
};


class XmoduleLED : public Xmodule {

    public:
        XmoduleLED() : Xmodule("LED-Xmodule", "/led") { }
        XmoduleLED(uint16_t led_count, uint8_t led_pin) : Xmodule("LED-Xmodule", "/led"), led_count(led_count), led_pin(led_pin) { }

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
        char const* webPage = R"===(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %1%</title>
    <script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
    <style>body { font-family: sans-serif; } table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h2>MVP3000 - Device ID %1%</h2> <h3 style='color: red;'>%0%</h3>
    <p><a href='/'>Home</a></p>
    <h3>%100%</h3>
    <h3>Settings</h3> <ul>
        <li>Brigtness:<br> <form action='/save' method='post'> <input name='brightness' value='%101%' type='number' min='0' max='255'> <input type='submit' value='Save'> </form> </li>
        <li>Duration [ms]:<br> <form action='/save' method='post'> <input name='duration' value='%102%' type='number' min='0' max='65535'> <input type='submit' value='Save'> </form> </li>
        <li>Effect:<br> <form action='/save' method='post'> <select name='fxmode'> %110% </select> <input type='submit' value='Save'> </form> </li> </ul>
    <h3>Action</h3> <ul>
        <li>Perform some action:<br> <form action='/start' method='post'> <input name='someAction' type='hidden'> <input type='submit' value='Action'> </form> </li> </ul>   
<p>&nbsp;</body></html>         
)===";

};


#endif