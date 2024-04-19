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

// Simulates faulty 'sensor' to evalute error handling

#include <MVP3000.h>
extern MVP3000 mvp;

// Adjust value count as desired
const uint8_t valueCount = 2;

int32_t data[valueCount];
CfgSensorHandler cfgSensorHandler = CfgSensorHandler(valueCount);
MVP3000CFG mvp3000cfg = MVP3000CFG(cfgSensorHandler);

void setup() {
    // Turn on/off averaging on the ESP to not mess with the generated noise
    mvp3000cfg.cfgSensorHandler.setAveraging(1);              

    // Init
    mvp.setup(mvp3000cfg);
}

void loop() {
    // Do stuff, handle new data
    mvp.loop();

    if (faultySensor(data)) {
        // The values are averaged
        mvp.sensorHandler.addSample(data);
    } else {
        mvp.log("Sensor fail");
    }
    
    // Do not ever use blocking delay in actual code
    delay(50);
}


bool faultySensor(int32_t *data) {
    // Random complete sensor fail in 0.2% 
    if (random(499) == 0)
        return false;

    // Create random data: +10..20, -100..200, +1000..2000, -10..20, ...
    for (uint8_t i = 0; i < valueCount; i++) {
        int32_t mag = 10 * pow10(i % 3);
        data[i] = (i % 2 == 0) ? random(mag, mag * 2) : random(-mag * 2, -mag);
    }

    // Random single value fail in 1%
    // If not handled this adds noise when averaging
    if (random(99) == 0) {
        data[random(0, valueCount - 1)] = 0;
        // data[random(0, valueCount - 1)] = NULL;
        // return false
    }

    // Random additional delay to introduce jitter
    delay(random(40));
    return true;
}
