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


uint32_t sntp_update_delay_MS_rfc_not_less_than_15000 () { return 15*1000; } // 1 hour update interval       60*60             // ESP8266 only?
// sntp_set_sync_interval(12 * 60 * 60 * 1000UL); // 12 hours ESP32?

void NetTime::setup() {

    // TODO option to disable NTP after first time set ?


    // This is called when SNTP has set the time
    settimeofday_cb([&]() {
        mvp.logger.write(CfgLogger::Level::INFO, "Time set by SNTP");
        if (millisAtTimeinfo == 0) {
            millisAtFirstTimeinfo = millis();
            timeAtFirstTimeinfo = time(nullptr);
        } else {
            // Time was already set, calculate the difference between local clock and NTP. Sadly there is only full seconds.
            // + means local is ahead
            int32_t diff = (millis() - millisAtTimeinfo) / 1000 - (time(nullptr) - timeAtTimeinfo);
            // This means small sync errors can add up to multiple seconds, which is caught here.
            totalDiff = (millis() - millisAtFirstTimeinfo) / 1000 - (time(nullptr) - timeAtFirstTimeinfo);
            if (diff != 0) {
                mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Resync local clock to NTP: %s%d s, total since NTP start: %s%d s (+ means local is ahead)", (diff > 0) ? "+" : "", diff, (totalDiff > 0) ? "+" : "", totalDiff);
            }
        }
        millisAtTimeinfo = millis();
        timeAtTimeinfo = time(nullptr);
    });
}
