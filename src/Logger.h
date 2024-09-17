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

#ifndef MVP3000_LOGGER
#define MVP3000_LOGGER

#include <Arduino.h>
#include <stdarg.h>

#include "_Helper_LinkedList.h"



struct CfgLogger {
    // Not loaded from SPIFFS, as thatis not started yet.
    
    enum Level: uint8_t {
        ERROR = 0,
        WARNING = 1,
        USER = 2,
        CONTROL = 3,
        DATA = 4,
        INFO = 5,
    };

    enum Target: uint8_t {
        NONE = 0,
        CONSOLE = 1,
        NETWORK = 2,
        BOTH = 3,
    };

    Level level = Level::INFO;

    Target target = Target::BOTH;

    boolean ansiColor = true;
};


class Logger {

    public:

        boolean errorReported = false;

        void setup();

        void ansiColor(boolean enable) { cfgLogger.ansiColor = enable; }

        String getRecentLog();
       
        // Plain test output
        void write(CfgLogger::Level targetLevel, const char *message);
        // Output data in CSV format
        void writeCSV(CfgLogger::Level targetLevel, int32_t* dataArray, uint8_t dataLength, uint8_t matrixColumnCount);
        // Formatted output: writeFormatted(CfgLogger::Level::INFO, "This is the string '%s' and the number %d", "Hellow World", 42);
        void writeFormatted(CfgLogger::Level targetLevel, const char* formatString, ...);

    private:

        struct DataStructLog {
            uint64_t time;
            uint8_t level;
            String message;

            DataStructLog(String message, uint8_t level) : time(millis()), message(message), level(level) { }
        };

        struct LinkedListLog : LinkedListNEW3000<DataStructLog> {
            LinkedListLog(uint16_t size) : LinkedListNEW3000<DataStructLog>(size) { }

            void append(uint8_t level, String message) {
                // Create data structure and add node to linked list
                // Using this-> as base class/function is templated
                this->appendDataStruct(new DataStructLog(message, level));
            }
        };


        CfgLogger cfgLogger;

        uint8_t logStoreLength = 10;
        LinkedListLog linkedListLog = LinkedListLog(logStoreLength);

        bool checkTargetLevel(CfgLogger::Level targetLevel);

        void serialPrint(CfgLogger::Level targetLevel, const char *message);

        std::function<void(const String &message)> webSocketPrint; // Function to print to the websocket

};

#endif