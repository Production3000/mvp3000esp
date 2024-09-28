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

#include "MVP3000.h"

#include "_Helper.h"
extern _Helper _helper;


MVP3000 mvp;


void MVP3000::setup() {
    // Start logging first obviously
    logger.setup();
    // Prepare flash to allow loading of saved configs
    config.setup();
    led.setup();

    net.setup();

    // Register actions
    net.netWeb.registerAction("restart", [&](int args, WebArgKeyValue argKey, WebArgKeyValue argValue) {
        delayedRestart(25); // Restarts after 25 ms
        return true;
    });
    net.netWeb.registerAction("reset", [&](int args, WebArgKeyValue argKey, WebArgKeyValue argValue) {
        config.asyncFactoryResetDevice((args == 3) && (argKey(2) == "keepwifi")); // If keepwifi is checked it is present in the args, otherwise not
        return true;
    }, "Factory reset initiated, this takes some 10 s ...");

    // Modules
    for (uint8_t i = 0; i < moduleCount; i++) {
        xmodules[i]->setup();
    }
}

void MVP3000::loop() {
    updateLoopDuration();
    checkStatus();

    config.loop();
    led.loop();
    net.loop();
    // Modules
    for (uint8_t i = 0; i < moduleCount; i++) {
        xmodules[i]->loop();
    }

    // Check if delayed restart was set
    if (delayedRestart_ms > 0) {
        if (millis() > delayedRestart_ms) {
            // delayedRestart_ms = 0; // Not needed as we reset the ESP
            _helper.ESPX->reset();
        }
    }
}

void MVP3000::addXmodule(_Xmodule *xmodule) {
    if (moduleCount >= MAX_MODULES) {
        return;
    }

    xmodules[moduleCount] = xmodule;
    // modules[moduleCount]->setup();
    moduleCount++;
}


void MVP3000::checkStatus() {
    // Never leave error state
    if (state == STATE_TYPE::ERROR)
        return;

    // Error was logged
    if (logger.errorReported) {
        state = STATE_TYPE::ERROR;
        return;
    }

    if ((net.netState == Net::NET_STATE_TYPE::CLIENT) || (net.netState == Net::NET_STATE_TYPE::AP))
        state = STATE_TYPE::GOOD;
    else
        state = STATE_TYPE::INIT;
}

void MVP3000::updateLoopDuration() {
    // Only start measuring loop duration after wifi is up, as this adds a single long duration and messes with max value
    if (state != STATE_TYPE::GOOD)
        return;

    // Skip first loop iteration, nothing to calculate
    if (loopLast_ms > 0) {
        // Current loop duration
        uint16_t loopDuration = millis() - loopLast_ms;

        // Update min and max loop duration
        loopDurationMax_ms = max(loopDurationMax_ms, loopDuration);
        loopDurationMin_ms = min(loopDurationMin_ms, loopDuration); // Is often 0, there are lots of loops where nothing happens

        // Calculate mean loop duration
        if (loopDurationMean_ms == 0)
            // Second loop iteration, kickstart averaging
            loopDurationMean_ms = loopDuration;
        else
            // Third and higher loop iteration, rolling average latest ten values
            loopDurationMean_ms = round((float_t)9/10 * loopDurationMean_ms + (float_t)1/10 * loopDuration);
    }

    // Remember this loop time
    loopLast_ms = millis();
}


///////////////////////////////////////////////////////////////////////////////////

String MVP3000::templateProcessor(uint8_t var) {
    switch (var) {
        case 11:
            return __DATE__ " " __TIME__; // Timestamp concatenated at compilation time
        case 12:
            return _helper.printFormatted("%d / %d", ESP.getFreeHeap(), _helper.ESPX->getHeapFragmentation());
        case 13:
            return _helper.uptimeString();
        case 14:
            return _helper.ESPX->getResetReason();
        case 15:
            return String(ESP.getCpuFreqMHz());
        case 16:
            return _helper.printFormatted("%d / %d / %d", loopDurationMean_ms, loopDurationMin_ms, loopDurationMax_ms);
        case 17:
            return _helper.timeString();

        case 18:
            return (net.netCom.isHardDisabled()) ? "UDP discovery (disabled)" : "<a href='/netcom'>UDP discovery</a>";

        case 20: // Not so long, could be one string
            if (moduleCount == 0)
                return "<li>None</li>";
            webPageProcessorCount = 0;
        case 21:
            webPageProcessorCount++;
            return _helper.printFormatted("<li>%s</li>%s",
                ((xmodules[webPageProcessorCount - 1]->uri).length() > 0) ?
                    _helper.printFormatted("<a href='%s'>%s</a>",
                        xmodules[webPageProcessorCount - 1]->uri.c_str(),
                        xmodules[webPageProcessorCount - 1]->description.c_str()).c_str() :
                    xmodules[webPageProcessorCount - 1]->description.c_str(),
                (webPageProcessorCount < moduleCount) ? "%21%" : "");

        default:
            return "";
    }
}
