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

// https://github.com/bogde/HX711
#include <HX711.h>
HX711 hx711;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

// Multiple HX711 load cell amplifier
const uint8_t valueCount = 1;
// Add a description of the sensor for the web interface
String infoName = "HX711 load cell";
String infoDescription = "Load cell weight sensor using the HX711 24bit ADC amplifier.";
String sensorTypes[valueCount] = {"Weight"};
String sensorUnits[valueCount] = {"0.1 g"};

int32_t data[valueCount];

// Convert native sensor units to the desired units
//  g -> 0.1 g
// int8_t exponent[valueCount] = {1};

// Init sensor module
XmoduleSensor xmoduleSensor(valueCount);

void setup() {
    // Init BME680 sensor
    hx711.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

    // Init the sensor module and add it to the mvp framework
    xmoduleSensor.setSensorInfo(infoName, infoDescription, sensorTypes, sensorUnits);
    // xmoduleSensor.setSampleToIntExponent(exponent);
    mvp.addXmodule(&xmoduleSensor);

    // Start mvp framework
    mvp.setup();
}

void loop() {
    // Measurement and Processing
    if (hx711.is_ready()) {
        // Add new data to the sensor module
        data[0] = hx711.read();
        xmoduleSensor.addSample(data);
    }

    // Do the work
    mvp.loop();
}
