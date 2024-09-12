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

#ifndef MVP3000_HELPER_LIMITTIMER
#define MVP3000_HELPER_LIMITTIMER

#include <Arduino.h>

/**
 * @brief Create a new timer to allow non-blocking millisecond delays.
 * 
 * @param interval_ms The interval in milliseconds, can be 0 for instant finish.
 * @param limit_count (optional) The number of intervals to run, 0 to ignore.
 */
struct LimitTimer {

    uint32_t _interval_ms; // 0 is off = instant finish
    uint64_t _next_ms = 0; // First call of justFinished() will return true
    uint64_t _plusOne_ms = std::numeric_limits<uint64_t>::max();

    uint8_t _limit_count; // 0 is no limit
    uint8_t _counter = 0;

    LimitTimer(uint32_t interval_ms) : _interval_ms(interval_ms), _limit_count(0) { }
    LimitTimer(uint32_t interval_ms, uint8_t limit_count) : _interval_ms(interval_ms), _limit_count(limit_count) { }


    /**
     * @brief Check if a interval has passed. True also on first call.
     */
    bool justFinished() {
        if (millis() > _next_ms) {
            // End interval if limit set and reached, set plusOne
            if ((_limit_count > 0) && (++_counter >= _limit_count)) {
                stop();
            } else {
                _next_ms = millis() + _interval_ms;
            }
            return true;
        }
        return false;
    }

    /**
     * @brief Check if an additional interval has passed since the count limit was reached.
     */
    bool plusOne() {
        if (millis() > _plusOne_ms) {
            _plusOne_ms = std::numeric_limits<uint64_t>::max();
            return true;
        }
        return false;
    }

    /**
     * @brief Restart the timer with a new interval. Only needed after the limit was reached or stop called.
     * 
     * @param interval_ms The new interval in milliseconds.
     */
    void restart(uint32_t interval_ms) {
        _interval_ms = interval_ms;
        restart();
    }

    /**
     * @brief Restart the timer with the previous settings. Only needed after the limit was reached or stop called.
     */
    void restart() {
        _counter = 0;
        _next_ms = 0; // Next call of justFinished() will return true
        _plusOne_ms = std::numeric_limits<uint64_t>::max();
    }

    /**
     * @brief Stop the timer, but start PlusOne. The timer will not finish anymore.
     */
    void stop() {
        _next_ms = std::numeric_limits<uint64_t>::max();
        _plusOne_ms = millis() + _interval_ms;
    }

    /**
     * @brief Check if the timer is running. True if the limit is not reached and the timer is not stopped.
     */
    boolean running() { // unlimited, limit not reached, not stopped
        return ( ( (_limit_count == 0) || (_counter < _limit_count) ) && (_next_ms != std::numeric_limits<uint64_t>::max()) );
    }

    /**
     * @brief Check if the timer or PlusOne is running.
     */
    boolean runningPlusOne() {
        return ( (_limit_count > 0) && (_counter == _limit_count) && (_plusOne_ms != std::numeric_limits<uint64_t>::max()) );
    }
};

#endif