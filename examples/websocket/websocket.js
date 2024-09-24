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

var websocket;

window.addEventListener('load', () => {
    document.getElementById('connect').addEventListener('click', connect);
} );

function connect() {
    document.getElementById('coninfo').innerHTML = "Connecting ...";
    writeData("");

    if (websocket)
        websocket.close();

    let deviceIp = document.getElementById('deviceip').value;
    let wsfolder = document.getElementById('wsfolder').value;

    let websocketurl = "ws://" + deviceIp.trim() + wsfolder;

    websocket = new WebSocket(websocketurl);

    websocket.onopen = function() {
        document.getElementById('coninfo').innerHTML = "Connected";
        console.log('Connection opened');
    }
    websocket.onclose = function() {
        document.getElementById('coninfo').innerHTML = "Not connected";
        console.log('Connection closed');
    }
    websocket.onmessage = function(e) {
        // Check if node is span or textarea
        // document.getElementById('data').tagName
        if (document.getElementById('data').tagName == "SPAN") {
            document.getElementById('data').innerHTML = e.data;
            return;
        }

        writeData(e.data);
    }
    websocket.onerror = function() {
        document.getElementById('coninfo').innerHTML = "Error";
        console.log('Connection error');
    };
}

function writeData(str) {
    if (document.getElementById('data').tagName == "TEXTAREA") {
        if (str.length == 0)
            document.getElementById('data').value = str;
        else
            document.getElementById('data').value = str + "\n" + document.getElementById('data').value;
    } else {
        document.getElementById('data').innerHTML = str;
    }
}
