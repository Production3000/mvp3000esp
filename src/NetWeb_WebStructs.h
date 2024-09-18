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

struct LinkedListWebActions : LinkedListNEW3101<DataStructWebAction> {
    LinkedListWebActions() : LinkedListNEW3101<DataStructWebAction>() { }
    LinkedListWebActions(uint16_t size) : LinkedListNEW3101<DataStructWebAction>(size) { }

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

struct LinkedListWebCfg : LinkedListNEW3100<DataStructWebCfg> {
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

typedef std::function<String (uint8_t)> AwsTemplateProcessorInt;
typedef std::function<String (const String&, AwsTemplateProcessorInt)> AwsTemplateProcessorWrapper;

// Collection of web pages TODO for some reason this does not work in a linked list ???
struct WebPageColl {
    struct Node {
        String uri;
        const char* html;
        String contentType;
        AwsResponseFiller responseFiller;

        AwsTemplateProcessor processor;
        AwsTemplateProcessorWrapper processorMain;
        AwsTemplateProcessorInt processorCustom;

        Node () { }
        Node(String _uri, const char* _html, String _contentType, AwsTemplateProcessorInt _processorCustom, AwsTemplateProcessorWrapper _processorMain, AsyncWebServer *server) : uri(_uri), html(_html), contentType(_contentType), processorCustom(_processorCustom), processorMain(_processorMain) { 
            processor = std::bind(&Node::htmlTemplateProcessor, this, std::placeholders::_1);
            responseFiller = std::bind(&Node::htmlTemplateResponseFiller, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            attach(server);
        }
        Node(String _uri, String _contentType, AwsResponseFiller _responseFiller, AsyncWebServer *server) : uri(_uri), html(""), contentType(_contentType), responseFiller(_responseFiller), processor(nullptr) {
            attach(server);
        }

        void attach(AsyncWebServer *server) {
            server->on(uri.c_str(), HTTP_GET, [&](AsyncWebServerRequest *request) {
                request->sendChunked(contentType, responseFiller, processor);
            });
        }

        String htmlTemplateProcessor(const String& var) {
            return processorMain(var, processorCustom);
        }

        size_t htmlTemplateResponseFiller(uint8_t *buffer, size_t maxLen, size_t index) {
            // Chunked response filler for the html template
            size_t len = strlen(html);
            if (index + maxLen > len) {
                maxLen = len - index;
            }
            memcpy(buffer, html + index, maxLen);
            return maxLen;
        }
    };

    static const uint8_t nodesSize = 10;
    uint8_t nodeCount = 0;

    Node* nodes[nodesSize];

    AsyncWebServer *server;

    AwsTemplateProcessorWrapper processorMain;


    WebPageColl(AsyncWebServer *_server) : server(_server) { }
    WebPageColl(AsyncWebServer *_server, AwsTemplateProcessorWrapper _processorMain) : server(_server), processorMain(_processorMain) { }


    bool add(String uri, const char* _html, AwsTemplateProcessorInt processor, String contentType) {
        if (nodeCount >= nodesSize) {
            return false;
        }
        nodes[nodeCount++] = new Node(uri, _html, contentType, processor, processorMain, server);
        return true;
    }
    bool add(String uri, AwsResponseFiller responseFiller, String contentType) {
        if (nodeCount >= nodesSize) {
            return false;
        }
        nodes[nodeCount++] = new Node(uri, contentType, responseFiller, server);
        return true;
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
        Node(String uri, WebSocketDataCallback _dataCallback, WebSocketEventLog _webSocketEventLog, AsyncWebServer *server) : dataCallback(_dataCallback), webSocketEventLog(_webSocketEventLog) {
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

        std::function<void(const String &message)> getTextAll() { return std::bind(&Node::textAll, this, std::placeholders::_1); }
        void textAll(const String &message) {
            websocket->textAll(message);
        }
    };

    static const uint8_t nodesSize = 3;
    uint8_t nodeCount = 0;

    Node* nodes[nodesSize];

    AsyncWebServer *server;
    WebSocketEventLog webSocketEventLog;


    WebSocketColl(AsyncWebServer *_server, WebSocketEventLog _webSocketEventLog) : server(_server), webSocketEventLog(_webSocketEventLog) { }


    bool add(String uri, WebSocketDataCallback dataCallback) {
        if (nodeCount >= nodesSize) {
            return false;
        }
        nodes[nodeCount++] = new Node(uri, dataCallback, webSocketEventLog, server);
        return true;
    }

    std::function<void(const String &message)> getTextAll() {
        return (nodeCount > 0) ? nodes[nodeCount - 1]->getTextAll() : nullptr;
    }
};


#endif