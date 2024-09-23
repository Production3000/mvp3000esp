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

#include "_Xmodule.h"

#include "MVP3000.h"
extern MVP3000 mvp;


void _Xmodule::setupFramework(){
    if (uri.length() > 2)
        mvp.net.netWeb.registerModulePage(uri);
}

void _Xmodule::mqttPrint(const String& topic, const String& payload){
    mvp.net.netMqtt.print(topic, payload);
}
