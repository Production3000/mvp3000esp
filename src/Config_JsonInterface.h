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

    virtual void exportToJson(JsonDocument &jsonDoc) { };
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

    struct SettingNode {
        uint32_t hash; // Hash of the var name
        void* varPtr; // Pointer to the actual value
        std::function<String()> get;
        std::function<boolean(const String&)> checkSet;

        // The get function needs to be type specific, to correctly convert the void* pointer back to the original type.
        // This cannot be templated and combined into a single linked list.
        SettingNode(const String& varName, uint8_t* _varPtr, std::function<bool(const String&)> checkSet) : hash(_helper.hashStringDjb2(varName.c_str())), varPtr(_varPtr), checkSet(checkSet) {
            get = [&]() { return String(*((uint8_t*)varPtr)); };
        };
        SettingNode(const String& varName, int16_t* _varPtr, std::function<bool(const String&)> checkSet) : hash(_helper.hashStringDjb2(varName.c_str())), varPtr(_varPtr), checkSet(checkSet) {
            get = [&]() { return String(*((int16_t*)varPtr)); };
        };
        SettingNode(const String& varName, uint16_t* _varPtr, std::function<bool(const String&)> checkSet) : hash(_helper.hashStringDjb2(varName.c_str())), varPtr(_varPtr), checkSet(checkSet) {
            get = [&]() { return String(*((uint16_t*)varPtr)); };
        };
        SettingNode(const String& varName, String* _varPtr, std::function<bool(const String&)> checkSet) : hash(_helper.hashStringDjb2(varName.c_str())), varPtr(_varPtr), checkSet(checkSet) {
            get = [&]() { return *((String*)varPtr); };
        };
        SettingNode(const String& varName, boolean* _varPtr, std::function<bool(const String&)> checkSet) : hash(_helper.hashStringDjb2(varName.c_str())), varPtr(_varPtr), checkSet(checkSet) {
            get = [&]() { return String(*((boolean*)varPtr)); };
        };

        SettingNode* next = nullptr;
    };

    SettingNode* head = nullptr; // First added setting
    SettingNode* tail = nullptr;

    /**
     * @brief Add a setting to the configuration.
     *
     * @tparam T The type of the setting variable.
     * @param varName The key of the setting.
     * @param varPtr Pointer to the setting.
     * @param checheckSetckValue A function to check if the value is valid.
     */
    template <typename T>
    void addSetting(const String& varName, T* varPtr, std::function<bool(const String&)> checkSet) {
        SettingNode* newSetting = new SettingNode(varName, varPtr, checkSet);
        if (head == nullptr) {
            head = newSetting;
            tail = newSetting;
        } else {
            tail->next = newSetting;
            tail = newSetting;
        }
    }

    void exportToJson(JsonDocument &jsonDoc) {
        // Loop through all settings and add to JSON document
        SettingNode* current = head;
        while (current != nullptr) {
            jsonDoc[String(current->hash)] = current->get();
            current = current->next;
        }
    }

    bool importFromJson(JsonDocument &jsonDoc) {
        // Loop through all settings
        bool success = true;
        SettingNode* current = head;
        while (current != nullptr) {
            String hashString = String(current->hash);
            // Compare hashes
            if (jsonDoc.containsKey(hashString)) {
                success &= current->checkSet(jsonDoc[hashString].as<String>());
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
        // Loop through all settings
        uint32_t hash = _helper.hashStringDjb2(key.c_str());
        SettingNode* current = head;
        while (current != nullptr) {
            // Compare hashes
            if (current->hash == hash) {
                return current->checkSet(value);
            }
            current = current->next;
        }
        return false;
    }
};

#endif