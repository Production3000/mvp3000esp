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

// Interface for exporting and importing configuration data to/from JSON
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

// Configuration structure
struct Cfg : public CfgStructJsonInterface {

    Helper helper;

    // Core setting structure for checking and setting values
    template <typename T>
    struct SettingCore {
        T *value;
        std::function<bool(T)> checkValue;
        SettingCore() {}; // Default constructor needed for some reason
        SettingCore(T *_value, std::function<bool(T)> _checkValue) : value(_value), checkValue(_checkValue) { };

        /**
         * @brief Check if the value is valid and set it if so.
         * 
         * @param _value The value to check and set.
         * @return True if the value is valid and was set, false otherwise.
         */
        bool checkValueAndSet(T _value) {
            if (!checkValue(_value))
                return false;
            *value = _value;
            return true;
        }
    };

    // Main setting structure
    struct SettingMain {
        uint32_t hash; // Hash of the key

        uint8_t type; // 0 = boolean, 1 = int, 2 = String
        union { // Pointer to the value
            SettingCore<boolean>* b;
            SettingCore<uint16_t>* i;
            SettingCore<String>* s;
        } settingCore;

        // Minimalistic linked list
        SettingMain* next;

        /**
         * @brief Default constructor
         * Automatically sets the type based on the value type.
         * 
         * @param _hash The hash of the key-string.
         * @param _value The value of the setting.
         * @param checkValue A function to check if the value is valid.
         */
        SettingMain() {}; // Default constructor
        SettingMain(uint32_t _hash, boolean *_value, std::function<bool(boolean)> checkValue) : hash(_hash), type(0) {
            settingCore.b = new SettingCore<boolean>(_value, checkValue);
        };
        SettingMain(uint32_t _hash, uint16_t *_value, std::function<bool(uint16_t)> checkValue) : hash(_hash), type(1) {
            settingCore.i = new SettingCore<uint16_t>(_value, checkValue);
        };
        SettingMain(uint32_t _hash, String *_value, std::function<bool(String)> checkValue) : hash(_hash), type(2) {
            settingCore.s = new SettingCore<String>(_value, checkValue);
        };
    };

    // Minimalistic linked list
    SettingMain* head;

    /**
     * @brief Add a setting to the configuration.
     * 
     * @tparam T The type of the setting.
     * @param key The key of the setting.
     * @param value The value of the setting.
     * @param checkValue A function to check if the value is valid.
     */
    template <typename T>
    void addSetting(String key, T *value, std::function<bool(T)> checkValue) {
        SettingMain* newSetting = new SettingMain(helper.hashStringDjb2(key.c_str()), value, checkValue);
        newSetting->next = head;
        head = newSetting;
    }

    /**
     * @brief Export the configuration data to a JSON document.
     * 
     * @param jsonDoc The JSON document to export the data to.
     */
    void exportToJson(JsonDocument &jsonDoc) {
        SettingMain* current = head;
        while (current != NULL) {
            switch (current->type) {
                case 0:
                    jsonDoc[String(current->hash)] = *current->settingCore.b->value; // Dereference
                    break;
                case 1:
                    jsonDoc[String(current->hash)] = *current->settingCore.i->value; // Dereference
                    break;
                case 2:
                    jsonDoc[String(current->hash)] = *current->settingCore.s->value; // Dereference
                    break;
            }
            current = current->next;
        }
    }

    /**
     * @brief Import the configuration data from a JSON document.
     * 
     * @param jsonDoc The JSON document to import the data from.
     */
    bool importFromJson(JsonDocument &jsonDoc) {
        // Loop through all settings and compare with hashes in JSON document
        bool success = true;
        SettingMain* current = head;
        while (current != NULL) {
            String hashString = String(current->hash);
            if (jsonDoc.containsKey(hashString)) {
                // Matching hash found, switch to type and check/set the value
                switch (current->type) {
                    case 0: // boolean
                        success &= current->settingCore.b->checkValueAndSet(jsonDoc[hashString].as<boolean>());
                        break;
                    case 1: // uint16_t
                        success &= current->settingCore.i->checkValueAndSet(jsonDoc[hashString].as<uint16_t>());
                        break;
                    case 2: // String
                        success &= current->settingCore.s->checkValueAndSet(jsonDoc[hashString].as<String>());
                        break;
                }
            }
            current = current->next;
        }
        return success;
    }

    /**
     * @brief Update a setting from a web request.
     * 
     * @param key The key of the setting to update, will be hashed.
     * @param value The new value of the setting.
     */
    bool updateFromWeb(String key, String value) {    
        // Loop through all settings to find the correct one    
        SettingMain* current = head;
        while (current != NULL) {
            if (current->hash == helper.hashStringDjb2(key.c_str())) {
                // Found the setting, switch to its type and check/set the value
                switch (current->type) {
                    case 0: // boolean
                        return current->settingCore.b->checkValueAndSet((value.toInt() != 0));
                    case 1: // uint16_t
                        return current->settingCore.i->checkValueAndSet(value.toInt());
                    case 2: // String
                        return current->settingCore.s->checkValueAndSet(value);
                }
            }
            current = current->next;
        }
        return false;
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