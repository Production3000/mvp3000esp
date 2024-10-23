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

#ifndef XMODULELED_WEBPAGE
#define XMODULELED_WEBPAGE

const char htmlXmoduleLed[] PROGMEM = R"===(%0%
<p><a href='%4%'>Home</a></p>
<h3>%100%</h3>
<h3>Settings</h3>
<ul>
    <li>Global brightness, :<br>
        <form action='/save' method='post'> <input name='globalBrightness' value='%101%' type='number' min='0' max='255'> <input type='submit' value='Save'> </form> </li>
</ul>
<h3>Effects</h3>
<ul>
    <li>Set brightness effect:<br>
        <form action='/start' method='post'> <select name='brightnessEffect'> <option value='-1'>-</option> %110% </select> <input name='duration' value='%112%' type='number' min='0' max='65535'> <input type='submit' value='Set'> </form> </li>
    <li>Set color effect:<br>
        <form action='/start' method='post'> <select name='colorEffect'> <option value='-1'>-</option> %120% </select> <input name='duration' value='%122%' type='number' min='0' max='65535'> <input type='submit' value='Set'> </form> </li>
</ul>
%9%)===";

#endif