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

#ifndef XMODULEEXAMPLE_WEBPAGE
#define XMODULEEXAMPLE_WEBPAGE


const char htmlXmoduleExample[] PROGMEM = R"===(%0%
<p><a href='/'>Home</a></p>
<h3>%100%</h3>
<h3>Settings</h3>
<ul>
    <li>Some fixed number: %101% </li>
    <li>Some editable number:<br> <form action='/save' method='post'> <input name='editableNumber' value='%102%' type='number' min='11112' max='65535'> <input type='submit' value='Save'> </form> </li>
</ul>
<h3>Action</h3>
<ul>
    <li>Perform some action:<br> <form action='/start' method='post'> <input name='someAction' type='hidden'> <input type='submit' value='Action'> </form> </li>
</ul>
%9%)===";

#endif