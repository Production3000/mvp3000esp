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

#ifndef MVP3000_HELPER
#define MVP3000_HELPER

#include <Arduino.h>


class Helper {
    public:
        bool isValidDecimal(String str, boolean limitToInteger = false);
        bool isValidDecimal(const char *str, boolean limitToInteger) { return isValidDecimal(String(str), limitToInteger); };
        bool isValidInteger(String str) { return isValidDecimal(str, true); };
        bool isValidInteger(const char *str) { return isValidDecimal(String(str), true); };

        String millisToTime(uint32_t total_ms);
        String upTime() { return millisToTime(millis()); };

        // The djb2 hash function by Dan Bernstein to use in switch statement
        // constexpr needs to be defined in .h
        constexpr uint32_t hashStringDjb2(const char* str, uint8_t h = 0) {
            return !str[h] ? 5381 : (hashStringDjb2(str, h+1) * 33) ^ str[h];
        };
        
};

#endif