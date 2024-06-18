# Sensor Module

The sensor module receives sensor data from a user script and processes it. It is then output in CSV format via serial console and optionally published to a MQTT broker.

### General Workflow

Begin on your laptop:

1.  Get the sensor hardware running in a basic proof-of-concept script.
2.  Include the MVP3000 framework in your code and feed it with the sensor data.
3.  Modify some special settings before compile time.
4.  Go to the web interface to join an existing network and set measurement parameters such as averaging and measure offset and scaling. Those values are presinstant over reboots.
5.  Use the scripts provided in **[MVP3000 Evaluation](https://github.com/Production3000/mvp3000evaluation)** to view live data, record measurements and avaluate noise.

Gain independence from serial cables:

6.  Setup a local MQTT broker as described in the **[MVP3000 Controller](https://github.com/Production3000/mvp3000controller)** or use a public one.


## Getting Started

### Example Scripts

A good starting point are the [sensor examples](/examples/sensor/), particularly [basic.ino](/examples/sensor/basic/basic.ino).

 *  [basic.ino](/examples/sensor/basic/basic.ino): This script provides a general implementation of the sensor module to show its functionality. It sets basic options and feeds generated 'data' into the MVP3000 framework.
 *  [matrix.ino](/examples/sensor/matrix/matrix.ino): This script generates 'data' of a typical matrix-type sensor and feeds it into the sensor module.
 *  [noise.ino](/examples/sensor/noise/noise.ino): This script generates 'data' with more or less deterministic noise patterns. It can be used to better understand the output from a real sensor. 

### Initialization

The sensor module needs to be initialized with the correct number of values the sensor measures.

    const uint8_t valueCount = 2;
    XmoduleSensor xmoduleSensor(valueCount);

The object is then passed to the MVP framework.

    mvp.addXmodule(&xmoduleSensor);

### Sensor data

Sensor data is passed as an array to the framework. Its data type can be either `int32_t *` or `float_t *`.

    float_t data[valueCount] = { 12345, 54.321 };
    xmoduleSensor.addSample(data);

Data handling within the MVP3000 framework is done as integer. When working with floats/decimal raw data or when rounding after averaging it is important to maintain sufficient significent digits. This is done by shifting the decimal point of the raw value *x* with an exponent multiplier *n* to yield the stored value *x' = x * 10<sup>n</sup>*. This value is stored and averaged by the sensor module, and reported. Before data analysis the decimal shift needs to be reversed using the inverse exponent *-n* multiplier to yield the final value *x" = x' * 10<sup>-n</sup>*.

    int8_t exponent[valueCount] = {-1, 2};
    xmoduleSensor.setSampleToIntExponent(exponent);

The following tables shows the 'encoding' step in the sensor module.

|           | Raw value | Exponent  | Multiplicator | Reported int              |
| ---       | ---       | ---       | ---           | ---                       |
| default   | x         | n         | 10<sup>n</sup>| x' = x * 10<sup>n</sup>   |
| Sensor A  | 12345     | -1        | 0.1           | 1235                      |
| Sensor B  | 54.321    | 2         | 100           | 5432                      |

The Following table shows the decoding step before data analysis.

|           | Reported int  | Inv. Exp. | Multiplicator     | Reported int              | Sign. digits  |
| ---       | ---           | ---       | ---               | ---                       | ---           |
| default   | x             | n         | 10<sup>-n</sup>   | x" = x' * 10<sup>-n</sup> | as raw        |
| Sensor A  | 1235          | 1         | 10                | 12350                     | 4 \*          |
| Sensor B  | 5432          | -2        | 0.01              | 54.32                     | 4             |

\* Note the rounding in the 4th significant digit.


## Settings and Options

Some options can be set during compile time as well as via the web interface. Generally the values set via the web interface and saved to SPIFF supersede the value set during compile time! In that case the value in the code is the default value when the user did not change the value or after a reset.

### Settings Available via the Web Interface

The module web interface can be disabled as a whole during compile time.

    xmoduleSensor.enableContentModuleNetWeb = false;

#### Sample averaging

The number of data samples that should be averaged before the data is reported. Default value is 10 samples.

    // Set default value to 20 samples
    xmoduleSensor.cfgXmoduleSensor.sampleAveraging = 20;

#### Offset/Scaling averaging

The number of data samples that should be averaged when measuring offset or scaling. Default value is 25 samples.

    // Set default value to 100 samples
    xmoduleSensor.cfgXmoduleSensor.averagingOffsetScaling = 100;

#### Minimum reporting interval

Set the minimum wait time in ms before a new data point is reported. Any intermediate values measured are dropped. This can be used to limit data transfer rates from fast sensors or when detailed data is undesired.

    // Set wait interval to 10000 ms
    xmoduleSensor.cfgXmoduleSensor.reportingInterval = 10000

#### Offset/Scaling

Offset and scaling can be measured and reset via the web interface. If needed it could be set.

    xmoduleSensor.dataProcessing.offset.values = int32_t *[valueCount]
    xmoduleSensor.dataProcessing.offset.values = float_t *[valueCount]


### Options at Compile Time 

#### Sensor Description

A brief description of the sensor, its values and their units. 

    // Optional: Add a description of the sensor for the web interface
    String infoName = "PN123";
    String infoDescription = "The PN123 is a great dummy sensor for testing. It generates 'data' of a typical combi-sensor with vastly different ranges, for example temperature and relative humidity.";
    String sensorTypes[valueCount] = {"T", "rH"};
    String sensorUnits[valueCount] = {"°C", "%"};

    // Optional: Set the sensor descriptions
    xmoduleSensor.cfgXmoduleSensor.setSensorInfo(infoName, infoDescription, sensorTypes, sensorUnits);

#### Data is of Matrix-Type

Matrix column count is used for CSV output only. Together with the number of rows the column count gives the total value count.

    const uint8_t rows = 3;
    const uint8_t columns = 4;
    const uint8_t valueCount = rows * columns;

    xmoduleSensor.cfgXmoduleSensor.dataMatrixColumnCount = columns;

    -> a1,a2,a3,a4; b1,b2,b3,b4; c1,c2,c3,c4;
    -> a1,a2,a3,a4; b1,b2,b3,b4; c1,c2,c3,c4;
    [...]



## Troubleshooting

Q: My raw sensor values are fine, but the averaged values reported by the framework are zero/strange?  
A: Check offset and scaling via the web interface. Possibly factory reset the device.

Q: The values defined in the code are ignored for some settings?  
A: The value set via the web interface supersede the value set during compile time!


## License

Licensed under the Apache License. See `LICENSE` for more information.

Copyright Production 3000
