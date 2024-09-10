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


void setup() {
    // Arduino IDE: Disable color-coded output to serial console
    // mvp.logAnsiColor(false);
    
    // Start mvp framework
    mvp.setup();
}

uint64_t lastMillis = 0;

void loop() {
    // Do the work
    mvp.loop();

    // IMPORTANT: Do not ever use blocking delay in actual code as it will impair performance of the framework, web interface and other modules
    if (millis() - lastMillis > 2000) {
        lastMillis = millis();
        mvp.log("This text will be printed in purple to Serial and the log-websocket.");
    }
}
