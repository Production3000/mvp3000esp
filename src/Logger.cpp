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

#include "Logger.h"

#include "MVP3000.h"
extern MVP3000 mvp;


void Logger::setup() {
    // Do nothing if turned off
    if (cfgLogger.target == CfgLogger::Target::NONE)
        return;

    if ((cfgLogger.target == CfgLogger::Target::CONSOLE) || (cfgLogger.target == CfgLogger::Target::BOTH)) {
        Serial.begin(115200);
        while (!Serial)
            yield();
        Serial.println("");
        Serial.println("");
    }

    if ((cfgLogger.target == CfgLogger::Target::NETWORK) || (cfgLogger.target == CfgLogger::Target::BOTH)) {
        webSocketPrint = mvp.net.netWeb.registerWebSocket("/wslog"); // Not sure why this works here, before the netWeb.setup() call ...
    }

    write(CfgLogger::Level::INFO, "Logger initialized.");
}


//////////////////////////////////////////////////////////////////////////////////

void Logger::write(CfgLogger::Level targetLevel, const char *message) {                                 // TODO feature: store last error messages and warnings and serve via web
    if (!checkTargetLevel(targetLevel))
        return;

    // Serial output
    if ((cfgLogger.target == CfgLogger::Target::CONSOLE) || (cfgLogger.target == CfgLogger::Target::BOTH)) {
        serialPrint(targetLevel, message);
    }
    // Network output, omit DATA level
    if ( ((cfgLogger.target == CfgLogger::Target::NETWORK) || (cfgLogger.target == CfgLogger::Target::BOTH)) && (targetLevel != CfgLogger::Level::DATA) ) {
        webSocketPrint(mvp.helper.upTime() + " " + message);
    }
}

void Logger::writeCSV(CfgLogger::Level targetLevel, int32_t* dataArray, uint8_t dataLength, uint8_t dataMatrixColumnCount) {
    String message = "";
    for (uint8_t i = 0; i < dataLength; i++) {
        // Outputs:
        //  1,2,3,4,5,6; for rowLength is max uint8/255
        //  1,2,3;4,5,6; for rowLength is 3
        // dataMatrixColumnCount defaults to 255, which is the maximum length of a single row
        message += String(dataArray[i]);
        message += ((i == dataLength - 1) || ((i + 1) % (dataMatrixColumnCount) == 0) ) ? ";" : "," ;
    }
    write(targetLevel, message.c_str());
}

void Logger::writeFormatted(CfgLogger::Level targetLevel, const char* formatString, ...) {
    char message[256]; // Define the buffer size as per your requirement
    va_list args;
    va_start(args, formatString);
    vsnprintf(message, sizeof(message), formatString, args);
    va_end(args);

    write(targetLevel, message);
}


//////////////////////////////////////////////////////////////////////////////////

bool Logger::checkTargetLevel(CfgLogger::Level targetLevel) {
    // Remember if any error was reported
    if (targetLevel == CfgLogger::Level::ERROR)
        errorReported = true;

    // Logging is turned off, nothing to do
    if (cfgLogger.target == CfgLogger::Target::NONE)
        return false;

    // Message level is below logging level, nothing to do
    if (targetLevel > cfgLogger.level)
        return false;

    return true;
}

void Logger::serialPrint(CfgLogger::Level targetLevel, const char *message) {
    // Prefix with timestamp
    Serial.print(mvp.helper.upTime());

    // Add type literal
    switch (targetLevel) {
        case CfgLogger::Level::CONTROL: Serial.print(" [C] "); break;
        case CfgLogger::Level::DATA: Serial.print(" [D] "); break;
        case CfgLogger::Level::ERROR : Serial.print(" [E] "); break;
        case CfgLogger::Level::INFO: Serial.print(" [I] "); break;
        case CfgLogger::Level::USER: Serial.print(" [U] "); break;
        case CfgLogger::Level::WARNING: Serial.print(" [W] "); break;
    }

    // Color-code messages for easier readability
    // ANSI escape sequences \033[XXXm where XXX is a series of semicolon-separated parameters.
    //  red     31
    //  green   32
    //  yellow  33
    //  blue    34
    //  magenta 95
    //  bold    1
    // To reset: \033[0m
    if (cfgLogger.ansiColor) {
        switch (targetLevel) {
            case CfgLogger::Level::CONTROL: Serial.print("\033[32m"); break; // green
            case CfgLogger::Level::DATA: Serial.print("\033[34m"); break; // blue
            case CfgLogger::Level::ERROR : Serial.print("\033[31;1m"); break; // red, bold
            case CfgLogger::Level::INFO: Serial.print("\033[90m"); break; // bright black, also called dark grey by commoners
            case CfgLogger::Level::USER: Serial.print("\033[95;1m"); break; // magenta, bold
            case CfgLogger::Level::WARNING: Serial.print("\033[33m"); break; // yellow
        }
    }

    // Print actual message
    Serial.print(message);

    // Reset ansi text formatting and end line
    if (cfgLogger.ansiColor) {
        Serial.print("\033[0m");
    }
    Serial.println("");
}
