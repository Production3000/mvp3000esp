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


typedef std::function<String(int)> WebArgKeyValue;
typedef std::function<bool(int, WebArgKeyValue, WebArgKeyValue)> WebActionFunction;

// Linked list for action callbacks
struct WebActionList {
    enum ResponseType {
        NONE = 0,
        MESSAGE = 1,
        RESTART = 2
    };

    struct Node {
        String action;
        ResponseType successResponse;
        String successMessage;
        WebActionFunction actionFkt;
        
        Node* next;

        Node() { }
        Node(String _action, ResponseType _successResponse, WebActionFunction _actionFkt, String _successMessage) : action(_action), successResponse(_successResponse), actionFkt(_actionFkt), successMessage(_successMessage) { }
    };

    Node* head = nullptr;


    void add(String action, ResponseType successResponse, WebActionFunction actionFkt, String successMessage) {
        Node* newNode = new Node(action, successResponse, actionFkt, successMessage);
        newNode->next = head;
        head = newNode;
    }

    // Loops through all cfgs and updates the value if found
    Node* loopActions(int args, WebArgKeyValue argKey, WebArgKeyValue argValue) {
        Node* current = head;
        // Loop through all nodes
        while (current != nullptr) {
            // Check if key matches and execute action
            if (argKey(0) == current->action) {
                if (current->actionFkt(args, argKey, argValue)) {
                    return current;
                } else {
                    return nullptr;
                }
            }
            current = current->next;
        }
        return nullptr;
    }
};


///////////////////////////////////////////////////////////////////////////////////

// Linked list for configuration objects
struct WebCfgList {
    struct Node {
        CfgJsonInterface* Cfg;
        std::function<void()> callback;
        Node* next;
    };
    Node* head = nullptr;

    std::function<void(CfgJsonInterface&)> saveCfgFkt; // Function to save the configuration

    WebCfgList() { }
    WebCfgList(std::function<void(CfgJsonInterface&)> saveCfgFkt) {
        this->saveCfgFkt = saveCfgFkt;
    }

    void add(CfgJsonInterface* Cfg, std::function<void()> callback) {
        Node* newNode = new Node;
        newNode->Cfg = Cfg;
        newNode->next = head;
        newNode->callback = callback;
        head = newNode;
    }

    // Loops through all cfgs and updates the value if found
    bool loopUpdateSingleValue(String key, String value) {
        Node* current = head;
        // Loop through all nodes
        while (current != nullptr) {
            // Try to update value, if successful save Cfg and return
            if (current->Cfg->updateSingleValue(key, value)) {
                saveCfgFkt(*current->Cfg);
                if (current->callback != nullptr) {
                    current->callback();
                }
                return true;
            }
            current = current->next;
        }
        return false;
    }
};


///////////////////////////////////////////////////////////////////////////////////

typedef std::function<String (uint8_t)> AwsTemplateProcessorInt;
typedef std::function<String (const String &, AwsTemplateProcessorInt)> AwsTemplateProcessorWrapper;

// Collection of web pages
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