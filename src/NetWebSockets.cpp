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


void NetWebSockets::loop() {
    if (ctrlUserCallback != nullptr) {
        // IMPORTANT: Execute the callback from the main loop, separated from the event
        ctrlUserCallback(ctrlData);
        ctrlUserCallback = nullptr;
    }
};


void NetWebSockets::registerWebSocket(const String& uri, NetworkCtrlCallback ctrlCallback) {
    if (webSocketState == WEBSOCKET_STATE::HARDDISABLED)
        return;
    linkedListWebSocket.appendUnique(uri, ctrlCallback, std::bind(&NetWebSockets::ctrlCbWrapper, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), &server);
};

void NetWebSockets::ctrlCbWrapper(AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len, NetworkCtrlCallback ctrlCallback) {
    switch (type) {
        case WS_EVT_CONNECT:
            mvp.logger.writeFormatted(CfgLogger::Level::INFO, "WS client %d connected from: %s", client->id(), client->remoteIP().toString().c_str());
            // IMPORTANT: Execute the callback from the main loop, separated from the event
            ctrlData = "CONNECT";
            ctrlUserCallback = ctrlCallback;
            break;
        case WS_EVT_DISCONNECT:
            mvp.logger.writeFormatted(CfgLogger::Level::INFO, "WS client %d disconnected.", client->id()); // No IP available

            // client->close();                                                 // TODO
            // client->server()->cleanupClients();
            // client->server()->_cleanBuffers();
            // client->server()->_handleDisconnect(client);                    

            break;
        case WS_EVT_ERROR:
            mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "WS error from: %s, client %d", client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DATA:
            mvp.logger.writeFormatted(CfgLogger::Level::INFO, "WS client %d data from: %s", client->id(), client->remoteIP().toString().c_str());
            if (ctrlCallback != nullptr) { // Only parse data if there is something to do
                AwsFrameInfo *info = (AwsFrameInfo*)arg;
                if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                    // IMPORTANT: Execute the callback from the main loop, separated from the event
                    data[len] = 0; // Terminate string
                    ctrlData = String((char*)data);
                    ctrlUserCallback = ctrlCallback;
                }
            }
            break;
        default: // WS_EVT_PONG
            break;
    }
}

void NetWebSockets::printWebSocket(const String& uri, const String& message) {
    if (webSocketState == WEBSOCKET_STATE::HARDDISABLED)
        return;
    linkedListWebSocket.printWebSocket(uri, message);
}


///////////////////////////////////////////////////////////////////////////////////
#include "NetWeb_HtmlStrings.h"

PGM_P NetWebSockets::getHtml() {
    if (webSocketState == WEBSOCKET_STATE::HARDDISABLED)
        return htmlNetWebSocketsDisabled;
    else
        return htmlNetWebSockets;
}

String NetWebSockets::templateProcessor(uint16_t var) {
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
