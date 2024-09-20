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

#include "XmoduleExample.h"


void XmoduleExample::setup() {
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

void XmoduleExample::loop() { } // Custom loop code here


////////////////////////////////////////////////////////////////////////////////

void XmoduleExample::saveCfgCallback(){
    mvp.logger.write(CfgLogger::INFO, "The config was changed via the web interface.");
}

void XmoduleExample::someAction() {
    mvp.logger.write(CfgLogger::INFO, "Some action performed.");
}


////////////////////////////////////////////////////////////////////////////////

String XmoduleExample::webPageProcessor(uint8_t var)  {
    String str; // All vars need to be created outside of the switch statement
    switch (var) {
        case 100:
            return description.c_str();

        case 101:
            return String(cfgXmoduleExample.fixedNumber);
        case 102:
            return String(cfgXmoduleExample.editableNumber);

        // case 150: // Split long strings into multiple parts
        //     // Check if list is empty
        //     if (linkedListExample.getSize() == 0) {                                                                   
        //         return "<li>None</li>";
        //     }
        //     // Set initial bookmark
        //     linkedListExample.bookmarkByIndex(0, true);
        // case 151:
        //     return _helper.printFormatted("<li>%s</li> %s", linkedListExample.getBookmarkData()->getDataTopic().c_str(),
        //         (linkedListExample.moveBookmark()) ? "%151%" : "");

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