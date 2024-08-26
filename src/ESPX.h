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

#ifndef MVP3000_ESPX
#define MVP3000_ESPX

#include <Arduino.h>


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
            ESP.getMinFreeHeap();
            ESP.getMaxAllocHeap();
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

#endif
