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

#include "NetWeb.h"


/**
 * @class Xmodule
 * @brief Base class for modules.
 * 
 * This class provides a base implementation for modules in the MVP3000 system.
 * It defines common methods to ease integration of custom code into the MVP3000 framework, 
 * particularly for interaction and configuration via a web interface.
 */
class Xmodule {

    /**
     * @public
     * @brief Description of the module.
     */
    public:
        String description = "MVP3000 module base class";
        String uri = "/xmodule"; // Leave blank to only list module in web interface but not link it

        NetWeb::WebPage* webPageXmodule;

        /**
         * @brief Virtual method for module setup.
         * 
         * This method needs be implemented in the derived classes to perform module-specific setup tasks.
         * It is called once from the MVP3000 setup method.
         */
        virtual void setup() = 0;

        /** 
         * @brief Virtual method for module loop.
         * 
         * This method needs be implemented in the derived classes to perform module-specific loop tasks.
         * It is called repeatedly from the MVP3000 loop method.
         * 
         * @note This method should not have blocking delays AT ALL, as this interferes with the overall performance of the MVP3000 system.
         */
        virtual void loop() = 0;

};

#endif
