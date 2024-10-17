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

#ifndef MVP3000_NETWEB_STRUCTS
#define MVP3000_NETWEB_STRUCTS

#include <Arduino.h>

#include <ESPAsyncWebServer.h>

#include "Config_JsonInterface.h"

#include "_Helper_LinkedList.h"


typedef std::function<String(int)> WebArgKeyValue;
typedef std::function<bool(int, WebArgKeyValue, WebArgKeyValue)> WebActionCallback;


// Linked list for web actions
struct DataStructWebAction {
    uint32_t actionKeyHash;

    uint8_t responseType;
    String successMessage;
    WebActionCallback actionCallback;

    DataStructWebAction(const String& actionKey) : actionKeyHash(_helper.hashStringDjb2(actionKey.c_str())) { } // For comparision only
    DataStructWebAction(const String& actionKey, uint8_t responseType, WebActionCallback actionCallback, const String& successMessage) : actionKeyHash(_helper.hashStringDjb2(actionKey.c_str())), responseType(responseType), actionCallback(actionCallback), successMessage(successMessage) { }
};

struct LinkedListWebActions : LinkedList3101<DataStructWebAction> {
    LinkedListWebActions() : LinkedList3101<DataStructWebAction>() { }

    enum ResponseType: uint8_t {
        NONE = 0,
        MESSAGE = 1,
        RESTART = 2
    };

    void appendUnique(const String& actionKey, ResponseType responseType, WebActionCallback actionCallback, const String& successMessage) {
        this->appendUniqueDataStruct(new DataStructWebAction(actionKey, responseType, actionCallback, successMessage));
    }

    DataStructWebAction* findAction(const String& argKey) {
        return this->findByContentData(DataStructWebAction(argKey));
    }

    boolean compareContent(DataStructWebAction* dataStruct, DataStructWebAction* other) override {
        return dataStruct->actionKeyHash == other->actionKeyHash;
    }
};


///////////////////////////////////////////////////////////////////////////////////

// Linked list for configuration objects
struct DataStructWebCfg {
    CfgJsonInterface* cfg;
    std::function<void()> callback;

    DataStructWebCfg(CfgJsonInterface* cfg, std::function<void()> callback) : cfg(cfg), callback(callback) { }
};

struct LinkedListWebCfg : LinkedList3100<DataStructWebCfg> {
    void append(CfgJsonInterface* cfg, std::function<void()> callback) {
        this->appendDataStruct(new DataStructWebCfg(cfg, callback));
    }

    bool updateSetting(const String& key, const String& value, std::function<void(CfgJsonInterface&)> saveCfgFkt) {
        boolean success = false;
        this->loop([&](DataStructWebCfg* current, uint16_t i) {
            // Try to update value, if successful save Cfg and return
            if (current->cfg->updateSingleValue(key, value)) {
                saveCfgFkt(*current->cfg);
                // Call after-save callback, if available
                if (current->callback != nullptr) {
                    current->callback();
                }
                success = true; // set flag
                return;
            }
        });
        return success;
    }
};


#endif