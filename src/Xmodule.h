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

        /**
         * @brief Virtual method for module setup.
         * 
         * This method can be overridden by derived classes to perform module-specific setup tasks.
         * It is called once from the MVP3000 setup method.
         */
        virtual void setup() {};

        /** 
         * @brief Virtual method for module loop.
         * 
         * This method needs be implemented in the derived classes to perform module-specific loop tasks.
         * It is called repeatedly from the MVP3000 loop method.
         * 
         * @note This method should not block or have long delays, as it may affect the overall performance of the MVP3000 system.
         */
        virtual void loop() = 0;

        /**
         * @brief Virtual method for module network web content.
         * 
         * This method can be overridden by derived classes to provide network web content for the module.
         * It is called from the serveRequest method in the netWeb class when the user accesses the module's web page.
         * If left unimplemented, it should be disabled by setting enableContentModuleNetWeb to false.
         */
        bool enableContentModuleNetWeb = true;
        virtual void contentModuleNetWeb() { };

        /**
         * @brief Virtual method for editing module network web configuration.
         * 
         * This method can be overridden by derived classes to handle editing module network web configuration.
         * It is called from the editCfg method in the netWeb class when the user saves a module specific configuration.
         * Depending of return value, the netWeb class will display a success or error message to the user.
         * It can be left unimplemented if the module does not require any configuration.
         * 
         * @param args The number of arguments in the web request.
         * @param argName A function that returns the name of the web argument given its index as string.
         * @param arg A function that returns the value of the web argument given its index as string.
         * @return True if the configuration was edited successfully, false otherwise.
         */
        virtual bool editCfgNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg) { return false; };

        /**
         * @brief Virtual method for starting an action in the module via network web.
         * 
         * This method can be overridden by derived classes to handle starting an action in the module via network web.
         * It is called from the startAction method in the netWeb class when the user triggers an action in the module.
         * Only the return value false will trigger an error message to the user. The success message is handled by the method itself.
         * It can be left unimplemented if the module does not support any actions.
         * 
         * @param args The number of arguments in the web request.
         * @param argName A function that returns the name of the web argument given its index as string.
         * @param arg A function that returns the value of the web argument given its index as string.
         * @return True if the action was started successfully, false otherwise.
         */
        virtual bool startActionNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg) { return false; };
};

#endif
