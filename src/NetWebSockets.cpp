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

#include "NetWebSockets.h"

#include "MVP3000.h"
extern MVP3000 mvp;


void NetWebSockets::setup() {
    if (webSocketState == WEBSOCKET_STATE::HARDDISABLED)
        webPage = webPageHardDisabled;
}

void NetWebSockets::printWebSocket(const String& uri, const String& message) {
    if (webSocketState == WEBSOCKET_STATE::HARDDISABLED)
        return;
    linkedListWebSocket.printWebSocket(uri, message);
}

void NetWebSockets::registerWebSocket(const String& uri, WebSocketCtrlCallback ctrlCallback) {
    if (webSocketState == WEBSOCKET_STATE::HARDDISABLED)
        return;
    linkedListWebSocket.appendUnique(uri, ctrlCallback, std::bind(&NetWebSockets::webSocketEventCallbackWrapper, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), &server);
};

void NetWebSockets::webSocketEventCallbackWrapper(AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len, WebSocketCtrlCallback ctrlCallback) {
    switch (type) {
        case WS_EVT_CONNECT:
            mvp.logger.writeFormatted(CfgLogger::Level::INFO, "WS client %d connected from: %s", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            mvp.logger.writeFormatted(CfgLogger::Level::INFO, "WS client %d disconnected.", client->id()); // No IP available
            break;
        case WS_EVT_ERROR:
            mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "WS error from: %s, client %d", client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DATA:
            mvp.logger.writeFormatted(CfgLogger::Level::INFO, "WS client %d data from: %s", client->id(), client->remoteIP().toString().c_str());
            if (ctrlCallback != nullptr) { // Only parse data if there is something to do
                AwsFrameInfo *info = (AwsFrameInfo*)arg;
                if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                    data[len] = 0; // Terminate string
                    // Execute callback
                    ctrlCallback((char*)data);
                }
            }
            break;
        default: // WS_EVT_PONG
            break;
    }
}


///////////////////////////////////////////////////////////////////////////////////

String NetWebSockets::templateProcessor(uint8_t var) {
    switch (var) {        
        case 80: // Filling of the websocket topics is better be split, long strings are never good during runtime
            if (linkedListWebSocket.getSize() == 0) {
                return "[No entries]";
            }
            // Set initial bookmark
            linkedListWebSocket.bookmarkByIndex(0);
        case 81:
            return _helper.printFormatted("<li>ws://%%2%%%s</li>%s",
                linkedListWebSocket.getBookmarkData()->uri.c_str(),
                (linkedListWebSocket.moveBookmark()) ? "%81%" : ""); // Recursive call if there are more entries

        default:
            return "";
    }
}