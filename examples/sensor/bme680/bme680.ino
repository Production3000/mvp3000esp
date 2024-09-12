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

#include <Adafruit_BME680.h>
Adafruit_BME680 bme680; // I2C
// WARNING: BME680 is blocking when used directly with performReading(), which will delay the loop execution

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
    // Init BME680 sensor
    bme680.begin();
    // Start the asyncronous reading
    bme680.beginReading();

    // Init the sensor module and add it to the mvp framework
    xmoduleSensor.setSensorInfo(infoName, infoDescription, sensorTypes, sensorUnits);
    xmoduleSensor.setSampleToIntExponent(exponent);
    mvp.addXmodule(&xmoduleSensor);
    
    // Start mvp framework
    mvp.setup();
}

void loop() {
    // Check if the sensor read-out is complete
    if (bme680.remainingReadingMillis() == 0) {
        bme680.endReading(); // will not block now

        data[0] = bme680.temperature;
        data[1] = bme680.humidity;
        data[2] = bme680.pressure;
        data[3] = bme680.gas_resistance;
        // bme.readAltitude(SEALEVELPRESSURE_HPA) // The absolute altitude is way off, it can be used for relative altitude changes only
        xmoduleSensor.addSample(data);

        // Start the next asyncronous reading
        bme680.beginReading();
    }

    // Do the work
    mvp.loop();
}
