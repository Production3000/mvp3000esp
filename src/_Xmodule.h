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

#ifndef MVP3000_XMODULE
#define MVP3000_XMODULE

#include <Arduino.h>

#include "Config.h"

/**
 * @class Xmodule
 * @brief Base class for modules.
 *
 * This class provides a base implementation for modules in the MVP3000 system.
 */
class _Xmodule {

    public:

        _Xmodule(String description) : description(description) {};
        _Xmodule(String description, String uri) : description(description), uri(uri) {
            setupFramework();
        };

        String description;
        String uri = ""; // Leave blank to only list module in web interface but not link it

        virtual void setup() { };
        virtual void loop() { };

        virtual String webPageProcessor(uint8_t var) { return ""; };
        // We cannot override the values, but we can override this function.
        virtual const char* getWebPage() { return ""; };

        void mqttPrint(const String& topic, const String& payload);
        void webSocketPrint(const String& topic, const String& payload);
        
    private:
        void setupFramework();

};

#endif
