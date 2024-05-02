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


class Xmodule {

    // Private can really only be accessed from the baseclass
    // Protected can be accessed from derived children
    // Public can be accessed from anywhere

    public:
        String description = "Module base class";

        virtual void setup() {};
        virtual void loop() {};

        virtual void contentModuleNetWeb() { };
        virtual bool editCfgNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg) { return true; };
        virtual bool startActionNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg) { return true; };
};

#endif
