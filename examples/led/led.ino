#include <MVP3000.h>
extern MVP3000 mvp;

// LED PIN
#define LED_PIN D6 // 'upper' J4 on the PCB
// #define LED_PIN D5 // 'lower' J5 on the PCB

// Declare our NeoPixel strip
#define LED_COUNT 4

#include "XmoduleLED.h"

XmoduleLED xmoduleLED(LED_COUNT, LED_PIN);



void setup() {
    // Add the custom module to the mvp framework
    mvp.addXmodule(&xmoduleLED);

    // Start mvp framework
    mvp.setup();
}

void loop() {
    mvp.loop();


}