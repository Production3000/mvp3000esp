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

#ifndef MVP3000_NETWEB
#define MVP3000_NETWEB

#include <Arduino.h>

#ifdef ESP8266
    #include <ESP8266WebServer.h>
#else
    #include <WebServer.h>
    // #include <ESPAsyncWebServer.h>                                                       // TODO
#endif

#include "ESPX.h"
#ifdef ESP8266
    extern EspClass ESPX = ESP;
#else
    extern EspClassX ESPX;
#endif


class NetWeb {

    private:

        #ifdef ESP8266
            ESP8266WebServer server;
        #else
            WebServer server;
        #endif

        static const uint16_t WEB_CHUNK_LENGTH = 1024;

        // Message to serve on next page load after form save
        const char *postMessage = "";

        void serveRequest();
        void serveForm();
        void serveFormCheckId();
        // serveModuleForm() defined in modules

        void contentStart();
        void contentClose();
        void contentHome();
        // moduleHome() defined in modules

        void responsePrepareRestart();

    public:

typedef std::function<void(int)> XXfun;
void testfun(std::function<void(int)> asd) { }

        void setup();
        void loop();

        void sendFormatted(const char* formatString, ...);

        void responseRedirect(const char* message);

};

#endif