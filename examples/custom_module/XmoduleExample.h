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

    // Modifiable settings saved to SPIFF
    uint16_t editableNumber = 22222;

    // Fixed settings, restored with reboot to value set at compile  
    uint16_t fixedNumber = 10101;

    CfgXmoduleExample() : CfgJsonInterface("XmoduleExample") {
        // Note above the config name, used as SPIFF file name

        // Initialize settings for load/save to SPIFF:
        //  var name string, to allow web-based form input (stored as int though)
        //  pointer to actual variable
        //  check function with optional range checks
        addSetting<uint16_t>(
            "editableNumber",
            &editableNumber,
            [](uint16_t _x) { return (_x < 11111) ? false : true; }
        );
    }

};


class XmoduleExample : public Xmodule {
    public:
        CfgXmoduleExample cfgXmoduleExample;

        void setup() override {
            description = "Xmodule Example";
            uri = "/example";

            // Read config from SPIFF
            mvp.config.readCfg(cfgXmoduleExample);

            // Define the module's web interface
            mvp.net.netWeb.registerPage(uri, R"===(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %0%</title>
    <script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
    <style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h2>MVP3000 - Device ID %0%</h2>
    <p><a href='/'>Home</a></p>
    <h3>%1%</h3>
    <h3>Settings</h3> <ul>
        <li>Some fixed number: %11% </li>
        <li>Some editable number:<br> <form action='/save' method='post'> <input name='editableNumber' value='%12%' type='number' min='11112' max='65535'> <input type='submit' value='Save'> </form> </li> </ul>
    <h3>Action</h3> <ul>
        <li>Perform some action:<br> <form action='/start' method='post'> <input name='someAction' type='hidden'> <input type='submit' value='Action'> </form> </li> </ul>   
<p>&nbsp;</body></html>         
                )===", [&](const String& var) -> String {
                    if (!mvp.helper.isValidInteger(var)) {
                        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Non-integer placeholder in template: %s (check for any unencoded percent symbol)", var.c_str());
                        return var;
                    }

                    String str;
                    switch (var.toInt()) {
                        case 0:
                            return String(ESPX.getChipId());

                        case 1:
                            return description.c_str();

                        case 11:
                            return String(cfgXmoduleExample.fixedNumber);
                        case 12:
                            return String(cfgXmoduleExample.editableNumber);

                        default: // Capture all
                            // The placeholder string can hold a secondary placeholder which then is also filled, and so on.
                            // if (var.toInt() < 50) {
                            //     return "added a placeholder ( %" + String(var.toInt() + 1) + "% )";
                            // } else {
                            //     return "stop this madness!!!";
                            // }
                            break;
                    }
                    mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Unknown placeholder in template: %s", var.c_str());
                    return var;
                }
            );

            // Register config
            mvp.net.netWeb.registerCfg(&cfgXmoduleExample);

            // Register action
            mvp.net.netWeb.registerAction("someAction", NetWeb::WebActionList::ResponseType::MESSAGE, [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
                // argKey(0) is the action name
                someAction();
                return true;
            }, "Some action was performed.");
                

            // Custom setup code here
        }

        void loop() override {
            // Custom loop code here
        }

        // Module custom methods

        void someAction() {
            mvp.logger.write(CfgLogger::INFO, "Some action performed.");
        }
};

#endif