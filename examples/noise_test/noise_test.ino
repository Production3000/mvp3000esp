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

// Generates 'sensor' data with more or less deterministic noise patterns

#include <random>
#include <MVP3000.h>
extern MVP3000 mvp;

// See https://cplusplus.com/reference/random/normal_distribution/
std::default_random_engine generator;
std::normal_distribution<double_t> distribution(50.0, 20.0); // mean value, standard deviation

// ATTENTION upper bound is exclusive
// random(3) = rndval % 3 and returns 0,1,2
// random(1, 4) = random(3) + 1 and returns 1,2,3

// Value count given by noise types
const uint8_t valueCount = 5;

int32_t data[valueCount];
CfgSensorHandler cfgSensorHandler = CfgSensorHandler(valueCount);
MVP3000CFG mvp3000cfg = MVP3000CFG(cfgSensorHandler);

uint32_t counter = 0;
boolean data2 = false;
uint8_t data4 = 0;

void setup() {
    // Turn off averaging on the ESP to not mess with the generated noise
    mvp3000cfg.cfgSensorHandler.setAveraging(1);

    // Init
    mvp.setup(mvp3000cfg);

    // Generates 'sensor' data with deterministic noise patterns
    // Alternating values
    data[0] = 10;
    // Sawtooth 21 frames
    data[1] = 19;
    // Zick zack 61 frames
    data2 = true;
    data[2] = 37;
    // Seeded random walk
    randomSeed(1234); // Switch to seeded software random
    data[3] = 0;
    // Gauss random
    data4 = 50;
    data[4] = 0;
}

void loop() {
    // Do stuff, handle new data
    mvp.loop();

    // Alternating values
    data[0] = -data[0];
    // Sawtooth 20 frames
    data[1] += (counter % 20 == 0) ? -38 : 2;
    // Zick zack 75 frames
    if (counter % 75 == 0)
        data2 = !data2;
    data[2] += (data2) ? 1 : -1;
    // Seeded random walk
    data[3] += random(15) - 7;
    // Gauss random
    data[4] = int32_t(distribution(generator) - data4);

    mvp.sensorHandler.addSample(data);

    counter++;

    // Do not ever use blocking delay in actual code
    delay(20);
}
