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

 *  


## <a name='ExampleScripts'></a>Example Scripts

Obviously the hardware needs to be available for testing.

 *  [effect.ino](/examples/led/effect/effect.ino): Simple custom effect script.
 *  [ondemand.ino](/examples/led/ondemand/ondemand.ino): Implementation of using the LEDs as indication of status based on script status/data/events/...

## <a name='Implementation'></a>Implementation

Please also see the documentation of the framework regarding [custom implementation](/README.md#custom-implementation).

### <a name='GeneralFunctionality'></a>General Functionality



### <a name='MethodsandOptions'></a>Methods and Options

##### Selected Methods and Options

Using a photosensor Brigtness can be adaptided 

    uint8_t analogPin = A0
    xmoduleLed.adaptiveBrightness(analogPin);

##### Constructor

 *  `XmoduleLED(uint8_t )`: Construct a new LED Module object.

##### Public Methods and Options

 *  `void (T *newSample)`:




## <a name='Troubleshooting'></a>Troubleshooting

See also [Troubleshooting](/README.md#troubleshooting) of the whole framework.

Q: 
A: 


## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000
