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
String infoName = "PULSE";
String infoDescription = "The PULSE is a dummy sensor for testing. It generates a) random pulsed 'data' and b) random stepping 'data'.";
String sensorTypes[valueCount] = {"Pulses", "Steps"};
String sensorUnits[valueCount] = {"a.u.", "a.u."};

// Local data variable
int32_t data[valueCount];

// Init sensor module
XmoduleSensor xmoduleSensor(valueCount);

// IMPORTANT: Do not ever use blocking delay() in the loop as it will impair web performance of the ESP and thus the framework.
LimitTimer timer(50);

// Pulse/step 'data' values
int16_t range = 10;
int16_t pulse = 100;
int16_t level = 10;

uint8_t counter = 0;


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
        // Pulse data
        if (counter < 10) {
            data[0] = random(pulse - range, pulse + range);
        } else {
            data[0] = random(-range, range);
        }
        
        // Step data
        if (counter == 0) {
            level = random(-pulse, pulse);
            mvp.log("New step/pulse");
        }
        data[1] = random(level - range, level + range);

        counter++;

        // Add new data. The values are averaged by default, which will arbitrarily overlap with the pulses/steps.
        xmoduleSensor.addSample(data);
    }
}
