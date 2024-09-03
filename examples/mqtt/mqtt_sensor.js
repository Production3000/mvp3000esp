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


var client = null;

window.addEventListener('load', initWebSocket);

function initWebSocket() {
    document.getElementById('connect').addEventListener('click', connect);
    document.getElementById('subscribe').addEventListener('click', subscribe);
    document.getElementById('tare').addEventListener('click', () => { sendmessage('TARE') });
    document.getElementById('clear').addEventListener('click', () => { sendmessage('CLEAR') });
}

function connect() {
    document.getElementById('coninfo').innerHTML = "Connecting ...";

    if (client)
        client.disconnect();

    let websocketurl = document.getElementById('websocketurl').value;
    // Check if there already is a ws:// or wss:// prefix
    if (!websocketurl.startsWith('ws://') && !websocketurl.startsWith('wss://')) {
        websocketurl = 'ws://' + websocketurl;
    }

    client = new Paho.MQTT.Client(websocketurl, "clientId");

    client.onConnectionLost = function(e) {
        document.getElementById('coninfo').innerHTML = "Not connected";
        document.getElementById('subinfo').innerHTML = "-";
        console.log('Connection closed: ' + e.errorMessage);
    }

    client.onMessageArrived = function(e) {
        document.getElementById('data').innerHTML = e.payloadString;
    }

    // For some reason the onConnect needs to be passed here and cannot be defined separately like the others
    client.connect({onSuccess:onConnect});
}

function onConnect() {
    document.getElementById('coninfo').innerHTML = "Connected";
    subscribe();
}

function subscribe() {
    let topic = document.getElementById('topic').value;
    if (!topic) {
        return;
    }
    topic += "_sensor";

    client.subscribe(topic);

    if (document.getElementById('subinfo').innerHTML.length == 1) {
        document.getElementById('subinfo').innerHTML = topic;
    } else {
        document.getElementById('subinfo').innerHTML += ", " + topic;
    }
}

function sendmessage(text) {
    let topic = document.getElementById('topic').value;
    if (!topic) {
        return;
    }

    message = new Paho.MQTT.Message(text);
    message.destinationName = topic + "_ctrl";
    client.send(message);

    console.log('Message sent: ' + text);
};

