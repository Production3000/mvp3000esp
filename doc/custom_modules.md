# Developing Custom Modules for MVP3000

[Custom Modules](/examples/custom_module/).

## Create a Custom Module

### The Configuration Struct

The configuration struct includes two types of settings:
 *  Editable by the user, typically via the web interface. Those can easily be saved/loaded to/from SPIFF. Please note, the values set by the user override the initial values set in code.
 *  Fixed settings during compile time.

Example configuration struct with comments:

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

### The Module

The custom module needs to overwrite at least the setup() method:
 *  setup() ... called once during initialization, loads 
 *  loop() ... called repeatedly from the main loop of the program

The most important part for integration of the module in the MVP3000 framwork is the definition of the modules web interface and available actions.

    class XmoduleExample : public Xmodule {
        public:
            CfgXmoduleExample cfgXmoduleExample;

            void setup() override {
                description = "XmoduleExample";
                uri = "/example";

                // Read config from SPIFF
                mvp.config.readCfg(cfgXmoduleExample);

                // Define the module's web interface
                mvp.net.netWeb.registerPage(uri,  R"===(
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
                mvp.net.netWeb.registerAction("someAction", [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
                    // argValue(0) is the action name
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

        // Execute custom code

        // Do not ever use blocking delay in actual code
        delay(50);
    }