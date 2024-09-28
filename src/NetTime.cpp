#include "NetTime.h"
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

#include "NetTime.h"

#include "MVP3000.h"
extern MVP3000 mvp;

const uint32_t asd = 15000;



void NetTime::setup() {

    // TODO every hour?
    // no network
    // first ok then no network?



    // This is called when SNTP has set the time
    settimeofday_cb([&]() {
        mvp.logger.write(CfgLogger::Level::INFO, "Time set by SNTP");
        if (millisAtTimeinfo == 0) {
            millisAtFirstTimeinfo = millis();
            timeAtFirstTimeinfo = time(nullptr);
        } else {
            // Time was already set, calculate the difference between local clock and NTP
            // Sadly we can only detect full seconds. This means small sync errors can add up to multiple seconds without notice here
            uint8_t diff = (millis() - millisAtTimeinfo) / 1000 - (time(nullptr) - timeAtTimeinfo); // + means local is ahead
            if (diff != 0)
                mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Resync local clock to NTP: %s%d s (local was %s)", (diff > 0) ? "+" : "", diff, (diff > 0) ? "ahead" : "behind");
            // This means small sync errors can add up to multiple seconds without notice here.
            totalDiff += (millis() - millisAtFirstTimeinfo) / 1000 - (time(nullptr) - timeAtFirstTimeinfo);
            if (totalDiff != 0)
                mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Total clock error since NTP start: %s%d s (local was %s)", (totalDiff > 0) ? "+" : "", totalDiff, (totalDiff > 0) ? "ahead" : "behind");
        }
        millisAtTimeinfo = millis();
        timeAtTimeinfo = time(nullptr);
    });
}
