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
        //  function for range checks
        addSetting<uint16_t>(
            "editableNumber",
            &editableNumber,
            [](uint16_t _x) { return (_x < 11111) ? false : true; }
        );
    }

};


class XmoduleExample : public _Xmodule {
    public:

        // The module name and the uri for its web interface. Leave the uri blank if the module has no web page.
        XmoduleExample() : _Xmodule("Xmodule Example", "/example") { };

        CfgXmoduleExample cfgXmoduleExample;

        void setup() override {

            // Read config from SPIFFS
            mvp.config.readCfg(cfgXmoduleExample);

            // Register config
            mvp.net.netWeb.registerCfg(&cfgXmoduleExample, std::bind(&XmoduleExample::saveCfgCallback, this));

            // Register action
            mvp.net.netWeb.registerAction("someAction", [&](int args, WebArgKeyValue argKey, WebArgKeyValue argValue) {
                // argKey(0) is the action name
                someAction();
                return true;
            }, "Some action was performed.");
                
            // Custom setup code here
        }

        void loop() override {
            // Custom loop code here
        }

        void saveCfgCallback() {
            mvp.logger.write(CfgLogger::INFO, "The config was changed via the web interface.");
        }

        // Module custom methods
        void someAction() {
            mvp.logger.write(CfgLogger::INFO, "Some action performed.");
        }

        // Web interface
        String webPageProcessor(uint8_t var) override {

            String str;
            switch (var) {
                case 100:
                    return description.c_str();

                case 101:
                    return String(cfgXmoduleExample.fixedNumber);
                case 102:
                    return String(cfgXmoduleExample.editableNumber);

                default:
                    //  Capture all
                    // The placeholder string can hold a secondary placeholder which then is also filled, and so on.
                    // if (var.toInt() < 50) {
                    //     return "added a placeholder ( %" + String(var.toInt() + 1) + "% )";
                    // } else {
                    //     return "stop this madness!!!";
                    // }
                    //  Or log an unknown placeholder value
                    // mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Unknown placeholder in template: %s", var.c_str());
                    return str;
            }
        }

        const char*  getWebPage() override { return R"===(%0%
<p><a href='/'>Home</a></p>
<h3>%100%</h3>
<h3>Settings</h3> <ul>
    <li>Some fixed number: %101% </li>
    <li>Some editable number:<br> <form action='/save' method='post'> <input name='editableNumber' value='%102%' type='number' min='11112' max='65535'> <input type='submit' value='Save'> </form> </li> </ul>
<h3>Action</h3> <ul>
    <li>Perform some action:<br> <form action='/start' method='post'> <input name='someAction' type='hidden'> <input type='submit' value='Action'> </form> </li> </ul>   
<p>&nbsp;</body></html>)==="; }

};

#endif