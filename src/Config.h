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
#include "Helper.h"


struct CfgStructJsonInterface {
    String cfgName = "template";

    /**
     * @brief Export the configuration data to a JSON document.
     * 
     * @param jsonDoc The JSON document to export the data to.
     */
    virtual void exportToJson(JsonDocument &jsonDoc) { };

    /**
     * @brief Import the configuration data from a JSON document.
     * 
     * @param jsonDoc The JSON document to import the data from.
     * @return True if the import was successful, false otherwise.
     */
    virtual bool importFromJson(JsonDocument &jsonDoc) { return true; };
};


struct Cfg : public CfgStructJsonInterface {

    Helper helper;

    template <typename T>                                       // TODO somehow use pointers to put all Settings in single array, maybe https://stackoverflow.com/questions/53744407/how-to-return-a-pointer-to-template-struct
    struct Setting {
        uint32_t hash;
        T *value;
        std::function<bool(T)> set;
        Setting() {}; // Default constructor needed for some reason
        Setting(uint32_t _hash,T *_value, std::function<bool(T)> _set) { hash = _hash; value = _value; set = _set; }
    };

    static const uint8_t MAX_SETTINGS = 15;
    uint8_t settingsBoolCount = 0;
    Setting<boolean> settingsBool[MAX_SETTINGS];
    uint8_t settingsIntCount = 0;
    Setting<uint16_t> settingsInt[MAX_SETTINGS];
    uint8_t settingsStringCount = 0;
    Setting<String> settingsString[MAX_SETTINGS];

    void addSetting(String key,  boolean *value, std::function<bool(boolean)> set) {
        settingsBool[settingsBoolCount] = Setting<boolean>(helper.hashStringDjb2(key.c_str()), value, set);
        settingsBoolCount++;
    }
    void addSetting(String key,  uint16_t *value, std::function<bool(uint16_t)> set) {
        settingsInt[settingsIntCount] = Setting<uint16_t>(helper.hashStringDjb2(key.c_str()), value, set);
        settingsIntCount++;
    }
    void addSetting(String key,  String *value, std::function<bool(String)> set) {
        settingsString[settingsStringCount] = Setting<String>(helper.hashStringDjb2(key.c_str()), value, set);
        settingsStringCount++;
    }

    bool updateFromWeb(String key, String value) {
        uint32_t hash = helper.hashStringDjb2(key.c_str());
        for (uint8_t i = 0; i < settingsBoolCount; i++) {
            if (settingsBool[i].hash == hash)
                return settingsBool[i].set((value.toInt() != 0));
        }
        for (uint8_t i = 0; i < settingsIntCount; i++) {
            if (settingsInt[i].hash == hash)
                return settingsInt[i].set((uint16_t)value.toInt());
        }
        for (uint8_t i = 0; i < settingsStringCount; i++) {
            if (settingsString[i].hash == hash)
                return settingsString[i].set(value);
        }
        return false;
    }

    bool importFromJson(JsonDocument &jsonDoc) {
        for (uint8_t i = 0; i < settingsBoolCount; i++) {
            String hashString = String(settingsBool[i].hash);
            if (jsonDoc.containsKey(hashString)) {
                settingsBool[i].set(jsonDoc[hashString].as<boolean>());
            }
        }
        for (uint8_t i = 0; i < settingsIntCount; i++) {
            String hashString = String(settingsInt[i].hash);
            if (jsonDoc.containsKey(hashString)) {
                settingsInt[i].set(jsonDoc[hashString].as<uint16_t>());
            }
        }
        for (uint8_t i = 0; i < settingsStringCount; i++) {
            String hashString = String(settingsString[i].hash);
            if (jsonDoc.containsKey(hashString)) {
                settingsString[i].set(jsonDoc[hashString].as<String>());
            }
        }
        return true;
    }

    void exportToJson(JsonDocument &jsonDoc) {
        for (uint8_t i = 0; i < settingsBoolCount; i++)
            jsonDoc[String(settingsBool[i].hash)] = *settingsBool[i].value; // Dereference
        for (uint8_t i = 0; i < settingsIntCount; i++)
            jsonDoc[String(settingsInt[i].hash)] = *settingsInt[i].value; // Dereference
        for (uint8_t i = 0; i < settingsStringCount; i++)
            jsonDoc[String(settingsString[i].hash)] = *settingsString[i].value; // Dereference
    }

};


class Config {
    private:
        JsonDocument jsonDoc;

        boolean fileSystemOK = false;
        bool isReadyFS();

        bool readFile(const char *filename, std::function<bool(File& file)> writerFunc);
        bool writeFile(const char *filename, std::function<bool(File& file)> writerFunc);
        void removeFile(const char *fileName);

    public:
        
        void setup();

        void readCfg(CfgStructJsonInterface &cfg);
        void writeCfg(CfgStructJsonInterface &cfg);

        // void removeCustomCfg(const char *cfgName);
        void factoryResetDevice();

        bool readFileToJson(const char *fileName);
        void writeJsonToFile(const char *fileName);

        // Templates need to be in the header to be included every time, or explicitly called in cpp to exist everywhere 
        // https://stackoverflow.com/questions/8752837/undefined-reference-to-template-class-constructor

        // template <class T> // Single value variables of any type except char*, char[n]
        // bool cfgReadGetValue(const char *varName, T &dest) {
        //     if (!jsonDoc.containsKey(varName) || !jsonDoc[varName].is<T>())
        //         return false;
        //     dest = jsonDoc[varName].as<T>(); // .as<T>() needd for String type
        //     return true;
        // };

        // template <class T> // Arrays
        // bool cfgReadGetValue(const char *varName, T &dest, uint8_t arraySize) {
        //     // Assigns values only if varName exists
        //     if (!jsonDoc.containsKey(varName) || !jsonDoc[varName].is<JsonArray>())
        //         return false;
        //     JsonArray jsonArray = jsonDoc[varName].as<JsonArray>();
        //     if (jsonArray.size() != arraySize) // Make sure size is correct to not have memory issues
        //         return false;
        //     uint8_t i = 0;
        //     for(JsonVariant value : jsonArray) {
        //         // if (!value.is<T>())
        //         //     return false;
        //         dest[i++] = value; // .as<T>() not defined for uint16_t and some others
        //     }
        // };

        // template <class T> // Single value variables of any type
        // void cfgWriteAddValue(const char *varName, T content) {
        //     jsonDoc[String(varName)] = content;
        // };
        // template <class T> // Arrays
        // void cfgWriteAddValue(const char *varName, T *content, uint8_t arraySize) {
        //     JsonArray data = jsonDoc.createNestedArray(varName);
        //     for (uint8_t i = 0; i < arraySize; i++) {
        //         data.add(*(content + i));
        //     }
        // };
};

#endif