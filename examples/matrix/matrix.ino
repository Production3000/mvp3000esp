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

// Generates 'sensor' data of a typical matrix-sensor with somewhat similar values for all dots

#include <MVP3000.h>
extern MVP3000 mvp;

// Adjust row and column counts as desired
const uint8_t rows = 3;
const uint8_t columns = 4;
const uint8_t valueCount = rows * columns;

int32_t data[valueCount];
CfgSensorHandler cfgSensorHandler = CfgSensorHandler(valueCount);
MVP3000CFG mvp3000cfg = MVP3000CFG(cfgSensorHandler);


void setup() {
    // Set matrix type output
    mvp3000cfg.cfgSensorHandler.setMatrixColumnCount(columns);

    // Init
    mvp.setup(mvp3000cfg);
}

void loop() {
    // Do stuff, handle new data
    mvp.loop();

    // Generates 'sensor' data of a typical matrix-sensor with somewhat similar values for all dots
    //  base 100 with random noise plus 10/20/30... depending on position in row, shifting with each row 
    for (uint8_t i = 0; i < valueCount; i++) {
        data[i] = 100 + random(50) + 10 * (i % (columns + 1));
    }

    // Add new data
    // The values are averaged
    mvp.sensorHandler.addSample(data);

    // Do not ever use blocking delay in actual code
    delay(20);
}
