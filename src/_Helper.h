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


struct _Helper {

// Replicates selected methods available only in ESP8266 to ESP32 to simplify code later on
// Ensures that the ESPX object is available in the same way on both platforms

    _Helper() {
#if defined(ESP8266)
        ESPX = &ESP;
#endif
    };

#if defined(ESP8266)
    EspClass* ESPX;
#elif defined(ESP32)
    EspClassX* ESPX = new EspClassX();
#else // Ensure architecture is correct, just for the sake of it
    #error "Unsupported platform"
#endif

///////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Check if a string is a valid decimal number (including integer)
     * 
     * @param str String to check
     */
    bool isValidDecimal(const String& str) { return _checkNumberType(str, false); };

    /**
     * @brief Check if a string is a valid integer
     * 
     * @param str String to check
     */
    bool isValidInteger(const String& str) { return _checkNumberType(str, true); };

    bool _checkNumberType(const String& str, boolean limitToInteger)  {
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


///////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Convert milliseconds to a time string
     * 
     * @param total_ms Milliseconds to convert
     * 
     * @return Time string in the format "d hh:mm:ss"
     */
    String millisToTime(uint64_t total_ms)  {
        uint64_t total_s = total_ms / 1000;
        uint16_t days = total_s / 86400; // 24*60*60
        uint32_t remaining_s = total_s % 86400;
        uint8_t hours = remaining_s / 3600; // 60*60
        remaining_s = total_s % 3600;
        uint8_t minutes = remaining_s / 60;
        uint8_t seconds = remaining_s % 60;
        return printFormatted("%dd %02d:%02d:%02d", days, hours, minutes, seconds);
    }


///////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Print a formatted string
     * 
     * @param formatString Format string
     * @param ... Arguments
     * 
     * @return Formatted string
     */
    String printFormatted(const String& formatString, ...) {
        va_list args;
        va_start(args, formatString);
        // Get length incl termination
        uint8_t len = vsnprintf(nullptr, 0, formatString.c_str(), args) + 1;
        char message[len];
        vsnprintf(message, sizeof(message), formatString.c_str(), args);
        va_end(args);
        return message;
    }


///////////////////////////////////////////////////////////////////////////////////

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
    // uint32_t hashStringDjb2(String str) { return hashStringDjb2(str.c_str()); };



///////////////////////////////////////////////////////////////////////////////////

#ifdef ESP32

    class EspClassX {
        public:

            uint32_t getChipId() {
                // From espressif, created 2020-06-07 by cweinhofer
                // Replicates the output from the ESP.getChipId() function on ESP8266, the last 3 bytes of the MAC address
                uint32_t chipId = 0;
                for(uint8_t i = 0; i < 17; i = i + 8) {
                    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
                }
                return chipId;
            };

            int8_t getHeapFragmentation() {
                // The ESP32 has two cores, with separate heaps.
                // If a thread (? linked list) allocates a lot of memory it seems to fill up the heap of the core it runs on.
                // The getMaxAllocHeap() then still indicates a rather large allocatable memory. This is however on the other core.
                // Best indicator is the shrinking difference between free and largest allocatable memory.
                // It starts at 50% allocatable, because half of total for each core.
                // It goes to >99% allocatable, with one heap quasi full, the other having all the space. 

                // Math:
                // 100 - 100 * (free - largest) / (total/2) -> initially 50 .. 100 in the end
                // Stretch to 0..100: -50 *2
                // 2* ( 50 - 100* (free - largest) / (total/2) )

                return  max( (int)(100 - ((ESP.getFreeHeap() - ESP.getMaxAllocHeap()) * 200) / (ESP.getHeapSize() / 2)) , 0);
            }

            String getResetReason() {
                // adapted from code from ESP8266, more reasons
                // REASON_DEFAULT_RST // normal startup by power on
                // REASON_WDT_RST // hardware watch dog reset
                // REASON_EXCEPTION_RST // exception reset, GPIO status won’t change
                // REASON_SOFT_WDT_RST // software watch dog reset, GPIO status won’t change
                // REASON_SOFT_RESTART // software restart ,system_restart , GPIO status won’t change
                // REASON_DEEP_SLEEP_AWAKE // wake up from deep-sleep
                // REASON_EXT_SYS_RST // external system reset

                const __FlashStringHelper* buff;
                switch (esp_reset_reason()) {
                    case ESP_RST_POWERON: // normal startup by power on - THIS IS ALSO RESET BUTTON PRESSED
                        buff = F("Power On"); break;
                    case ESP_RST_INT_WDT: // Reset (software or hardware) due to interrupt watchdog
                        buff = F("Interrupt Watchdog"); break;
                    case ESP_RST_PANIC: // Software reset due to exception/panic
                        buff = F("Exception"); break;
                    case ESP_RST_TASK_WDT: // Task watchdog
                        buff = F("Task Watchdog"); break;
                    case ESP_RST_WDT: // Other watchdog
                        buff = F("Other Watchdog"); break;
                    case ESP_RST_SW: // ESP.restart
                        buff = F("Software restart"); break;
                    case ESP_RST_BROWNOUT: // Brownout reset due to low supply voltage
                        buff = F("Brownout reset"); break;
                    case ESP_RST_SDIO: // Reset over SDIO
                        buff = F("SDIO reset"); break;
                    case ESP_RST_DEEPSLEEP: // Wake up from deep-sleep
                        buff = F("Deep-Sleep Wake"); break;
                    case ESP_RST_EXT: // Reset by external pin (not applicable for ESP32)
                        buff = F("External System"); break;
                    case ESP_RST_UNKNOWN:
                    default:
                        buff = F("Unknown"); break;
                }
                return String(buff);
            }

            void reset() { ESP.restart(); };

    };

#endif

};

#endif