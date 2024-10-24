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

// IMPORTANT: Do not ever use blocking delay() in the loop as it will impair web performance of the ESP and thus the framework.
LimitTimer timer(2000);

void setup() {
    // Arduino IDE: Disable color-coded output to serial console
    // mvp.logDisableAnsiColor();

    // Enable the WebSocket log, open it using: examples/websocket/websocket_log.html
    mvp.logSetTarget(CfgLogger::OutputTarget::WEBLOG, true);

    // Start mvp framework
    mvp.setup();
}

void loop() {
    // Do not flood the log output
    if (timer.justFinished()) {
        mvp.log("This text will be printed every 2s to serial in purple and to the log-websocket.");
    }

    // Do the work
    mvp.loop();
}
