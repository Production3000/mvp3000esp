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

// Loads/saves the config files from/to disk.
// Offers functions to write/read custom configs, with different encoding/decoding options for number arrays

#ifndef MVP3000_CONFIG
#define MVP3000_CONFIG

#include <Arduino.h>
#ifdef ESP32
    #include <SPIFFS.h>
#endif
#include <FS.h>
#include <ArduinoJson.h>

#include "Logger.h"


class Config {
    private:
        JsonDocument jsonDoc;

        boolean fileSystemOK = false;
        bool isReadyFS();

        bool readFile(const char *filename, std::function<bool(File& file)> writerFunc);
        bool writeFile(const char *filename, std::function<bool(File& file)> writerFunc);

    public:
        
        void setup();

        // void removeCustomCfg(const char *cfgName);
        void removeCfg(const char *fileName);
        void factoryResetDevice();



        // Templates need to be in the header to be included every time, or explicitly called in cpp to exist everywhere 
        // https://stackoverflow.com/questions/8752837/undefined-reference-to-template-class-constructor

        bool cfgReadPrepare(const char *fileName);
        void cfgReadClose() { jsonDoc.clear(); };
        template <class T> // Single value variables of any type except char*, char[n]
        bool cfgReadGetValue(const char *varName, T &dest) {
            if (!jsonDoc.containsKey(varName) || !jsonDoc[varName].is<T>())
                return false;
            dest = jsonDoc[varName].as<T>(); // .as<T>() needd for String type
            return true;
        };
        // template <class T> // Single value of char[n]
        // bool cfgReadGetValueChar(const char *varName, T &dest) {
        //     String tmp;
        //     boolean success = cfgReadGetValue(varName, tmp);
        //     strncpy(dest, tmp.c_str(), sizeof(dest));
        //     return success;
        // };

        template <class T> // Arrays
        bool cfgReadGetValue(const char *varName, T &dest, uint8_t arraySize) {
            // Assigns values only if varName exists
            if (!jsonDoc.containsKey(varName) || !jsonDoc[varName].is<JsonArray>())
                return false;
            JsonArray jsonArray = jsonDoc[varName].as<JsonArray>();
            if (jsonArray.size() != arraySize) // Make sure size is correct to not have memory issues
                return false;
            uint8_t i = 0;
            for(JsonVariant value : jsonArray) {
                // if (!value.is<T>())
                //     return false;
                dest[i++] = value; // .as<T>() not defined for uint16_t and some others
            }
        };

        void cfgWritePrepare();        
        void cfgWriteClose(const char *fileName);
        template <class T> // Single value variables of any type
        void cfgWriteAddValue(const char *varName, T content) {
            jsonDoc[varName] = content;
        };
        template <class T> // Arrays
        void cfgWriteAddValue(const char *varName, T *content, uint8_t arraySize) {
            JsonArray data = jsonDoc.createNestedArray(varName);
            for (uint8_t i = 0; i < arraySize; i++) {
                data.add(*(content + i));
            }
        };
};

#endif