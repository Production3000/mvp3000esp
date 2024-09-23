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
var topic = null;

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

    // Client ID with random part to avoid AMQJS0008I Socket closed on test.mosquitto.org
    client = new Paho.MQTT.Client(websocketurl, "clientId" + (Math.random() + 1).toString(36));

    client.onConnectionLost = function(e) {
        document.getElementById('coninfo').innerHTML = "Not connected: " + e.errorMessage;
        document.getElementById('subinfo').innerHTML = "-";
        document.getElementById('ctrlinfo').innerHTML = "-";
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
    if (document.getElementById('topic').value.length == 0) {
        console.log("No topic specified");
        return;
    }
    topic = document.getElementById('topic').value + "_sensor";

    let _topic = topic + "_data";
    client.subscribe(_topic);

    if (document.getElementById('subinfo').innerHTML.length == 1) {
        document.getElementById('subinfo').innerHTML = _topic;
    } else {
        document.getElementById('subinfo').innerHTML = _topic + ", " + document.getElementById('subinfo').innerHTML;
    }

    document.getElementById('ctrlinfo').innerHTML = topic + "_ctrl";
}

function sendmessage(text) {
    if (!topic) {
        return;
    }

    let _topic = topic + "_ctrl";

    message = new Paho.MQTT.Message(text);
    message.destinationName = _topic;
    client.send(message);
};

