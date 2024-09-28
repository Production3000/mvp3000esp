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

#ifndef MVP3000_XMODULEEXAMPLE
#define MVP3000_XMODULEEXAMPLE

#include <Arduino.h>

#include <MVP3000.h>
extern MVP3000 mvp;


struct CfgXmoduleExample : public CfgJsonInterface {

    // Fixed settings, restored with reboot to value set at compile
    uint16_t fixedNumber = 10101;

    // Modifiable settings saved to SPIFFS
    uint16_t editableNumber = 22222;

    // The config name is used as SPIFFS file name
    CfgXmoduleExample() : CfgJsonInterface("XmoduleExample") {
        // Initialize settings for load/save to SPIFFS:
        //  name of the variable, to allow input from a web-form
        //  reference pointer to actual variable
        //  function to check range and assign value
        addSetting<uint16_t>(
            "editableNumber",
            &editableNumber,
            [&](const String& s) { uint16_t n = s.toInt(); if (n < 11111) return false; editableNumber = n; return true; }
        );
    }

};


class XmoduleExample : public _Xmodule {
    public:

        // The module name and the uri for its web interface. Leave the uri blank if the module has no web page.
        XmoduleExample() : _Xmodule("Xmodule Example", "/example") { };

        CfgXmoduleExample cfgXmoduleExample;

        void setup() override;
        void loop() override;

        void saveCfgCallback();

        // Module custom methods
        void someAction();

        // Web interface
        String webPageProcessor(uint8_t var) override;

        // We cannot override the values, but we can override this function.
        const char* getWebPage() override { return R"===(%0%
<p><a href='/'>Home</a></p>
<h3>%100%</h3>
<h3>Settings</h3> <ul>
    <li>Some fixed number: %101% </li>
    <li>Some editable number:<br> <form action='/save' method='post'> <input name='editableNumber' value='%102%' type='number' min='11112' max='65535'> <input type='submit' value='Save'> </form> </li> </ul>
<h3>Action</h3> <ul>
    <li>Perform some action:<br> <form action='/start' method='post'> <input name='someAction' type='hidden'> <input type='submit' value='Action'> </form> </li> </ul>
%9%)==="; }

};

#endif