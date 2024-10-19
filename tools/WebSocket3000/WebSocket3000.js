/* 
Copyright Production 3000

Licensed under the Apache License, Version 2.0 (the 'License');
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an 'AS IS' BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

WebSocket3000 = new function() {
    let $ = this;

    $.init = function(url, onconnected, ondisconnected, onmessage) {
        $.url = url;
        $.onopen = onconnected;
        $.onclose = ondisconnected;
        $.onmessage = onmessage;
        $.connect();
    };

    $.url;
    $.websocket;

    $.onopen;
    $.onclose;
    $.onmessage;

    $.connect = function() {
        $.websocket = new WebSocket(`ws://${location.host}${$.url}`);

        $.websocket.onopen = $.onopen;
        $.websocket.onclose = function() {
            $.onclose();
            $.connect();
        };
        $.websocket.onmessage = $.onmessage;
        $.websocket.onerror = function(e) {
            $.onclose();
            console.log(e);
        };
    };
};