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


struct CfgLogger {
    
    enum Level: uint8_t {
        USER = 0,
        ERROR = 1,
        WARNING = 2,
        DATA = 3,
        INFO = 4,
    };

    Level level = Level::INFO;

    enum Target: uint8_t {
        NONE = 0,
        CONSOLE = 1,
        NETWORK = 2,
        BOTH = 3,
    };

    Target target = Target::BOTH;

    boolean ansiColor = true;

    // Inits
    CfgLogger() { };
    CfgLogger(Target _target) : target(_target) { };
    CfgLogger(Target _target, Level _level) : target(_target), level(_level) { };
};


class Logger {
    private:
        CfgLogger cfgLogger;

        bool checkTargetLevel(CfgLogger::Level targetLevel);

        void serialWrite(CfgLogger::Level targetLevel, const char *message);

    public:

        boolean errorReported = false;

        void setup(CfgLogger _cfgLogger);
       
        // Plain test output
        void write(CfgLogger::Level targetLevel, const char *message);
        // Output data in CSV format
        void writeCSV(CfgLogger::Level targetLevel, int32_t* dataArray, uint8_t dataLength, uint8_t dataMatrixColumnCount);
        // Formatted output: writeFormatted(CfgLogger::Level::INFO, "This is the string '%s' and the number %d", "Hellow World", 42);
        void writeFormatted(CfgLogger::Level targetLevel, const char* formatString, ...);

};

#endif