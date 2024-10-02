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
    mvp.setAlternateRoot(altResponseFiller);

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


const char* altHtml = R"===(<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Custom Landing Page</title>
<style>body { font-family: sans-serif; }</style> </head>
<body> <h2>MVP3000 - Custom Landing Page</h2>
<p>This is a custom landing page. The MVP3000 settings were moved to <a href='/mvp3000'>/mvp3000</a>.
<p>&nbsp;</body></html>
)===";

size_t altResponseFiller(uint8_t *buffer, size_t maxLen, size_t index) {
    // Chunked response filler for the html template
    
    size_t len = strlen(altHtml);
    if (index + maxLen > len) {
        maxLen = len - index;
    }
    memcpy(buffer, altHtml + index, maxLen);
    return maxLen;
}