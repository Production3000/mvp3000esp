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

#ifndef MVP3000_CONFIG
#define MVP3000_CONFIG

#include <Arduino.h>
#ifdef ESP32
    #include <SPIFFS.h>
#endif
#include <FS.h>
#include <ArduinoJson.h>

#include "Logger.h"

#include "Config_JsonInterface.h"


class Config {
    private:
        JsonDocument jsonDoc;

        boolean fileSystemOK = false;
        bool isReadyFS();

        bool readFileToJson(const char *fileName);
        void writeJsonToFile(const char *fileName);

        bool readFile(const char *filename, std::function<bool(File& file)> writerFunc);
        bool writeFile(const char *filename, std::function<bool(File& file)> writerFunc);
        void removeFile(const char *fileName);

    public:
        void setup();
        // There is no loop action in this class

        void readCfg(JsonInterface &cfg);
        void writeCfg(JsonInterface &cfg);

        void factoryResetDevice(boolean keepWifi = false);
};

#endif