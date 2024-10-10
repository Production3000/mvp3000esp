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

#include "XmoduleLED.h"

#include "_Helper.h"
extern _Helper _helper;


void XmoduleLED::setup() {
    // Read and register config
    mvp.config.readCfg(cfgXmoduleLED);
    mvp.net.netWeb.registerCfg(&cfgXmoduleLED, std::bind(&XmoduleLED::saveCfgCallback, this));

    pixels = new Adafruit_NeoPixel(cfgXmoduleLED.ledCount, cfgXmoduleLED.ledPin, NEO_GRB + NEO_KHZ800);

    // pixelGroup = new PixelGroup(cfgXmoduleLED.ledCount, 0, cfgXmoduleLED.ledCount - 1);
    // pixelGroup = new PixelGroup(cfgXmoduleLED.ledCount, 0, 5);

    pixels->begin();
    pixels->clear();
    pixels->setBrightness(cfgXmoduleLED.globalBrightness);
}

void XmoduleLED::loop() {
    if ((adcPin) && (brightnessTimer.justFinished())) {
        measureBrightness();
    }

    if (updateTimer.justFinished()) {
        if ((xledState == XLED_STATE::FXBRIGHT) || (xledState == XLED_STATE::FXFULL)) {
            calculateBrightnessEffect();
        }

        if ((xledState == XLED_STATE::FXCOLOR) || (xledState == XLED_STATE::FXFULL)) {
            calculateColorEffect();
        }
        drawLed();
    }
}


void XmoduleLED::drawLed() {
    pixels->clear();
    for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
        // Extract r, g, b from color
        uint8_t r = (uint8_t)(currentColors[i] >> 16);
        uint8_t g = (uint8_t)(currentColors[i] >> 8);
        uint8_t b = (uint8_t)currentColors[i];
        r = (float_t)r * currentBrightness[i] / 255;
        g = (float_t)g * currentBrightness[i] / 255;
        b = (float_t)b * currentBrightness[i] / 255;
        pixels->setPixelColor(i, r, g, b);
    }
    pixels->show();
}

void XmoduleLED::setBrightnessEffect(uint16_t duration_ms, XledFx::BRIGHTNESSFX effect) {
    FxContainer fx = xledFx.brightnessFx[effect];
    setBrightnessEffect(duration_ms, std::get<0>(fx), std::get<1>(fx), std::get<2>(fx), std::get<3>(fx));
}

void XmoduleLED::setBrightnessEffect(uint16_t duration_ms, boolean useSubFrames, boolean runEndless, boolean fullRange, FxBrightnessSetter brightnessSetter) {
    fxBrightnessDuration_ms = duration_ms;
    fxBrightnessRunEndless = runEndless;
    fxBrightnessUseSubFrames = useSubFrames;
    fxBrightnessFullRange = fullRange;
    fxBrightnessSetter = brightnessSetter;
    appendXledState(XLED_STATE::FXBRIGHT);
    resetTimer();
}

void XmoduleLED::setColorEffect(uint16_t duration_ms, XledFx::COLORFX effect) {
    FxColorContainer fx = xledFx.colorFx[effect];
    setColorEffect(duration_ms, std::get<0>(fx), std::get<1>(fx), std::get<2>(fx), std::get<3>(fx));
}

void XmoduleLED::setColorEffect(uint16_t duration_ms, boolean useSubFrames, boolean runEndless, boolean fullRange, FxColorSetter colorSetter) {
    fxColorDuration_ms = duration_ms;
    fxColorUseSubFrames = useSubFrames;
    fxColorRunEndless = runEndless;
    fxColorFullRange = fullRange;
    fxColorSetter = colorSetter;
    appendXledState(XLED_STATE::FXCOLOR);
    resetTimer();
}


void XmoduleLED::calculateColorEffect() {
    // The maximum position will never be achieved - this is a problem for fade effect that go for 0 to 255
    // 0 / 40 first/1st
    // 1 / 40
    // 39 / 40 last/40th
    uint16_t frameCount = cfgXmoduleLED.refreshRateFx_Hz * fxColorDuration_ms / 1000;
    uint16_t dividingFrameCount = (fxBrightnessFullRange) ? frameCount -1 : frameCount;

    // Fx can be repeating or single run
    // There are two duration types: 1) many gradual steps during duration cycle (fade, wheel) or 2) a single change per duration cycle (blink, random color change).

    if (fxColorUseSubFrames || (fxColorFrame == 0)) {
        uint16_t timingPosition = 65535 * fxColorFrame / dividingFrameCount;
        for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
            currentColors[i] = fxColorSetter(i, cfgXmoduleLED.ledCount, timingPosition, currentColors);
        }
    }

    fxColorFrame++;
    if (fxColorFrame >= frameCount) {
        // Limited resolution for short durations: 40 * 140 / 1000 = 5 --> 125 ms instead of the targeted 140 ms
        fxColorFrame = 0;
        if (!fxColorRunEndless) {
            removeXledState(XLED_STATE::FXCOLOR);
        }
    }
}

void XmoduleLED::calculateBrightnessEffect() {
    uint16_t frameCount = cfgXmoduleLED.refreshRateFx_Hz * fxBrightnessDuration_ms / 1000;
    uint16_t dividingFrameCount = (fxBrightnessFullRange) ? frameCount -1 : frameCount;

    if (fxBrightnessUseSubFrames || (fxBrightnessFrame == 0)) {
        uint16_t timingPosition = 65535 * fxBrightnessFrame / dividingFrameCount;
        for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
            currentBrightness[i] = fxBrightnessSetter(i, cfgXmoduleLED.ledCount, timingPosition, currentBrightness);
        }
    }

    fxBrightnessFrame++;
    if (fxBrightnessFrame >= frameCount) {
        fxBrightnessFrame = 0;
        if (!fxBrightnessRunEndless) {
            removeXledState(XLED_STATE::FXBRIGHT);
        }
    }
}



///////////////////////////////////////////////////////////////////////////////////

void XmoduleLED::adaptiveGlobalBrightness(uint8_t analogPin, uint8_t analogBits) {
    adcBits = analogBits;
    if (adcBits == 0)
        adcBits = _helper.adcBits;
    adcPin = analogPin;
}

void XmoduleLED::measureBrightness() {
    // Base brightness 55 + 0..200
    uint16_t nwread = 55 + 250 * analogRead(adcPin) / pow(2, adcBits);
    nwread = constrain(nwread, 0, 255);
    if (analogReading == -1) {
        analogReading = nwread;
    } else {
        analogReading = (9 * analogReading + nwread) / 10; // Moving average over 10 measurements
    }
    
    pixels->setBrightness(analogReading);
    pixels->show();
}

void XmoduleLED::setGlobalBrightness(uint8_t globalBrightness) {
    adcPin = 0;
    pixels->setBrightness(globalBrightness);
    pixels->show();
}


///////////////////////////////////////////////////////////////////////////////////

void XmoduleLED::saveCfgCallback() {
    // mvp.logger.write(CfgLogger::INFO, "The config was changed via the web interface.");
    // setLed();
}

String XmoduleLED::webPageProcessor(uint8_t var) {
    switch (var) {
        case 100:
            return description;

        // TODO LED count
        case 101:
            return String(cfgXmoduleLED.globalBrightness);
        case 102:
            return "asdasd";

        case 110: // The fx modes
            // for (uint8_t i = 0; i < MODE_COUNT; i++) {
            //     str += "<option value='" + String(i) + "'";
            //     if (i == cfgXmoduleLED.fxMode) {
            //         str += " selected";
            //     }
            //     str += ">" + String(ws2812fx->getModeName(i)) + "</option>";
            // }
            return "";

        default:
            return "";
    }
}
