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


#include <MVP3000.h>
extern MVP3000 mvp;

// For I2C devices
#include <Wire.h>
// https://github.com/sparkfun/SparkFun_SCD30_Arduino_Library
#include <SparkFun_SCD30_Arduino_Library.h>
SCD30 sdc30;

const uint8_t valueCount = 3;

// Add a description of the sensor for the web interface
String infoName = "Sensirion SDC30";
String infoDescription = "Sensirion SDC30 ambient CO2 sensor module, measures also temperature and relative humidity.";
String sensorTypes[valueCount] = {"CO2", "T", "rH"};
String sensorUnits[valueCount] = {"ppm", "0.1 &deg;C", "0.1 &percnt;"};

// Local data variable
float_t data[valueCount];

// Convert native sensor units to the desired units
//  ppm
//  °C -> 0.1 °C
//  % -> 0.1 %
int8_t exponent[valueCount] = {0, 1, 1};

// Init sensor module
XmoduleSensor xmoduleSensor(valueCount);

void setup() {
    // Init the sensor module and add it to the mvp framework
    xmoduleSensor.setSensorInfo(infoName, infoDescription, sensorTypes, sensorUnits);
    xmoduleSensor.setSampleToIntExponent(exponent);
    xmoduleSensor.setSampleAveraging(1); // Initial value to not require the user to set it on the web page
    mvp.addXmodule(&xmoduleSensor);

    // Start mvp framework
    mvp.setup();

    // I2C
    Wire.begin();

    // Init SCD30 sensor
    if (sdc30.begin() == false) {
        mvp.log("Sensor not detected. Please check wiring.");
    }
}

void loop() {
    //The SCD30 has data ready every two seconds. Averaging in the sensor module can be set to a low value.
    if (sdc30.dataAvailable()) {
        data[0] = (float_t)sdc30.getCO2();
        data[1] = sdc30.getTemperature();
        data[2] = sdc30.getHumidity();

        // Add data to the sensor module
        xmoduleSensor.addSample(data);
    }

    // Do the work
    mvp.loop();
}
