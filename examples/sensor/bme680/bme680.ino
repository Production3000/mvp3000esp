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

// BME68x Sensor library https://github.com/boschsensortec/Bosch-BME68x-Library
#include <Wire.h>
#include <bme68xLibrary.h>
#define MEAS_DUR 140
#define NEW_GAS_MEAS (BME68X_GASM_VALID_MSK | BME68X_HEAT_STAB_MSK | BME68X_NEW_DATA_MSK)
Bme68x bme;

const uint8_t valueCount = 4;

// Add a description of the sensor for the web interface
String infoName = "BME680";
String infoDescription = "Environmental sensor measuring temperature, humidity, pressure, and air resistance.";
String sensorTypes[valueCount] = {"T", "rH", "P", "Gas R"};
String sensorUnits[valueCount] = {"0.1 &deg;C", "0.1 &percnt;", "hPa", "Ohm"};

// Local data variable
float_t data[valueCount];

// Convert native sensor units to the desired units
//  °C -> 0.1 °C
//  % -> 0.1 %
//  Pa -> hPa
//  Ohm
int8_t exponent[valueCount] = {1, 1, -2, 0};

// Init sensor module
XmoduleSensor xmoduleSensor(valueCount);

void setup() {
    // Init the sensor module and add it to the mvp framework
    xmoduleSensor.setSensorInfo(infoName, infoDescription, sensorTypes, sensorUnits);
    xmoduleSensor.setSampleToIntExponent(exponent);
    mvp.addXmodule(&xmoduleSensor);

    // Start mvp framework
    mvp.setup();

    // I2C
    Wire.begin();

    // Init BME680 sensor
    bme.begin(BME68X_I2C_ADDR_HIGH, Wire);
    if(bme.checkStatus())
        mvp.logFormatted("Sensor %s: %s", (bme.checkStatus() == BME68X_ERROR)? "error" : "warning", bme.statusString().c_str());

    // The following is taken from the example code of the library
	/* Set the default configuration for temperature, pressure and humidity */
	bme.setTPH();

	/* Heater temperature in degree Celsius */
	uint16_t tempProf[10] = { 320, 100, 100, 100, 200, 200, 200, 320, 320, 320 };
	/* Multiplier to the shared heater duration */
	uint16_t mulProf[10] = { 5, 2, 10, 30, 5, 5, 5, 5, 5, 5 };
	/* Shared heating duration in milliseconds */
	uint16_t sharedHeatrDur = MEAS_DUR - (bme.getMeasDur(BME68X_PARALLEL_MODE) / 1000);

	bme.setHeaterProf(tempProf, mulProf, sharedHeatrDur, 10);
	bme.setOpMode(BME68X_PARALLEL_MODE);
}

void loop() {
    // Check if the sensor read-out is complete
	if (bme.fetchData()) {
        bme68xData bmeData;
        uint8_t nFieldsLeft = 1;
        while (nFieldsLeft) {
			nFieldsLeft = bme.getData(bmeData);
			if (bmeData.status == NEW_GAS_MEAS) {
                data[0] = bmeData.temperature;
                data[1] = bmeData.humidity;
                data[2] = bmeData.pressure;
                data[3] = bmeData.gas_resistance;
                // Add data to the sensor module
                xmoduleSensor.addSample(data);
			}
		}
	}

    // Do the work
    mvp.loop();
}
