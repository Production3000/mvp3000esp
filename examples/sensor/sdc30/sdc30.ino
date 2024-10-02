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

// Sensirion I2C SCD30 https://github.com/Sensirion/arduino-i2c-scd30
// This is a very nice library that returns actually usefull error codes, which we mostly ignore here.
#include <SensirionI2cScd30.h>
#include <Wire.h>
SensirionI2cScd30 sdc30;
uint16_t sdcDataReady = 0;
int16_t sdcError;


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
    xmoduleSensor.setSampleAveraging(1); // The SCD30 has already averaged data ready every 1.5 s. Averaging in the framwwork can be set to a low value.
    xmoduleSensor.setSampleToIntExponent(exponent);
    mvp.addXmodule(&xmoduleSensor);

    // Start mvp framework
    mvp.setup();
    delay(2000);

    // I2C
    Wire.begin();

    // Init SCD30 sensor
    sdc30.begin(Wire, SCD30_I2C_ADDR_61);
    sdc30.stopPeriodicMeasurement();
    sdc30.softReset();
    delay(2000); // This is in the demo code, not sure it is needed and why
    
    sdcError = sdc30.startPeriodicMeasurement(0);
    if (sdcError != NO_ERROR) {
        static char errorMessage[128];
        errorToString(sdcError, errorMessage, sizeof errorMessage);
        mvp.logFormatted("Error trying to execute startPeriodicMeasurement(): %s", errorMessage);
    }
}

void loop() {
    sdc30.getDataReady(sdcDataReady);
    if (sdcDataReady) {
        sdcError = sdc30.readMeasurementData(data[0], data[1], data[2]);
        if (sdcError == NO_ERROR) {
            // Add data to the sensor module
            if (data[0] > 1) // First measurement is always 0 CO2, probalby some averaging thing
                xmoduleSensor.addSample(data);
        } else {
            mvp.logFormatted("Error trying to execute readMeasurementData()");
        }
    }

    // Do the work
    mvp.loop();
}
