#include "XmoduleLED.h"

void XmoduleLED::setup() {
    // Read and register config
    mvp.config.readCfg(cfgXmoduleLED);
    mvp.net.netWeb.registerCfg(&cfgXmoduleLED, std::bind(&XmoduleLED::saveCfgCallback, this));

    ws2812fx = new WS2812FX(led_count, led_pin, NEO_GRB + NEO_KHZ800);
    ws2812fx->init();
    setLed();
    ws2812fx->start();
}

void XmoduleLED::loop() {
    ws2812fx->service();
}

void XmoduleLED::setLed() {
    ws2812fx->setBrightness(cfgXmoduleLED.brightness);
    ws2812fx->setSpeed(cfgXmoduleLED.duration);
    if (currentFxMode != cfgXmoduleLED.fxMode) {
        currentFxMode = cfgXmoduleLED.fxMode;
        ws2812fx->setMode(cfgXmoduleLED.fxMode);
    }
}

void XmoduleLED::saveCfgCallback() {
    mvp.logger.write(CfgLogger::INFO, "The config was changed via the web interface.");
    setLed();
}

String XmoduleLED::webPageProcessor(uint8_t var) {
    String str;
    switch (var) {
        case 100:
            return description;

        case 101:
            return String(cfgXmoduleLED.brightness);
        case 102:
            return String(cfgXmoduleLED.duration);

        case 110: // The fx modes
            for (uint8_t i = 0; i < MODE_COUNT; i++) {
                str += "<option value='" + String(i) + "'";
                if (i == cfgXmoduleLED.fxMode) {
                    str += " selected";
                }
                str += ">" + String(ws2812fx->getModeName(i)) + "</option>";
            }
            return str;

        default:
            return str;
    }
}
