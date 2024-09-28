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

#ifndef MVP3000_NETTIME
#define MVP3000_NETTIME

#include <Arduino.h>



class NetTime {

    public:

        void setup();
        void loop();

        uint64_t millisSinceEpoch(uint64_t millisStamp = millis()) {
            return timeAtTimeinfo * 1000 + millisStamp - millisAtTimeinfo;
        }

        void cbSyncTime();

    private:

        boolean ntpRequested = false;
        uint64_t waitBeforeRequest = 0;

        uint64_t millisAtTimeinfo = 0;
        time_t timeAtTimeinfo = 0;

        uint64_t millisAtFirstTimeinfo = 0;
        time_t timeAtFirstTimeinfo = 0;
        int32_t totalDiff = 0;


    public:



};

#endif