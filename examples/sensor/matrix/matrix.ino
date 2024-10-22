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
#include <XmoduleSensor/XmoduleSensor.h>
extern MVP3000 mvp;

// Adjust row and column counts as desired
const uint8_t rows = 3;
const uint8_t columns = 4;
const uint8_t valueCount = rows * columns;

// Add a description of the sensor for the web interface, types and units are omitted
String infoName = "MATRIX";
String infoDescription = "The MATRIX is a dummy sensor for testing. It generates 'data' of a typical matrix-sensor with somewhat similar values for all pixels.";
String pixelType = "pixel";
String pixelUnit = "counts";

// Local data variable
int32_t data[valueCount];

// Init sensor module
XmoduleSensor xmoduleSensor(valueCount);

void setup() {
    // Set the sensor descriptions, matrix column count is used for CSV output: a1,a2,a3,a4;b1,b2,b3,b4;c1 ...
    xmoduleSensor.setSensorInfo(infoName, infoDescription, pixelType, pixelUnit, columns);

    // Add the sensor module to the mvp framework
    mvp.addXmodule(&xmoduleSensor);

    // Start mvp framework
    mvp.setup();
}

void loop() {
    // Do the work
    mvp.loop();

    if (fakeSensorReady()) {
        // Generates 'data' of a typical matrix-sensor with somewhat similar values for all dots
        //  base 100 with random noise plus 10/20/30... depending on position in row, shifting with each row
        for (uint8_t i = 0; i < valueCount; i++) {
            data[i] = 100 + random(50) + 10 * (i % (columns + 1));
        }

        // Add new data. The values are averaged by default, expected output is close to 15, -150, 1500, -15, ...
        xmoduleSensor.addSample(data);
    }

    // IMPORTANT: Do not ever use blocking delay() in actual code
}


uint32_t nextMeasurement_ms = 0;
uint32_t measurementInterval_ms = 50;
bool fakeSensorReady() {
    // This simulates sensor readout delay. For a real sensor another option might be to just increase averaging count.
    if (millis() > nextMeasurement_ms) {
        nextMeasurement_ms = millis() + measurementInterval_ms;
        return true;
    }
    return false;
}