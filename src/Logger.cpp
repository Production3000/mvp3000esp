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

#include "_Helper.h"
extern _Helper _helper;


void Logger::setup() {
    if (cfgLogger.outputSettings.isSet(CfgLogger::OutputTarget::CONSOLE)) {
        Serial.begin(115200);
        while (!Serial)
            yield();
        Serial.println("");
        Serial.println("");
    }

    if (!cfgLogger.outputSettings.isSet(CfgLogger::OutputTarget::WEBLOG))
        webPage = webPageHardDisabled;

    if (cfgLogger.outputSettings.isSet(CfgLogger::OutputTarget::WEBSOCKET))
        mvp.net.netWeb.webSockets.registerWebSocket(webSocketUri);

    write(CfgLogger::Level::INFO, "Logger initialized.");
}


//////////////////////////////////////////////////////////////////////////////////

void Logger::write(CfgLogger::Level messageLevel, const String& message) {
    // Remember if any error was reported
    if (messageLevel == CfgLogger::Level::ERROR)
        errorReported = true;

    // Logging is turned off, nothing to do
    if (cfgLogger.outputSettings.isNone())
        return;

    // Message level is below logging level, nothing to do
    if (messageLevel < cfgLogger.level)
        return;

    // Output to serial, webpage, and websocket
    if (cfgLogger.outputSettings.isSet(CfgLogger::OutputTarget::CONSOLE))
        printSerial(messageLevel, message);

    if (cfgLogger.outputSettings.isSet(CfgLogger::OutputTarget::WEBLOG))
        if (messageLevel >= CfgLogger::Level::USER) // USER, WARNING, ERROR are stored
            linkedListLog.append(mvp.net.netTime.millisSinceEpoch(), messageLevel, message);
        
    if (cfgLogger.outputSettings.isSet(CfgLogger::OutputTarget::WEBSOCKET))
        if (messageLevel != CfgLogger::Level::DATA) // Omit data, this is to be handled within the module using a separate websocket
            printNetwork(messageLevel, message);
}

void Logger::writeFormatted(CfgLogger::Level messageLevel, const String& formatString, ...) {
    // writeFormatted(CfgLogger::Level::INFO, "This is the string '%s' and the number %d", "Hello World", 42);
    va_list args;
    va_start(args, formatString);
    write(messageLevel, _helper.printFormatted(formatString, args));
}

void Logger::writeFormatted(CfgLogger::Level messageLevel, const String& formatString, va_list& args) {
    write(messageLevel, _helper.printFormatted(formatString, args));
}


//////////////////////////////////////////////////////////////////////////////////

void Logger::printNetwork(CfgLogger::Level messageLevel, const String &message) {
    // Prefix with timestamp, add type literal
    String str = _helper.timeString();
    str += levelToString(messageLevel);
    str += message;
    mvp.net.netWeb.webSockets.printWebSocket(webSocketUri, str);
}

void Logger::printSerial(CfgLogger::Level messageLevel, const String &message) {
    // Prefix with timestamp, add type literal
    Serial.print(_helper.timeString());
    Serial.print(levelToString(messageLevel));

    // Color-code messages for easier readability
    // ANSI escape sequences \033[XXXXm where XXXX is a series of semicolon-separated parameters.
    // To reset formatting afterwards: \033[0m
    if (cfgLogger.ansiColor) {
        switch (messageLevel) {
            case CfgLogger::Level::INFO: Serial.print("\033[90m"); break; // bright black, also called dark grey by commoners
            case CfgLogger::Level::DATA: Serial.print("\033[34m"); break; // blue
            case CfgLogger::Level::CONTROL: Serial.print("\033[32m"); break; // green
            case CfgLogger::Level::USER: Serial.print("\033[95;1m"); break; // magenta, bold
            case CfgLogger::Level::WARNING: Serial.print("\033[33m"); break; // yellow
            case CfgLogger::Level::ERROR : Serial.print("\033[31;1m"); break; // red, bold
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

String Logger::levelToString(CfgLogger::Level messageLevel) {
    switch (messageLevel) {
        case CfgLogger::Level::INFO: return " [I] ";
        case CfgLogger::Level::DATA: return " [D] ";
        case CfgLogger::Level::CONTROL: return " [C] ";
        case CfgLogger::Level::USER: return " [U] ";
        case CfgLogger::Level::WARNING: return " [W] ";
        case CfgLogger::Level::ERROR : return " [E] ";
        default: return ""; // Does not happen
    }
}

///////////////////////////////////////////////////////////////////////////////////

String Logger::templateProcessor(uint16_t var) {
    switch (var) {

        case 30:
            if (linkedListLog.getSize() == 0) {
                return "[No log entries]";
            }
            // Set initial bookmark
            linkedListLog.bookmarkByIndex(0, true);
        case 31:
            return _helper.printFormatted("%s%s%s %s",
                _helper.msEpochToUtcString(linkedListLog.getBookmarkData()->millisStamp).c_str(),
                levelToString(linkedListLog.getBookmarkData()->level),
                linkedListLog.getBookmarkData()->message.c_str(),
                (linkedListLog.moveBookmark(true)) ? "\n%31%" : ""); // Recursive call if there are more entries

        default:
            return "";
    }
}
