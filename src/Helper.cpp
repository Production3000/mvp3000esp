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

#include "Helper.h"


bool Helper::isValidDecimal(String str, boolean limitToInteger) {
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

String Helper::millisToTime(uint32_t total_ms) {
    uint32_t total_s = total_ms / 1000;
    uint8_t hours = total_s / 3600;
    uint16_t remaining_s = total_s % 3600;
    uint8_t minutes = remaining_s / 60;
    uint8_t seconds = remaining_s % 60;
    char buffer[12];
    sprintf(buffer, "%d:%02d:%02d", hours, minutes, seconds);
    return String(buffer);
}
