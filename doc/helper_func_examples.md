# Helper Functions and Classes - Examples

The [Helper classes and functions](/doc/helper_func.md) provide a range of tools. 

<!-- vscode-markdown-toc -->
* [Linked List Examples](#LinkedListExamples)
	* [Adaptive Data List](#AdaptiveDataList)
	* [Unique Log Entries](#UniqueLogEntries)
	* [Bookmarking for Output](#BookmarkingforOutput)
* [License](#License)

<!-- vscode-markdown-toc-config
	numbering=false
	autoSave=true
	/vscode-markdown-toc-config -->
<!-- /vscode-markdown-toc -->


## <a name='LinkedListExamples'></a>Linked List Examples

Any implementation of a LinkedList3xxx:

 1. Define the DataStruct that holds the data. Add an equals() method if needed.
 2. Define a custom LinkedList class/struct that inherits the desired flavour/combination. Create at least an append() method matching the DataStruct.
 3. Optionally extend the custom LinkedList class/struct with additional functionality.

### <a name='AdaptiveDataList'></a>Adaptive Data List

It is not recommended to use an adaptive list during development as this hides any memory leaks in user code.

    struct DataStructSensor {
        uint64_t time;
        float_t data;

        DataStructSensor(uint64_t time, float_t data) : time(time), data(data) { }
    };
    
    struct LinkedListSensor : LinkedList3100<DataStructSensor> {
        LinkedListSensor() : LinkedList3100<DataStructSensor>() { }
        LinkedListSensor(uint16_t size) : LinkedList3100<DataStructSensor>(size) { }

        void append(uint64_t time, float_t data) {
            this->appendDataStruct(new DataStructSensor(time, data));
        }
    }

    // Enable adaptive growing from the start
    // LinkedListSensor linkedListsensor = LinkedListSensor();

    // Require explicit enabling of adaptive growing from the user script, useful for debugging memory leaks
    LinkedListSensor linkedListsensor = LinkedListSensor(10);
    linkedListsensor.enableAdaptiveGrowing();

    linkedListsensor.append(millis(), 12345);

### <a name='UniqueLogEntries'></a>Unique Log Entries

Timestamped log of domain visits. Duplicate entries are moved to the tail (newest) end of the list.

    struct DataStructLog {
        uint64_t time;
        String domain;

        DataStructLog(const String& domain) : time(millis()), domain(domain) { }

        bool equals(DataStructLog* other) {
            if (other == nullptr)
                return false;
            // Compare the data string, ignore the time stamp
            return domain.equals(other->domain);
        }
    };

    struct LinkedListLog : LinkedList3001<DataStructLog> {
        LinkedListLog(uint16_t size) : LinkedList3001<DataStructLog>(size) { }

        void appendUnique(const String& domain) {
            this->appendUniqueDataStruct(new DataStructLog(domain), true);
        }

        // void remove(const String& domain) {
        //     this->removeByContent(DataStructLog(domain));
        // }
    };

    LinkedListLog linkedListLog = LinkedListLog(10);

    linkedListLog.append("mydomain.com");

### <a name='BookmarkingforOutput'></a>Bookmarking for Output

    struct DataStructText {
        String longText;

        DataStructText(const String& longText) : longText(longText) { }
    };

    struct LinkedListText : LinkedList3010<DataStructText> {
        LinkedListText(uint16_t size) : LinkedList3010<DataStructText>(size) { }

        void append(const String& longText) {
            this->appendDataStruct(new DataStructText(longText));
        }
    };

    LinkedListText linkedListText = LinkedListText(10);

    linkedListText.append("Text1");
    linkedListText.append("Text2");
    linkedListText.append("Text3");

    // Set initial bookmark to newest one "Text3"
    linkedListText.bookmarkByIndex(0, true);

    while(true) {
        Serial.println(linkedListText.getBookmarkData()->longText)
        // Move bookmark, break if there is none
        if (!linkedListText.moveBookmark())
            break;
    }


## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000