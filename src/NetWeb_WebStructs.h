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
typedef std::function<bool(int, WebArgKeyValue, WebArgKeyValue)> WebActionFunction;


// Linked list for web actions
struct DataStructWebAction {
    enum ResponseType {
        NONE = 0,
        MESSAGE = 1,
        RESTART = 2
    };

    String actionKey;
    ResponseType successResponse;
    String successMessage;
    WebActionFunction actionFkt;

    DataStructWebAction(const String& actionKey) : actionKey(actionKey) { } // For comparision only
    DataStructWebAction(const String& actionKey, ResponseType successResponse, WebActionFunction actionFkt, const String& successMessage) : actionKey(actionKey), successResponse(successResponse), actionFkt(actionFkt), successMessage(successMessage) { }

    bool equals(DataStructWebAction* other) {
        if (other == nullptr)
            return false;
        // Compare the actionKey string
        return actionKey.equals(other->actionKey);
    }
};

struct LinkedListWebActions : LinkedList3101<DataStructWebAction> {
    LinkedListWebActions() : LinkedList3101<DataStructWebAction>() { }
    LinkedListWebActions(uint16_t size) : LinkedList3101<DataStructWebAction>(size) { }

    void appendUnique(const String& actionKey, DataStructWebAction::ResponseType successResponse, WebActionFunction actionFkt, const String& successMessage) {
        this->appendUniqueDataStruct(new DataStructWebAction(actionKey, successResponse, actionFkt, successMessage));
    }

    DataStructWebAction* findAction(const String& argKey) {
        Node* node = this->findByContent(new DataStructWebAction(argKey));
        if (node == nullptr) {
            return nullptr;
        }
        return node->dataStruct;
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
    // Function to save the configuration
    std::function<void(CfgJsonInterface&)> saveCfgFkt;

    void setSaveCfgFkt(std::function<void(CfgJsonInterface&)> saveCfgFkt) {
        this->saveCfgFkt = saveCfgFkt;
    }

    void append(CfgJsonInterface* cfg, std::function<void()> callback) {
        this->appendDataStruct(new DataStructWebCfg(cfg, callback));
    }

    bool updateSetting(const String& key, const String& value) {
        boolean success = false;
        this->loop([&](DataStructWebCfg* current, uint16_t i) {
            // Try to update value, if successful save Cfg and return
            if (current->cfg->updateSingleValue(key, value)) {
                saveCfgFkt(*current->cfg);
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


///////////////////////////////////////////////////////////////////////////////////

typedef std::function<void(AsyncWebSocketClient *, AwsEventType)> WebSocketEventLog;
typedef std::function<void(char*)> WebSocketDataCallback;

// Collection of websockets
struct WebSocketColl {
    struct Node {
        AsyncWebSocket* websocket;

        WebSocketDataCallback dataCallback;
        WebSocketEventLog webSocketEventLog;

        Node() { }
        Node(const String& uri, WebSocketDataCallback dataCallback, WebSocketEventLog webSocketEventLog, AsyncWebServer *server) : dataCallback(dataCallback), webSocketEventLog(webSocketEventLog) {
            websocket = new AsyncWebSocket(uri);
            // Event log and custom handerl
            websocket->onEvent([&](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
                // General event log
                webSocketEventLog(client, type);
                // Custom callback for data input from websocket
                if ((type == WS_EVT_DATA) && (dataCallback != nullptr)) { // Only parse data if there is something to do
                    AwsFrameInfo *info = (AwsFrameInfo*)arg;
                    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                        data[len] = 0; // Terminate string
                        // Execute callback
                        dataCallback((char*)data);
                    }
                }
            });
            // Attach
            server->addHandler(websocket);
        }

        std::function<void(const String& message)> getTextAll() { return std::bind(&Node::textAll, this, std::placeholders::_1); }
        void textAll(const String& message) {
            websocket->textAll(message);
        }
    };

    static const uint8_t nodesSize = 3;
    uint8_t nodeCount = 0;

    Node* nodes[nodesSize];

    AsyncWebServer *server;
    WebSocketEventLog webSocketEventLog;


    WebSocketColl(AsyncWebServer *server, WebSocketEventLog webSocketEventLog) : server(server), webSocketEventLog(webSocketEventLog) { }


    bool add(const String& uri, WebSocketDataCallback dataCallback) {
        if (nodeCount >= nodesSize) {
            return false;
        }
        nodes[nodeCount++] = new Node(uri, dataCallback, webSocketEventLog, server);
        return true;
    }

    std::function<void(const String& message)> getTextAll() {
        return (nodeCount > 0) ? nodes[nodeCount - 1]->getTextAll() : nullptr;
    }
};


#endif