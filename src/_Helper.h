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


struct Helper {

    /**
     * @brief Check if a string is a valid integer
     * 
     * @param str String to check
     */
    bool isValidInteger(String str) { return isValidDecimal(str, true); };
    bool isValidInteger(const char *str) { return isValidDecimal(String(str), true); };

    /**
     * @brief Check if a string is a valid decimal number
     * 
     * @param str String to check
     * @param limitToInteger If true, only integers are allowed (better use the overload isValidInteger)
     */
    bool isValidDecimal(const char *str, boolean limitToInteger) { return isValidDecimal(String(str), limitToInteger); };
    bool isValidDecimal(String str, boolean limitToInteger = false)  {
        // Empty string is not an integer
        if (str.length() < 1)
            return false;

        // Handle negative numbers, skip first character if minus and not the only character
        uint8_t startIndex = 0;
        if ((str.length() > 1) && (str.charAt(0) == '-'))
            startIndex = 1;

        uint8_t dotCount = 0;
        for(uint8_t i = startIndex; i < str.length(); i++) {

            // Not a number if any character is not a digit or a dot
            if (!( isDigit(str.charAt(i)) || (str.charAt(i) == '.') ))
                return false;

            if (str.charAt(i) == '.')
                dotCount++;
        }

        // Integer has no dot
        if (limitToInteger && (dotCount != 0))
            return false;
        // Decimal has exactly one dot
        if (!limitToInteger && (dotCount != 1))
            return false;
        
        return true;
    }



    /**
     * @brief Convert milliseconds to a time string
     * 
     * @param total_ms Milliseconds to convert
     */
    String millisToTime(uint32_t total_ms)  {
        uint32_t total_s = total_ms / 1000;
        uint16_t days = total_s / 86400; // 24*60*60
        uint32_t remaining_s = total_s % 86400;
        uint8_t hours = total_s / 3600; // 60*60
        remaining_s = total_s % 3600;
        uint8_t minutes = remaining_s / 60;
        uint8_t seconds = remaining_s % 60;
        char buffer[15];
        if (days > 0) {
            sprintf(buffer, "%dd %02d:%02d:%02d", hours, minutes, seconds);
        } else {
            sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
        }            
        return String(buffer);
    }

    /**
     * @brief Get the current uptime in a time string
     */
    String upTime() { return millisToTime(millis()); };



    /**
     * @brief Convert a string to a (quasi) unique hash
     * 
     * The djb2 hash function by Dan Bernstein is used to convert a string to a hash.
     * 
     * @param str String to convert
     * @param defaultValue Default value if conversion fails
     */
    constexpr uint32_t hashStringDjb2(const char* str, uint8_t h = 0) {
        // constexpr needs to be defined in .h file
        return !str[h] ? 5381 : (hashStringDjb2(str, h+1) * 33) ^ str[h];
    };

};

#endif