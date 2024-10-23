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

#ifndef MVP3000_NETWEBSOCKETS
#define MVP3000_NETWEBSOCKETS

#include <Arduino.h>

#include <ESPAsyncWebServer.h>

#include "NetWeb_WebStructs.h"

typedef std::function<void(const String&)> NetworkCtrlCallback;
typedef std::function<void(AsyncWebSocketClient *, AwsEventType, void*, uint8_t*, size_t, NetworkCtrlCallback)> WebSocketCtrlCbWrapper;


class NetWebSockets {
    public:

        /**
         * @brief Register a websocket to be used with the web interface.
         *
         * @param uri The URI of the websocket.
         * @param dataCallback (optional) The function to execute when data is received. Leave empty to not execute a function.
         * @return Returns the function to write data to the websocket.
         */
        void registerWebSocket(const String& uri, NetworkCtrlCallback dataCallback = nullptr);

        /**
         * @brief Write data to a websocket.
         *
         * @param uri The URI of the websocket.
         * @param message The message to write.
         */
        void printWebSocket(const String& uri, const String& message);

    public:

        NetWebSockets(AsyncWebServer& server) : server(server) { }
        ~NetWebSockets() { }

        // void setup() { };
        void loop();

        void hardDisable() { webSocketState = WEBSOCKET_STATE::HARDDISABLED; }

    private:

        enum class WEBSOCKET_STATE: uint8_t {
            HARDDISABLED = 0, // DISABLED is reserved
            ENABLED = 1,
        };
        WEBSOCKET_STATE webSocketState = WEBSOCKET_STATE::ENABLED;

        struct DataStructSocketPack {
            String uri;

            AsyncWebSocket* webSocket;
            NetworkCtrlCallback ctrlCallback;
            WebSocketCtrlCbWrapper ctrlCbWrapper;

            DataStructSocketPack(const String& uri) : uri(uri) { };
            DataStructSocketPack(const String& _uri, NetworkCtrlCallback _ctrlCallback, WebSocketCtrlCbWrapper _ctrlCbWrapper, AsyncWebServer *server) : uri(_uri), ctrlCallback(_ctrlCallback), ctrlCbWrapper(_ctrlCbWrapper) {
                // Create websocket and attached to server
                webSocket = new AsyncWebSocket(_uri);
                webSocket->onEvent([&](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
                    // IMPORTANT: Execute the callback from the main loop, separated from the event
                    // Any delay() in any called function will crash the ESP8266 (ESP32 untested)
                    // The reason is unclear, maybe the wrapped vTaskDelay()
                    ctrlCbWrapper(client, type, arg, data, len, ctrlCallback);
                });
                server->addHandler(webSocket);
            }
        };

        struct LinkedListWebSocket : LinkedList3111<DataStructSocketPack> {
            void appendUnique(const String& uri, NetworkCtrlCallback ctrlCallback, WebSocketCtrlCbWrapper ctrlCbWrapper, AsyncWebServer* server) {
                this->appendUniqueDataStruct(new DataStructSocketPack(uri, ctrlCallback, ctrlCbWrapper, server));
            }

            DataStructSocketPack* findUri(const String& uri) {
                return this->findByContentData(DataStructSocketPack(uri));
            }

            boolean compareContent(DataStructSocketPack* dataStruct, DataStructSocketPack* other) override {
                return dataStruct->uri.equals(other->uri);
            }

            void printWebSocket(const String& uri, const String& message) {
                // Find the uri in the list
                DataStructSocketPack* socketPack = findUri(uri);
                if (socketPack != nullptr)
                    socketPack->webSocket->textAll(message);
            }
        };

        AsyncWebServer& server;

        LinkedListWebSocket linkedListWebSocket; // Adaptive size

        String ctrlData = "";
        NetworkCtrlCallback ctrlUserCallback = nullptr;
        void ctrlCbWrapper(AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len, NetworkCtrlCallback ctrlCallback);

    public:

        String templateProcessor(uint16_t var);
        PGM_P getHtml();
};

#endif