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



## Integration with the Main Script

The custom module can easily be integrated with the MVP3000 framework.

    mvp.addXmodule(&xmoduleExample);



## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000