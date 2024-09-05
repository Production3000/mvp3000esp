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

#ifndef MVP3000_HELNEWPER
#define MVP3000_HELNEWPER

#include <Arduino.h>

struct LimitTimer {

    uint32_t _interval_ms;
    uint8_t _limit_count; // 0 means no limit

    uint64_t _next_ms = 0; // First call of justFinished() will return true
    uint64_t _plusOne_ms = std::numeric_limits<uint64_t>::max();

    uint8_t _counter = 0;

    LimitTimer(uint32_t interval_ms) : _interval_ms(interval_ms), _limit_count(0) { }
    LimitTimer(uint32_t interval_ms, uint8_t limit_count) : _interval_ms(interval_ms), _limit_count(limit_count) { }

    /**
     * @brief Check if the interval has passed.
     * 
     * @return true on the first call and if the interval has passed while the limit was not reached.
     */
    bool justFinished() {
        if (millis() > _next_ms) {
            // End interval if limit set and reached, set plusOne
            if ((_limit_count > 0) && (++_counter >= _limit_count)) {
                _next_ms = std::numeric_limits<uint64_t>::max();
                _plusOne_ms = millis() + _interval_ms;
            } else {
                _next_ms = millis() + _interval_ms;
            }
            return true;
        }
        return false;
    }

    /**
     * @brief Check if a single additional interval passed after the limit was reached.
     */
    bool plusOne() {
        if (millis() > _plusOne_ms) {
            _plusOne_ms = std::numeric_limits<uint64_t>::max();
            return true;
        }
        return false;
    }

    void reset() {
        _counter = 0;
        _next_ms = 0; // Next call of justFinished() will return true
        _plusOne_ms = std::numeric_limits<uint64_t>::max();
    }

    bool running() {
        return ( (_limit_count == 0) || (_counter < _limit_count) || ((_counter == _limit_count) && (_plusOne_ms != std::numeric_limits<uint64_t>::max()) ) ) ? true : false;
    }
};

#endif