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

var keepData = true;

window.addEventListener('load', () => {
    document.getElementById('connect').addEventListener('click', connect);
} );

function connect() {
    document.getElementById('coninfo').innerHTML = "Connecting ...";

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
        if (keepData)
            document.getElementById('data').innerHTML = e.data + "\n" + document.getElementById('data').innerHTML;
        else
            document.getElementById('data').innerHTML = e.data;
    }
    websocket.onerror = function() {
        document.getElementById('coninfo').innerHTML = "Error";
        console.log('Connection error');
    };
}
