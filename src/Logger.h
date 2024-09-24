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
    // Not loaded from SPIFFS, as that is not started yet.

    enum Level: uint8_t {
        INFO = 0,
        DATA = 1,
        CONTROL = 2,
        USER = 3,
        WARNING = 4,
        ERROR = 5,
    };

    enum Target: uint8_t {
        NONE = 0,
        CONSOLE = 1,
        NETWORK = 2,
        BOTH = 3,
    };

    boolean ansiColor = true;
    Level level = Level::INFO;
    Target target = Target::BOTH;
};


class Logger {

    public:

        /**
         * @brief Write a message to the log
         * 
         * @param messageLevel The level of the message
         * @param message The message
         */
        void write(CfgLogger::Level messageLevel, const String& message);

        /**
         * @brief Write a formatted message to the log
         * 
         * @param messageLevel The level of the message
         * @param formatString The format string
         * @param ... The arguments to the format string
         */
        void writeFormatted(CfgLogger::Level messageLevel, const String& formatString, ...);

    public:

        boolean errorReported = false;

        void setup();

        void disableAnsiColor() { cfgLogger.ansiColor = false; }
        void setLevel(CfgLogger::Level level) { cfgLogger.level = level; }
        void setTarget(CfgLogger::Target target) { cfgLogger.target = target; }

    private:

        struct DataStructLog {
            uint64_t time;
            CfgLogger::Level level;
            String message;

            DataStructLog(const String& message, CfgLogger::Level level) : time(millis()), message(message), level(level) { }
        };

        struct LinkedListLog : LinkedList3010<DataStructLog> {
            LinkedListLog(uint16_t size) : LinkedList3010<DataStructLog>(size) { }

            void append(CfgLogger::Level level, const String& message) {
                // Create data structure and add node to linked list
                // Using this-> as base class/function is templated
                this->appendDataStruct(new DataStructLog(message, level));
            }
        };

        CfgLogger cfgLogger;

        String webSocketUri = "/wslog";

        uint8_t logStoreLength = 5;
        LinkedListLog linkedListLog = LinkedListLog(logStoreLength);

        void printNetwork(CfgLogger::Level messageLevel, const String& message);
        void printSerial(CfgLogger::Level messageLevel, const String& message);

        String levelToString(CfgLogger::Level messageLevel);

    public:

        String templateProcessor(uint8_t var);
        const char* webPage = R"===(
<h3>Log</h3> <ul>
<li>Log websocket: ws://%2%/wslog </li>
<li>Recent log: <ul> %30% </ul> </li> </ul>
)===";

};

#endif