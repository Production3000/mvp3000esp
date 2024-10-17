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

#ifndef MVP3000_NETWEB_HTMLSTRINGS
#define MVP3000_NETWEB_HTMLSTRINGS


const char htmlHead[] PROGMEM = R"===(
<!DOCTYPE html>
<html lang='en'>
<head>
    <title>MVP3000 - Device ID %1%</title>
    <script>
        function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }
        function cboxSum(e) { let sum = 0; for (var i = 0; i < e.length; i++) { if (e[i].type == 'checkbox') { sum += (e[i].checked) ? parseInt(e[i].value) : 0; } }; e['setFilter'].value = sum; }
    </script>
    <style>
        body { font-family: sans-serif; }
        table { border-collapse: collapse; border-style: hidden; }
        td { border: 1px solid black; vertical-align:top; padding:5px; }
        input:invalid { background-color: #eeccdd; }
    </style>
</head>
<body>
    <h2>MVP3000 - Device ID %1%</h2>
    <h3 style='color: red;'>%3%</h3>
)===";

const char htmlFoot[] PROGMEM = "<p>&nbsp;</body></html>";

const char htmlRedirect[] PROGMEM = "<!DOCTYPE html> <head> <meta http-equiv='refresh' content='4;url=/'> </head> <body> <h3 style='color: red;'>Restarting ...</h3> </body> </html>";


const char htmlSystem[] PROGMEM = R"===(
<h3>System</h3>
<ul>
    <li>ID: %1%</li>
    <li>Build: %11%</li>
    <li>Memory: %12%&percnt; fragmented</li>
    <li>Device time: %17%</li>
    <li>Uptime: %13%</li>
    <li>Last restart reason: %14%</li>
    <li>CPU frequency: %15% MHz</li>
    <li>Main loop duration: %16% ms (mean/min/max)</li>
</ul>
<h3>Modules</h3>
<ul> %20% </ul>
<h3>Maintenance</h3>
<ul>
    <li> <form action='/start' method='post' onsubmit='return confirm(`Restart?`);'> <input name='restart' type='hidden'> <input type='submit' value='Restart' > </form> </li>
    <li> <form action='/checkstart' method='post' onsubmit='return promptId(this);'> <input name='reset' type='hidden'> <input name='deviceId' type='hidden'> <input type='submit' value='Factory reset'> <input type='checkbox' name='keepwifi' checked value='1'> keep Wifi </form> </li>
</ul>
)===";


const char htmlLogger[] PROGMEM = R"===(
<h3>Web Log</h3>
<textarea rows="5" cols="120" readonly>%30%</textarea>
)===";

const char htmlLoggerDisabled[] PROGMEM = "<h3>Web Log (DISABLED)</h3>";


const char htmlNet[] PROGMEM = R"===(
<h3>Network</h3>
<ul>
    <li>Fallback AP SSID: '%41%'</li>
    <li>Network credentials: leave SSID empty to remove, any changes are applied at once.<br> <form action='/start' method='post' onsubmit='return confirm(`Change network?`);'> <input name='setwifi' type='hidden'> SSID <input name='newSsid' value='%42%'> Passphrase <input type='password' name='newPass' value='%43%'> <input type='submit' value='Save'> </form> </li>
    <li>Reconnect tries: <br> <form action='/save' method='post'> <input name='clientConnectRetries' type='number' value='%44%' min='1' max='255'> <input type='submit' value='Save'> </form> </li>
    <li>Force client mode. WARNING: If the credentials are wrong, the device will be inaccessible via network, thus require re-flashing!
    <form action='/checksave' method='post' onsubmit='return promptId(this);'> <input name='forceClientMode' type='checkbox' %45% value='1'> <input name='forceClientMode' type='hidden' value='0'> <input name='deviceId' type='hidden'> <input type='submit' value='Save'> </form> </li>
</ul>
)===";


const char htmlNetWebSockets[] PROGMEM = R"===(
<h3>WebSockets</h3>
<ul> %80% </ul>
)===";

const char htmlNetWebSocketsDisabled[] PROGMEM = "<h3>WebSockets (DISABLED)</h3>";

const char htmlNetMqtt[] PROGMEM = R"===(
<h3>MQTT Communication</h3>
<ul>
    <li>Status: %62% </li>
    <li>Local broker: %63% </li>
    <li>Forced external broker:<br> <form action='/save' method='post'> <input name='mqttForcedBroker' value='%64%'> <input type='submit' value='Save'> </form> </li>
    <li>MQTT port: default is 1883 (unsecure) <br> <form action='/save' method='post'> <input name='mqttPort' value='%65%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li>
    <li>Topics: <ul> %70% </ul> </li>
</ul>
)===";

const char htmlNetMqttDisabled[] PROGMEM = "<h3>MQTT Communication (DISABLED)</h3>";
const char htmlNetMqttNoTopics[] PROGMEM = "<h3>MQTT Communication (No Topics)</h3>";

const char htmlNetCom[] PROGMEM = R"===(
<h3>UDP Auto-Discovery</h3>
<ul>
    <li>Discovered server: %51% </li>
    <li>Discovered clients: [not implemented] </li>
    <li>Auto-discovery port: 1024-65535, default is 4211.<br> <form action='/save' method='post'> <input name='discoveryPort' value='%52%' type='number' min='1024' max='65535'> <input type='submit' value='Save'> </form> </li>
</ul>
)===";

const char htmlNetComDisabled[] PROGMEM = "<h3>UDP Auto-Discovery (DISABLED)</h3>";

#endif