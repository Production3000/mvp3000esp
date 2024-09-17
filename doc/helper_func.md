# Helper Functions and Classes

<!-- vscode-markdown-toc -->
* [Helper Class](#HelperClass)
	* [Check String for Integer / Decimal](#CheckStringforIntegerDecimal)
	* [Millis to Time String](#MillistoTimeString)
* [LimitTimer](#LimitTimer)
	* [Constructor and Functions](#ConstructorandFunctions)
	* [Usage Example](#UsageExample)
* [Linked List](#LinkedList)
	* [Predefined Lists](#PredefinedLists)
		* [LinkedList3000](#LinkedList3000)
		* [LinkedList3001](#LinkedList3001)
		* [LinkedList3010](#LinkedList3010)
	* [Implementation Example](#ImplementationExample)
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

##### Constructor

 *  `LimitTimer(uint32_t interval_ms)`: Initializes the timer with the specified interval and no limit of iterations.
 *  `LimitTimer(uint32_t interval_ms, uint8_t limit_count)`: Initializes the timer with the specified interval and limits the number of iterations.

##### Methods

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

A templated linked list implementation for the MVP3000 framework. The list has a maximum size limit set during initialization. If the limit is reached, the oldest element is automatically removed.

 * 3000 bare: append, clear, loop, getNewest, getOldest, getSize - dataStruct has no requirements
 * 3001 extends bare: appendUnique, findByContent, removeByContent - dataStruct needs equals() method
 * 3010 extends bare: bookmarkByIndex, hasBookmark, moveBookmark - dataStruct has no requirements
 * 3011 combines 3001 and 3010


### <a name='PredefinedLists'></a>Predefined Lists

#### <a name='LinkedList3000'></a>LinkedList3000

##### Constructor

* `LinkedList3000(uint16_t size)`: Initializes the linked list with the specified maximum size.
* `LinkedList3000()`: Default constructor with a maximum size of 1.

##### Methods

* `void append(T* newDataStruct)`: Appends a node to the linked list. Removes the oldest node if the list is full.
* `void clear()`: Clears the linked list.
* `void loop(std::function<void(T*&, uint16_t)> callback, boolean reverse = false)`: Loops through all elements in the linked list and calls the given callback function.
* `T* getNewestData()`: Returns the newest data in the list.
* `T* getOldestData()`: Returns the oldest data in the list.
* `uint16_t getSize() const`: Returns the current size of the list.
* `uint16_t getMaxSize() const`: Returns the maximum size of the list.

#### <a name='LinkedList3001'></a>LinkedList3001

Extends LinkedList3000 with additional functionalities: appendUnique, findByContent, and removeByContent. The data structure needs an `equals()` method.

##### Constructor

* `LinkedList3001(uint16_t size)`: Initializes the linked list with the specified maximum size.

##### Methods

* `void appendUnique(T* dataStruct, boolean moveToFront = true)`: Appends an element to the list or moves it to the tail of the list if it already exists.
* `Node* findByContent(T* dataStruct)`: Finds a node by its content.
* `void removeByContent(T* dataStruct)`: Removes a node by its content.

#### <a name='LinkedList3010'></a>LinkedList3010

Extends LinkedList3000 with additional functionalities: bookmarkByIndex, hasBookmark, moveBookmark, and getBookmarkData. The data structure has no additional requirements.

##### Constructor

* `LinkedList3010(uint16_t size)`: Initializes the linked list with the specified maximum size.

##### Methods

* `void bookmarkByIndex(uint16_t index, boolean reverse = false, boolean noNull = false)`: Finds the node at the given index and bookmarks it.
* `boolean hasBookmark()`: Checks if the bookmark is set.
* `boolean moveBookmark(boolean reverse = false)`: Moves the bookmark to the next node.
* `T* getBookmarkData()`: Returns the data at the bookmarked node.

### <a name='ImplementationExample'></a>Implementation Example

Create dataStruct.

Create list that inherits the required features. Add custom functions. to the class


## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000