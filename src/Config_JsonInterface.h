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

#ifndef MVP3000_CONFIG_JSONINTERFACE
#define MVP3000_CONFIG_JSONINTERFACE

#include <Arduino.h>
#include <ArduinoJson.h>

#include "_Helper.h"
extern _Helper _helper;


/**
 * @brief General interface for exporting and importing configuration data to/from JSON.
 *
 * It is the basis for the CfgJsonInterface configuration structure.
 * It can be extended to work with more complex data structures, like arrays or nested objects.
 *
 * @param _cfgName The name of the configuration file.
 */
struct JsonInterface {
    String cfgName;

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

    JsonInterface(const String& cfgName) : cfgName(cfgName) { };

};



/**
 * @brief Configuration structure to hold single value configuration items
 *
 * The structure is designed to be extended with class/module specific settings.
 * The values are stored in the main program and the structure is used to manage them.
 * It offser functions to read/write the settings to/from a JSON file.
 *
 * @param _cfgName The name of the configuration file.
 */
struct CfgJsonInterface : public JsonInterface {
    CfgJsonInterface(const String& cfgName) : JsonInterface(cfgName) { };

    // Type-specific core setting structure
    template <typename T>
    struct SettingCore {
        T *value; // This is a pointer to the actual value
        std::function<bool(T)> checkValue;

        SettingCore(T *_value, std::function<bool(T)> _checkValue) : value(_value), checkValue(_checkValue) { };

        bool checkValueAndSet(T _value) {
            if (!checkValue(_value))
                return false;
            *value = _value;
            return true;
        }
    };

    // Main setting structure, minimalistic linked list
    struct SettingNode {
        uint32_t hash; // Hash of the key

        uint8_t type; // 0 = boolean, 1 = int, 2 = String
        union { // Pointer to the type-specific setting core
            SettingCore<boolean>* b;
            SettingCore<uint16_t>* i;
            SettingCore<String>* s;
        } settingCore;

        SettingNode* next = nullptr;

        /**
         * @brief Default constructor, overloaded to select the type-specific settings core based on the value type.
         *
         * @param _hash The hash of the key-string.
         * @param _value The value of the setting.
         * @param checkValue A function to check if the value is valid.
         */
        SettingNode() {}; // Default constructor
        SettingNode(uint32_t _hash, boolean *_value, std::function<bool(boolean)> checkValue) : hash(_hash), type(0) {
            settingCore.b = new SettingCore<boolean>(_value, checkValue);
        };
        SettingNode(uint32_t _hash, uint16_t *_value, std::function<bool(uint16_t)> checkValue) : hash(_hash), type(1) {
            settingCore.i = new SettingCore<uint16_t>(_value, checkValue);
        };
        SettingNode(uint32_t _hash, String *_value, std::function<bool(const String&)> checkValue) : hash(_hash), type(2) {
            settingCore.s = new SettingCore<String>(_value, checkValue);
        };
    };

    // Minimalistic linked list
    SettingNode* head = nullptr; // First added setting
    SettingNode* tail = nullptr;

    /**
     * @brief Add a setting to the configuration.
     *
     * @tparam T The type of the setting.
     * @param key The key of the setting.
     * @param value The value of the setting.
     * @param checkValue A function to check if the value is valid.
     */
    template <typename T>
    void addSetting(const String& key, T *value, std::function<bool(T)> checkValue) {
        SettingNode* newSetting = new SettingNode(_helper.hashStringDjb2(key.c_str()), value, checkValue);
        if (head == nullptr) {
            head = newSetting;
            tail = newSetting;
        } else {
            tail->next = newSetting;
            tail = newSetting;
        }
    }

    /**
     * @brief Export the configuration data to a JSON document.
     *
     * @param jsonDoc The JSON document to export the data to.
     */
    void exportToJson(JsonDocument &jsonDoc) {
        SettingNode* current = head;
        while (current != nullptr) {
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
        SettingNode* current = head;
        while (current != nullptr) {
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
                    default:
                        Serial.println("NOT IMPLEMENTED TYPE!");
                        success = false;
                }
            }
            current = current->next;
        }
        return success;
    }

    /**
     * @brief Update a single setting, typically from a web request.
     *
     * @param key The key of the setting to update, will be hashed.
     * @param value The new value of the setting.
     */
    bool updateSingleValue(const String& key, const String& value) {
        // Loop through all settings to find the correct one
        SettingNode* current = head;
        while (current != nullptr) {
            if (current->hash == _helper.hashStringDjb2(key.c_str())) {
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

#endif