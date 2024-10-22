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

#ifndef XMODULESENSOR_WEBPAGE
#define XMODULESENSOR_WEBPAGE

const char htmlXmoduleSensor[] PROGMEM = R"===(%0%
<p><a href='/'>Home</a></p>
<h3>%101%: %102%</h3>
<p>%103%</p>
<h3>Data Handling</h3>
<ul>
    <li>Averaging count sample measurements:<br>
        <form action='/save' method='post'> <input name='avgCountSample' value='%111%' type='number' min='1' max='255'> <input type='submit' value='Save'> </form> </li>
    <li>Averaging count offset/scaling measurements:<br>
        <form action='/save' method='post'> <input name='avgCountOffsetScaling' value='%112%' type='number' min='1' max='255'> <input type='submit' value='Save'> </form> </li>
    <li>Reporting minimum interval between data points, 0 to report all measurements:<br>
        <form action='/save' method='post'> <input name='reportingInterval' value='%113%' type='number' min='0' max='65535'> [ms] <input type='submit' value='Save'> </form> </li>
    <li>Reporting minimum change threshold, 0 to report all measurements:<br>
        <form action='/save' method='post'> <input name='thresholdPermilleChange' value='%116%' type='number' min='0' max='255'> &permil; <input type='submit' value='Save'> </form> </li>
    <li>Apply threshold only to single value, -1 to apply to all values:<br>
        <form action='/save' method='post'> <input name='thresholdOnlySingleIndex' value='%117%' type='number' min='-1' max='255'> <input type='submit' value='Save'> </form> </li>
</ul>
<h3>Data Interface</h3
 <ul>
    <li>Data storage: %114% </li>
    <li>Current data: <a href='/sensordata'>/sensordata</a> </li>
    <li>Live websocket: ws://%2%/wssensor </li>
    <li>CSV data: <a href='/sensordatasscaled'>/sensordatasscaled</a>, <a href='/sensordatasraw'>/sensordatasraw</a> </li>
</ul>
<h3>Sensor Details</h3>
<table>
    <tr>
        <td>#</td>
        <td>Type</td>
        <td>Unit</td>
        <td>Offset</td>
        <td>Scaling</td>
        <td>Float to Int exp. 10<sup>x</sup></td>
    </tr>
    %120%
    <tr>
        <td colspan='3'></td>
        <td valign='bottom'> <form action='/start' method='post' onsubmit='return confirm(`Measure offset?`);'> <input name='measureOffset' type='hidden'> <input type='submit' value='Measure offset'> </form> </td>
        <td> <form action='/start' method='post' onsubmit='return confirm(`Measure scaling?`);'> <input name='measureScaling' type='hidden'> Value number #<br> <input name='valueNumber' type='number' min='1' max='%115%'><br> Target setpoint<br> <input name='targetValue' type='number'><br> <input type='submit' value='Measure scaling'> </form> </td>
        <td></td>
    </tr>
    <tr>
        <td colspan='3'></td>
        <td> <form action='/start' method='post' onsubmit='return confirm(`Reset offset?`);'> <input name='resetOffset' type='hidden'> <input type='submit' value='Reset offset'> </form> </td>
        <td> <form action='/start' method='post' onsubmit='return confirm(`Reset scaling?`);'> <input name='resetScaling' type='hidden'> <input type='submit' value='Reset scaling'> </form> </td>
        <td></td>
    </tr>
</table>
%9%)===";

#endif