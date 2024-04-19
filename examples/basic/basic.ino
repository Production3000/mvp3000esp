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

// Generates 'sensor' data of a typical combi-sensor with vastly different ranges (e.g. temperature, rH, pressure, gas quality)

#include <MVP3000.h>
extern MVP3000 mvp;

// Adjust value count as desired
const uint8_t valueCount = 2;

int32_t data[valueCount];
CfgSensorHandler cfgSensorHandler = CfgSensorHandler(valueCount);
MVP3000CFG mvp3000cfg = MVP3000CFG(cfgSensorHandler);


void setup() {
    // Init
    mvp.setup(mvp3000cfg);
}

void loop() {
    // Do stuff, handle new data
    mvp.loop();

    // Create random data: +10..20, -100..200, +1000..2000, -10..20, ...
    for (uint8_t i = 0; i < valueCount; i++) {
        int32_t mag = 10 * pow10(i % 3);
        data[i] = (i % 2 == 0) ? random(mag, mag * 2) : random(-mag * 2, -mag);
    }

    // Add new data
    // The values are averaged, expected output is close to 15, -150, 1500, -15, ... 
    mvp.sensorHandler.addSample(data);

    // Do not ever use blocking delay in actual code
    delay(50);
}
