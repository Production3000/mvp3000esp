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

Output is calculated for color and brightness separately.

The global brightness effectively sets a maximum brightness any LED can have.

### <a name='FirstSteps'></a>First Steps



### <a name='WebInterface'></a>Web Interface

 *  Set brightness
 * 	Change the number of LED in the strip 


## <a name='ExampleScripts'></a>Example Scripts

Obviously the hardware needs to be available for testing.

 *  [effect.ino](/examples/led/effect/effect.ino): Demo script implementing a custom effect.
 *  [ondemand.ino](/examples/led/ondemand/ondemand.ino): Using fixed colors to indicate status/data/events/...

## <a name='Implementation'></a>Implementation

Please also see the documentation of the framework regarding [custom implementation](/README.md#custom-implementation).

### <a name='GeneralFunctionality'></a>General Functionality



### <a name='MethodsandOptions'></a>Methods and Options

##### Selected Methods and Options

Use a photoresistor connected to the internal ADC to automatically adapt the LED brightness depending on ambient light conditions.

    uint8_t analogPin = A0
    xmoduleLed.adaptiveGlobalBrightness(analogPin);

##### Constructor

 *  `XmoduleLED(uint8_t ledPin, uint8_t ledCount)`: Construct a new LED Module object. 

##### Public Methods and Options

Brightness:

 *  `void setBrightnessEffect(uint16_t duration_ms, XledFx::BRIGHTNESSFX effect)`: Select a predefined brightness effect for the LED strip.
 *  `void setBrightnessEffect(uint16_t duration_ms, boolean useFrames, boolean runEndless, FxBrightnessSetter brightnessSetter)`: Set a custom brightness effect for the LED strip.

 *  `void setFixedBrightnessIndividual(uint8_t* brightness)`: Set the brightness of each LED individually.
 *  `void setFixedBrightnessSync(uint8_t brightness)`: Set the brightness of all LEDs to the same value.

Colors:

 *  `void setColorEffect(uint16_t duration_ms, XledFx::COLORFX effect)`: Select a predefined color effect for the LED strip.
 *  `void setColorEffect(uint16_t duration_ms, boolean useFrames, boolean runEndless, boolean colorWheel, FxColorSetter colorSetter)`: Set a custom color effect for the LED strip.

 *  `void setFixedColorIndividual(uint32_t* colors)`: Set the color of each LED individually.
 *  `void setFixedColorSync(uint32_t color)`: Set the color of all LEDs to the same value.
 *  `void setFixedColorRandom()`: Set the color of each LED individually to a random color.

Global brightness maximum:

 *  `void adaptiveGlobalBrightness(uint8_t analogPin, uint8_t analogBits = 0)`: Use a photoresistor to automatically adapt the global brightness of the LED strip. This overrides the global brightness setting.
 *  `void fixedGlobalBrightness(uint8_t brightness)`: Set the global brightness of the LED strip. This turns adaptive global brightness off.


## <a name='Troubleshooting'></a>Troubleshooting

See also [Troubleshooting](/README.md#troubleshooting) of the whole framework.

Q: 
A: 


## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000
