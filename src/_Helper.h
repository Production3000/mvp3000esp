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

// Additional defines are in Arduino.h
#define checkrange(amt, low, high) ( ((amt)<(low) || (amt) > (high)) ? (false) : (true) )  // Compare constrain(amt,low,high)


struct _Helper {

    _Helper() {

#if defined(ESP8266)
        ESPX = &ESP;
#elif defined(ESP32)
        ESPX = new EspClassX();
#else // Ensure architecture is correct, just for the sake of it
    #error "Unsupported platform"
#endif

    };

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

    uint64_t millisAtTimeinfo = 0;
    time_t timeAtTimeinfo = 0;

    // Convert millis to the NTP synced time
    uint64_t millisSinceEpoch(uint64_t millisStamp = millis()) {
        return timeAtTimeinfo * 1000 + millisStamp - millisAtTimeinfo;
    }
    
    String msEpochToUtcString(uint64_t millisStamp) {
        time_t seconds = millisStamp / 1000;
        tm timeinfo;
        localtime_r(&seconds, &timeinfo);
        return printFormatted("%04d-%02d-%02d %02d:%02d:%02d", timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    /**
     * @brief Get the local time in UTC string format.
     *
     * @return Current time in ISO format "YYYY-MM-DD hh:mm:ss"
     */
    String timeUtcString() {
        // This crazy getLocalTime() in ESP time class has a default wait time of 5 seconds!!!
        tm timeinfo;
        time_t now = time(nullptr);
        localtime_r(&now, &timeinfo);
        if(timeinfo.tm_year > (1970 - 1900)){ // realtime
            return printFormatted("%04d-%02d-%02d %02d:%02d:%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        } else {
            return uptimeUtcString(); // uptime
        }
    }

    String uptimeUtcString() {
        time_t seconds = millis() / 1000;
        tm timeinfo;
        localtime_r(&seconds, &timeinfo);
        timeinfo.tm_year -= 70; // 1970
        // Why is the month 0 ??? !!!
        timeinfo.tm_mday--; // 1st day is 0
        return printFormatted("%04d-%02d-%02d %02d:%02d:%02d", timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
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
        return printFormatted(formatString, args);
        // NOTE: va_end(args) is called in printFormatted to allow direct return
    }

    String printFormatted(const String& formatString, va_list& args) {
        // Get length including termination
        uint8_t len = vsnprintf(nullptr, 0, formatString.c_str(), args) + 1;
        char buffer[len];
        vsnprintf(buffer, len, formatString.c_str(), args);
        va_end(args);
        return buffer;
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
    uint32_t hashStringDjb2(const char* str, uint8_t h = 0) {
        // constexpr needs to be defined in .h file
        return !str[h] ? 5381 : (hashStringDjb2(str, h+1) * 33) ^ str[h];
    };


///////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Struct to store multiple boolean settings as bits in one variable. Initializes with all true. Use together with an settings-enum.
     * 
     * @tparam T Defines the size of the settings variable and thus the number of boolean settings. Accepts only unsigned types.
     */
    template<typename T, typename std::enable_if<!std::is_signed<T>::value, int>::type = 0>
    struct MultiBoolSettings {
        T _settings;

        MultiBoolSettings() : _settings(-1) { } // Default initializes all to true
        MultiBoolSettings(T settings) : _settings(settings) { } // Initialize with custom settings, 0 is all false

        void change(uint8_t bit, boolean value) {
            if (value)
                set(bit);
            else
                unset(bit);
        }

        void set(uint8_t bit) { bitSet(_settings, bit); }
        void unset(uint8_t bit) { bitClear(_settings, bit); }

        T* getAll() { return &_settings; }
        void setAll(T settings) { _settings = settings; }

        boolean isNone() { return _settings == 0; }
        boolean isSet(uint8_t bit) { return bitRead(_settings, bit); }
    };


///////////////////////////////////////////////////////////////////////////////////

#if defined(ESP8266)
    EspClass* ESPX;
#elif defined(ESP32)

    // Replicates selected methods available only in ESP8266 to ESP32 to simplify code later on
    // Ensures that the ESPX object is available in the same way on both platforms
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

    EspClassX* ESPX;

#endif

};

#endif