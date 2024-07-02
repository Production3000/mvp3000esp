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
            description = "XmoduleExample";

            // Read config
            mvp.config.readCfg(cfgXmoduleExample);

            // Custom setup code here
        }

        void loop() override {
            // Custom loop code here
        }

        // 
        void contentModuleNetWeb() override {
            mvp.net.netWeb.sendFormatted("<h1>%s</h1>", description.c_str());

            // Settings
            mvp.net.netWeb.sendFormatted("\
                <h3>Settings</h3> <ul> \
                <li>Some fixed number: %d </li> \
                <li>Some editable number:<br> <form action='/save' method='post'> <input name='editableNumber' value='%d' type='number' min='11112' max='65535'> <input type='submit' value='Save'> </form> </li> </ul>",
                cfgXmoduleExample.editableNumber, cfgXmoduleExample.fixedNumber);

            // Actions
            mvp.net.netWeb.sendFormatted("\
                <h3>Action</h3> <ul> \
                <li>Perform some action:<br> <form action='/start' method='post'> <input name='someAction' type='hidden'> <input type='submit' value='Action'> </form> </li> </ul>");
        }

        bool editCfgNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg) override {
            boolean success = cfgXmoduleExample.updateFromWeb(argName(0), arg(0));
            if (success)
                mvp.config.writeCfg(cfgXmoduleExample);
            return success;
        }

        bool startActionNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg) override {
            boolean success = true;
            switch (mvp.helper.hashStringDjb2(argName(0).c_str())) {

                case mvp.helper.hashStringDjb2("someAction"):
                    someAction();
                    mvp.net.netWeb.responseRedirect("Some action performed.");
                    break;

                default: // Keyword not found
                    success = false;
            }
            return success;
        }

        // Module custom methods

        void someAction() {
            mvp.logger.write(CfgLogger::INFO, "Some action performed.");
        }
};


#endif