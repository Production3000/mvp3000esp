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
#include <SensirionI2cScd30.h>
#include <Wire.h>
SensirionI2cScd30 sdc30;

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

    xmoduleSensor.setNetworkCtrlCallback(networkCtrlCallback);

    // Start mvp framework
    mvp.setup();

    // I2C
    Wire.begin();

    // Init SCD30 sensor
    sdc30.begin(Wire, SCD30_I2C_ADDR_61);
    sdc30.stopPeriodicMeasurement();
    int16_t sdcError = sdc30.softReset(); // Blocking 2000 ms
    if (sdcError != NO_ERROR) {
        static char errorMessage[128];
        errorToString(sdcError, errorMessage, sizeof errorMessage); // not sure if this works for the combined error codes
        mvp.logFormatted("Error starting sensor: %s", errorMessage);
    }

    // Get and print sensor information
    uint8_t major = 0;
    uint8_t minor = 0;
    sdc30.readFirmwareVersion(major, minor);
    uint16_t altitude;
    sdc30.getAltitudeCompensation(altitude);
    uint16_t temperatureOffset;
    sdc30.getTemperatureOffset(temperatureOffset);
    uint16_t co2RefConcentration;
    sdc30.getForceRecalibrationStatus(co2RefConcentration);
    uint16_t isActive;
    sdc30.getAutoCalibrationStatus(isActive);
    mvp.logFormatted("SDC30: Firmware version %d.%d, altitude %d m, temp offset %d C, sensor CO2 ambient %d ppm, auto calibration %s", major, minor, altitude, temperatureOffset, co2RefConcentration, (isActive) ? "active" : "disabled");

    // Start periodic measurement
    sdc30.startPeriodicMeasurement(0);
}

void loop() {
    uint16_t sdcDataReady;
    sdc30.getDataReady(sdcDataReady);
    if (sdcDataReady) {
        int16_t sdcError = sdc30.readMeasurementData(data[0], data[1], data[2]);
        if (sdcError == NO_ERROR) {
            // Add data to the sensor module
            if (data[0] > 1) // First CO2 measurement is always 0, probably some running median thing
                xmoduleSensor.addSample(data);
        } else {
            mvp.logFormatted("Error trying to execute readMeasurementData()");
        }
    }

    // Do the work
    mvp.loop();
}


boolean networkCtrlCallback(char* data) {
    if (strcmp(data, "CALIBRATE") == 0) {
        calibrateSensor();
        return true;
    }
    return false;
}

void calibrateSensor() {
    // Following the forced re-calibration (FRC) as described in the datasheet white paper.
    mvp.log("The device needs to be OPERATING at FRESH air for AT LEAST 5 minutes before calibration!");

    // Datasheet states 2 minutes. 
    if (millis() < 5*60*1000) {
        mvp.log("Calibration failed. Uptime less than 5 minutes.");
        return;
    }

    // Set calibration, this offsets essentially the current measurement
    sdc30.forceRecalibration(420);

    mvp.log("Calibration successfull. Restarting measurements");
    // Restart sensor
    sdc30.stopPeriodicMeasurement();
    sdc30.softReset(); // Blocking 2000 ms
    sdc30.startPeriodicMeasurement(0);
}
