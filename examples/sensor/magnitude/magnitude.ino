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

// Adjust value count for the used sensor (e.g. temperature, rH)
const uint8_t valueCount = 2;

// Add a description of the sensor for the web interface
String infoName = "MAGNITUDE";
String infoDescription = "The MAGNITUDE is a dummy sensor for testing. It generates 'data' with values of vastly different orders of magnitude.";
String sensorTypes[valueCount] = {"rnd(10/20)", "-rnd(100/200)"};
String sensorUnits[valueCount] = {"a.u.", "a.u."};

// Local data variable
int32_t data[valueCount];

// Init sensor module
XmoduleSensor xmoduleSensor(valueCount);

// IMPORTANT: Do not ever use blocking delay() in the loop as it will impair web performance of the ESP and thus the framework.
LimitTimer timer(50);

void setup() {
    // Optional: Set the sensor descriptions
    xmoduleSensor.setSensorInfo(infoName, infoDescription, sensorTypes, sensorUnits);

    // Add the sensor module to the mvp framework
    mvp.addXmodule(&xmoduleSensor);

    // Start mvp framework
    mvp.setup();
}

void loop() {
    // Do the work
    mvp.loop();

    // Simulate a real-world sensor delay of 50 ms
    if (timer.justFinished()) {
        // Create random data: 10..20, -100..-200, 1000..2000, -10..-20, 100..200, -1000..-2000
        for (uint8_t i = 0; i < valueCount; i++) {
            int32_t mag = 10 * pow(10, i % 3);
            data[i] = (i % 2 == 0) ? random(mag, mag * 2) : random(-mag * 2, -mag);
        }

        // Add new data. The values are averaged by default, expected output is close to 15, -150, 1500, -15, ...
        xmoduleSensor.addSample(data);
    }
}
