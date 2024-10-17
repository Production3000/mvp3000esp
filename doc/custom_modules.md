# Developing Custom Modules for MVP3000

Most users will not need to create a custom module, particularly if a similar one already exists. We are happy to help implementing a missing feature.

<!-- vscode-markdown-toc -->
* [Example Custom Module](#ExampleCustomModule)
	* [The Configuration Struct](#TheConfigurationStruct)
	* [The Module](#TheModule)
	* [In Setup()](#InSetup)
	* [Contribute](#Contribute)
* [License](#License)

<!-- vscode-markdown-toc-config
	numbering=false
	autoSave=true
	/vscode-markdown-toc-config -->
<!-- /vscode-markdown-toc -->


## <a name='ExampleCustomModule'></a>Example Custom Module

Please refer to the simplified custom module example [class](/examples/custom_module/custom_module.h) when reading the following sections. Main parts of the code are explained there. Its [implementation](/examples/custom_module/custom_module.ino) is ready to compile.

### <a name='TheConfigurationStruct'></a>The Configuration Struct

The configuration struct includes two types of settings:
 *  Editable by the user, typically via the web interface. Those can easily be saved/loaded to/from SPIFFS.
 *  Fixed settings during compile time.

Please note, the values set by the user and saved to SPIFFS override the initial values set in code.

### <a name='TheModule'></a>The Module

The constructor defines the module name and the uri for its web interface. Leave the uri blank to disable the web interface.

    XmoduleExample() : _Xmodule("Xmodule Example", "/example") { };

The custom module needs to overwrite the setup and loop methods:
 *  `void setup() override` : called once during initialization
 *  `void loop() override` : called repeatedly from the main loop of the program

In order to provide a web interface define:
 *  `const char* getWebPage() override { return R"..." }` : String containing the HTML template with placeholders.
 *  `String webPageProcessor(uint8_t var) override` : Function to return the placeholder value.


 *  The percent symbol % is used as deliminator by the string parser in the ESPAsyncWebServer library. This can only be changed by providing an alternate definition with the compiler args - which is not feasible.
 * Just us %%, while this brakes CSS :/

### <a name='InSetup'></a>In Setup()

    // Read config from SPIFFS
    mvp.config.readCfg(...);

    // Register config
    mvp.net.netWeb.registerCfg(...);

    // Register action
    mvp.net.netWeb.registerAction(...);

    // Register WebSocket
    mvp.net.netWeb.webSockets.registerWebSocket(...);

    // Register MQTT
    mvp.net.netMqtt.registerMqtt(...);

    // Register Filler Page, for data
    mvp.net.netWeb.registerFillerPage(...);


### <a name='Contribute'></a>Contribute

We are looking forward to your input on this project, be it bug reports, feature requests, or a successful implementation that does something cool. In the latter case we will gladly link you.

Please follow the [GitHub guide](https://docs.github.com/en/get-started/exploring-projects-on-github/contributing-to-a-project) on how to contribute code to this project.


## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000