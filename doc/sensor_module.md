# Sensor Module

The sensor module receives sensor data from the user script and processes it. It handles averaging, offset, scaling, and Tare. It publishes the live data to serial, to WebSocket, and MQTT. Stored recent data can be downloaded as CSV.

## <a name='Contents'></a>Contents

<!-- vscode-markdown-toc -->
* [Contents](#Contents)
* [Getting Started](#GettingStarted)
	* [Example Scripts](#ExampleScripts)
	* [First steps](#Firststeps)
	* [Web Interface](#WebInterface)
	* [WebSocket and MQTT](#WebSocketandMQTT)
	* [Code and Options](#CodeandOptions)
* [Sensor Data Handling Details](#SensorDataHandlingDetails)
	* [Sample-to-Int Exponent](#Sample-to-IntExponent)
	* [Offset, Scaling, Tare](#OffsetScalingTare)
* [Troubleshooting](#Troubleshooting)
* [License](#License)

<!-- vscode-markdown-toc-config
	numbering=false
	autoSave=true
	/vscode-markdown-toc-config -->
<!-- /vscode-markdown-toc -->

## <a name='GettingStarted'></a>Getting Started

### <a name='ExampleScripts'></a>Example Scripts

The example sensor scripts generate random 'data' of common sensor types. They can be used for testing evaluation scripts and developing display options.

 *  [basic.ino](/examples/sensor/basic/basic.ino): A general implementation of the sensor module to show its functionality. It sets basic options and feeds generated 'data' into the MVP3000 framework.
 *  [matrix.ino](/examples/sensor/matrix/matrix.ino): Typical matrix-type sensor 'data'.
 *  [noise.ino](/examples/sensor/noise/noise.ino): The generated 'data' has more or less deterministic noise patterns. It can be used to better understand the noisy output from a real sensor. 

### <a name='Firststeps'></a>First steps

The [basic.ino](/examples/sensor/basic/basic.ino) example is a good choice to get started.

The sensor module is initialized with the number of values the sensor measures simultaneously/within one cycle and passed to the MVP framework during setup. In the main loop the sensor data is passed to the framework, as an array of numeric types such as `int32_t *` as in the following example.

    #include <MVP3000.h>
    extern MVP3000 mvp;

    const uint8_t valueCount = 2;
    XmoduleSensor xmoduleSensor(valueCount);

    int32_t data[valueCount];

    void setup() {
        // Your custom code
        mvp.addXmodule(&xmoduleSensor);
        mvp.setup();
    }

    void loop() {
        // Your custom code
        xmoduleSensor.addSample(data);
        mvp.loop();
    }

### <a name='WebInterface'></a>Web Interface

 *  ...


### <a name='WebSocketandMQTT'></a>WebSocket and MQTT

Use the [WebSocket sensor example](/examples/websocket/websocket_sensor.html) or the [MQTT sensor example](/examples/mqtt/mqtt_sensor.html) to display the sensor data in a browser or store it in a remote database. Both allow to set/clear [Tare](#OffsetScalingTare).

To use WebSockets obviously the receiving device needs to be able to reach the ESP device over the network.

To use MQTT obviously both the ESP device and the receiving device need to be able to access the MQTT broker. Make sure the correct URL/IP is set in the web interface of the ESP and in the browser. For testing purposes one can use the public *test.mosquitto.org* broker.

### <a name='CodeandOptions'></a>Code and Options

Add a description of the sensor and its measurement units for the web interface. Please note, some special characters need to be encoded: Degree ° is non-ASCII, use `&deg;` and the percent symbol % is used by the string parser, use `&percnt;`

    String infoName = "BASIC";
    String infoDescription = "The BASIC is a great dummy sensor for testing. It generates 'data' of a typical combi-sensor with vastly different ranges, for example temperature and relative humidity.";
    String sensorTypes[valueCount] = {"T", "rH"};
    String sensorUnits[valueCount] = {"&deg;C", "&percnt;"};

    xmoduleSensor.setSensorInfo(infoName, infoDescription, sensorTypes, sensorUnits);

In case of a array like matrix sensor, the columncount allows formatted CSV output: -> a1,a2,a3,a4; b1,b2,b3,b4; c1,c2,c3,c4; -> a1, [...].

    const uint8_t columns = 4;
    xmoduleSensor.setSensorInfo(infoName, infoDescription, "pixel", "a.u.", columns);

Shift the decimal point of the sample values by the given exponent, see section [Sample-to-Int Exponent](#Sample-to-IntExponent) for more information.

    int8_t exponent[valueCount] = {-1, 2};
    xmoduleSensor.setSampleToIntExponent(exponent);

The number of measurements stored is limited by the available memory on the ESP. Data collection can be set to adaptive mode, growing depending on available memory. If turned on this feature could lead to stability issues, depending on other operations and memory fragmentation.

    xmoduleSensor.setDataCollectionAdaptive();


## <a name='SensorDataHandlingDetails'></a>Sensor Data Handling Details

### <a name='Sample-to-IntExponent'></a>Sample-to-Int Exponent

Data handling within the MVP3000 framework is done as integer. When working with floats/decimal raw data or when rounding after averaging it is important to maintain sufficient significent digits. This is done by shifting the decimal point of the raw value *x* using an exponent *n* multiplier to yield the decimal-shifted value *x' = x \* 10<sup>n</sup>*. This value *x'* is stored, averaged, and scaled by the sensor module, and reported. The following table shows the 'encoding' step on the ESP.

|           | Raw value | Exponent  | Multiplicator | Reported int              |
| ---       | ---       | ---       | ---           | ---                       |
|           | x         | n         | 10<sup>n</sup>| x' = x * 10<sup>n</sup>   |
| Sensor A  | 12345     | -1        | 0.1           | 1235                      |
| Sensor B  | 54.321    | 2         | 100           | 5432                      |

In order to retrieve the original value for display or data analysis, the decimal shift of the reported value needs to be reversed. This is done using the inverse exponent *-n* multiplier to yield the decimal-un-shifted value *x" = x' \* 10<sup>-n</sup>*. The following table shows the 'decoding' step on the computer.

|           | Received int  | Inv. Exp. | Multiplicator     | Reported value            | Sign. digits  |
| ---       | ---           | ---       | ---               | ---                       | ---           |
|           | x             | n         | 10<sup>-n</sup>   | x" = x' * 10<sup>-n</sup> |               |
| Sensor A  | 1235          | 1         | 10                | 12350                     | 4 \*          |
| Sensor B  | 5432          | -2        | 0.01              | 54.32                     | 4             |

*\* Note the rounded value of the 4th significant digit.*

### <a name='OffsetScalingTare'></a>Offset, Scaling, Tare

Many sensors are already calibrated during fabrication. However, sensor response often shifts over time and can be quite different even for two identical sensors. Best practice:
 *  Regularly recalibrate all sensors.
 *  Calibrate the sensors at values close to the intended measurement range.
 *  Cover at least the whole intended measurement range, no extrapolation.

**Offset** defines the current measurement value as zero. It essentially shifts all values vertically by an equal distance. Typically this is the lower end of the intended measurement range.

**Scaling** stretches the current measurement to a given value. Typically this is the upper end of the intended measurement range.

**Tare** is a second offset, see [Wikipedia](https://en.wikipedia.org/wiki/Tare_weight). It offers a quick way to zero the current value, but without premanently changing the measured offset and scaling. It can be used to remove any small and slow drift of the sensor, for example caused by a change of environmental temperature.

[IMG]

NOTE (obvious): Scaling in the MVP3000 framework is done linearly. The data coming from the sensor needs to be of (more or less) linear nature. This is very often the case already. However sometimes a different slope is a better representation of the real world and used instead. One example are the *1/x* inverse conductance and resistivity. In this case the measurements need to be inverted/linearized before passing them to the framwork to use the scaling feature. 


## <a name='Troubleshooting'></a>Troubleshooting

Q: The raw sensor values are fine, but the averaged values reported by the framework are zero/strange?  
A: Check offset and scaling via the web interface. Possibly factory reset the device.

Q: Some settings in the code are ignored?  
A: The value set via the web interface supersede the value set during compile time. Possibly factory reset the device.


## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000
