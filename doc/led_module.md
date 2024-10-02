# Sensor Module

The sensor module receives sensor data from the user script and processes it. It handles averaging, offset, scaling, and Tare. It publishes the live data to serial, to WebSocket, and MQTT. Stored recent data can be downloaded as CSV.

## <a name='Contents'></a>Contents

<!-- vscode-markdown-toc -->
* [Contents](#Contents)
* [Getting Started](#GettingStarted)
	* [First Steps](#FirstSteps)
	* [Web Interface](#WebInterface)
* [Example Scripts](#ExampleScripts)
* [Implementation](#Implementation)
	* [General Functionality](#GeneralFunctionality)
	* [Methods and Options](#MethodsandOptions)
* [Troubleshooting](#Troubleshooting)
* [License](#License)

<!-- vscode-markdown-toc-config
	numbering=false
	autoSave=true
	/vscode-markdown-toc-config -->
<!-- /vscode-markdown-toc -->

## <a name='GettingStarted'></a>Getting Started



### <a name='FirstSteps'></a>First Steps



### <a name='WebInterface'></a>Web Interface

 *  Set brightness
 * 	Change the number of LED in the strip 


## <a name='ExampleScripts'></a>Example Scripts

Obviously the hardware needs to be available for testing.

 *  [effect.ino](/examples/led/effect/effect.ino): Demo script implementing a custom effect.
 *  [ondemand.ino](/examples/led/ondemand/ondemand.ino): Using the on demand functionality to display script status/data/events/...

## <a name='Implementation'></a>Implementation

Please also see the documentation of the framework regarding [custom implementation](/README.md#custom-implementation).

### <a name='GeneralFunctionality'></a>General Functionality



### <a name='MethodsandOptions'></a>Methods and Options

##### Selected Methods and Options

Use a photoresistor connected to the internal ADC to automatically adapt the LED brightness depending on ambient light conditions.

    uint8_t analogPin = A0
    xmoduleLed.adaptiveBrightness(analogPin);

##### Constructor

 *  `XmoduleLED(uint8_t ledPin, uint8_t ledCount)`: Construct a new LED Module object. 

##### Public Methods and Options

 *  `void adaptiveBrightness(uint8_t analogPin, uint8_t analogBits = 0)`: Use a photoresistor to automatically adapt the brightness of the LED strip.
 *  `void setBrightness(uint8_t brightness)`: Set the brightness of the LED strip. This turns adaptive brightness off.
 *  `void setLed(CallbackSyncSetter setOnceSyncSetter)`:
 *  `void setLed(CallbackSingleSetter setOnceSingleSetter)`:
 *  `void setLed(CallbackArraySetter setOnceArraySetter)`:
 *  `void demandLedUpdate()`: Demand an update of the LED strip. This is necessary if the LED display depends on the status of the script.
 *  `void setOnDemandSetter(CallbackArraySetter onDemandArraySetter)`:
 *  `void setOnDemandSetter(CallbackSingleSetter onDemandSingleSetter)`:
 *  `void setOnDemandSetter(CallbackSyncSetter onDemandSyncSetter)`:
 *  `void setEffect(uint8_t effect)`: Display a pre-defined effect.
 *  `setEffectSetter(FxSingleSetter fxCallback, uint16_t duration_ms, boolean onlyOnNewCycle = false)`: Set a custom effect. Each LED is set individually.
 *  `setEffectSetter(FxSyncSetter fxCallback, uint16_t duration_ms, boolean onlyOnNewCycle = false)`: Set a custom effect. All LED are synchronized to display the same color.


## <a name='Troubleshooting'></a>Troubleshooting

See also [Troubleshooting](/README.md#troubleshooting) of the whole framework.

Q: 
A: 


## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000
