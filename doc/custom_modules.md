# Developing Custom Modules for MVP3000

[Custom Modules](/examples/custom_module/).

## Create a Custom Module

### The Configuration Struct

The configuration struct includes two types of settings:
 *  Editable by the user, typically via the web interface. Those can easily be saved/loaded to/from SPIFF. Please note, the values set by the user override the initial values set in code.
 *  Fixed settings during compile time.

Example configuration struct with comments:

    struct CfgXmoduleExample : public Cfg {

        // Modifiable settings saved to SPIFF
        uint16_t editableNumber = 22222;

        // Fixed settings, restored with reboot to value set at compile  
        uint16_t fixedNumber = 10101;

        CfgXmoduleExample() {

            // The config name, used as SPIFF file name
            cfgName = "XmoduleExample";

            // Initialize settings for load/save to SPIFF:
            //  var name string, to allow web-based form input (stored as int though)
            //  pointer to actual variable
            //  set function with optional range checks
            addSetting(
                "editableNumber",
                &editableNumber,
                [&](uint16_t _x) { if (_x < 11111) return false; else editableNumber = _x; return true; }
            );
        }

    };

### The Module

The custom module needs to overwrite at least one method:
 *  loop() ... called repeatedly from the main loop of the program
Four additional methods are optional to overwrite:
 *  setup() ... called once during initialization
 *  contentModuleNetWeb() ... called if the user browses to the modules web interface, with module specific HTML content.
 *  editCfgNetWeb() ... called if the user saves a setting via the modules web interface.
 *  startActionNetWeb() ... called if the user initiates an action via the modules web interface.

Example module with additional comments:

    class XmoduleExample : public Xmodule {
        public:
            CfgXmoduleExample cfgXmoduleExample;

            void setup() override {
                description = "XmoduleExample";
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
                    <li>Some editable number:<br> <form action='/save' method='post'> <input name='editableNumber' value='%d' type='number' min='11112' max='65535'> <input type='submit' value='Save'> </form> </li> \
                    <li>Some fixed number: %d </li> </ul>",
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


## Integration with the Main Script

The custom module can easily be integrated with the MVP3000 framework.

    #include <MVP3000.h>
    extern MVP3000 mvp;

    #include "XmoduleExample.h"

    // Init sensor module
    XmoduleExample xmoduleExample;

    void setup() {
        // Add the custom module to the mvp framework
        mvp.addXmodule(&xmoduleExample);

        // Start mvp framework
        mvp.setup();
    }

    void loop() {
        // Do the work
        mvp.loop();

        // Execute custom module code

        // Do not ever use blocking delay in actual code
        delay(50);
    }