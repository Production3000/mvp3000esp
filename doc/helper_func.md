# Helper Functions and Classes

Various helper functions and classes are provided.

## <a name='Contents'></a>Contents

<!-- vscode-markdown-toc -->
* [Contents](#Contents)
* [LimitTimer](#LimitTimer)
	* [LimitTimer](#LimitTimer-1)
	* [Usage Example](#UsageExample)
* [License](#License)

<!-- vscode-markdown-toc-config
	numbering=false
	autoSave=true
	/vscode-markdown-toc-config -->
<!-- /vscode-markdown-toc -->



## <a name='LimitTimer'></a>LimitTimer

The LimitTimer class provides a mechanism for creating non-blocking millisecond delays. It allows you to set an interval and optionally limit the number of intervals to run. This is particularly useful in embedded systems programming where blocking delays can disrupt the timing of other tasks.

### <a name='LimitTimer-1'></a>LimitTimer

Constructor:

 *  `LimitTimer(uint32_t interval_ms)`: Initializes the timer with the specified interval and no limit of iterations.
 *  `LimitTimer(uint32_t interval_ms, uint8_t limit_count)`: Initializes the timer with the specified interval and limits the number of iterations.

Functions:

 *  `bool justFinished()`: Checks if an interval has passed. Returns true on the first call and every time the interval elapses. If the limit is set and reached, the timer stops. Otherwise it is automatically restarted.
 *  `bool plusOne()`: Checks if an additional interval has passed since the iteration limit was reached.
 *  `void restart()`: Restart the timer with the previous settings after the limit was reached or it was stopped.
 *  `void restart(uint32_t interval_ms)`: Restart the timer with a new interval after the limit was reached or it was stopped.
 *  `boolean running()`: Returns true if the timer is running and the iteration count was not reached.
 *  `boolean runningPlusOne()`: Returns true if the PlusOne interval is running.
 *  `void stop()`: Stops the timer, but starts PlusOne.

### <a name='UsageExample'></a>Usage Example

    #include <Arduino.h>
    #include "LimitTimer.h"

    LimitTimer timer(1000, 5); // 1-second interval, limit of 5 intervals

    void setup() {
        Serial.begin(115200);
    }

    void loop() {
        if (timer.justFinished()) {
            Serial.println("Interval finished");
        }

        if (timer.plusOne()) {
            Serial.println("Additional interval finished");
        }
    }


## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000