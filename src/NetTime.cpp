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


#if defined(ESP8266)
    uint32_t sntp_update_delay_MS_rfc_not_less_than_15000 () { return 60*60*1000; } // 1 hour update interval
#else 
    #include "esp_sntp.h"                        // ESP32 only
    void cbSyncTime32(struct timeval *tv)  {
        mvp.net.netTime.cbSyncTime(); // timeval is with microseconds
    }
#endif


void NetTime::setup() {
    #if defined(ESP8266)
        settimeofday_cb([&]() { cbSyncTime(); });
    #else 
        sntp_set_sync_interval(60*60*1000); // 1 hour update interval
        sntp_set_time_sync_notification_cb(cbSyncTime32);
    #endif
}

void NetTime::loop() {
    if (ntpRequested) 
        return;

    if (!mvp.net.connectedAsClient())
        return;

    if (waitBeforeRequest == 0) {
        waitBeforeRequest = millis() + 500; // At least on ESP8266 there is a pause on conenct, so maybe wait a little
    }
    
    if (millis() > waitBeforeRequest) {
        // Query NTP server
        ntpRequested = true;
            
        #if defined(ESP8266)
            configTime("GMT", "pool.ntp.org");
        #else
            configTzTime("GMT", "pool.ntp.org");
        #endif

    }
}

void NetTime::cbSyncTime()  {
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
}
