# Helper Functions and Classes

<!-- vscode-markdown-toc -->
* [Helper Class](#HelperClass)
	* [Check String for Integer / Decimal](#CheckStringforIntegerDecimal)
	* [Millis to Time String](#MillistoTimeString)
* [LimitTimer](#LimitTimer)
	* [Constructor and Functions](#ConstructorandFunctions)
	* [Usage Example](#UsageExample)
* [Linked List](#LinkedList)
	* [Concept](#Concept)
	* [Predefined Lists](#PredefinedLists)
	* [Custom Lists](#CustomLists)
* [License](#License)

<!-- vscode-markdown-toc-config
	numbering=false
	autoSave=true
	/vscode-markdown-toc-config -->
<!-- /vscode-markdown-toc -->


## <a name='HelperClass'></a>Helper Class

The Helper class provides a range of functions for checking and converting strings. 

### <a name='CheckStringforIntegerDecimal'></a>Check String for Integer / Decimal 

 *  `bool isValidDecimal(const char *str)`: Check if a string is a valid decimal number.
 *  `bool isValidDecimal(String str)`: Check if a string is a valid decimal number.
 *  `bool isValidInteger(const char *str)`: Check if a string is a valid integer number.
 *  `bool isValidInteger(String str)`: Check if a string is a valid integer number.

### <a name='MillistoTimeString'></a>Millis to Time String 

 *  `String millisToTime(uint64_t total_ms)`: Convert milliseconds to a time string with the format "d hh:mm:ss"


## <a name='LimitTimer'></a>LimitTimer

The LimitTimer class provides a mechanism for creating non-blocking millisecond delays. It allows you to set an interval and optionally limit the number of intervals to run. This is particularly useful in embedded systems programming where blocking delays can disrupt the timing of other tasks.

### <a name='ConstructorandFunctions'></a>Constructor and Functions

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
    #include "_Helper_LimitTimer.h"

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


## <a name='LinkedList'></a>Linked List

### <a name='Concept'></a>Concept

### <a name='PredefinedLists'></a>Predefined Lists

### <a name='CustomLists'></a>Custom Lists


## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000