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

#include "ESPX.h"

// Replicates selected methods available only in ESP8266 to ESP32 to simplify code later on
// Ensures that the ESPX object is available in the same way on both platforms
#if defined(ESP8266)
    EspClass ESPX = ESP;
#elif defined(ESP32)
    EspClassX ESPX;
#else // Ensure architecture is correct, just for the sake of it
    #error "Unsupported platform"
#endif
