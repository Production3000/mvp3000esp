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

#include "MVP3000.h"
extern MVP3000 mvp;

#include "_Helper.h"
extern _Helper _helper;


void XmoduleLED::setup() {
    // Read and register config
    mvp.config.readCfg(cfgXmoduleLED);
    mvp.net.netWeb.registerCfg(&cfgXmoduleLED, std::bind(&XmoduleLED::saveCfgCallback, this));

    // Register action
    mvp.net.netWeb.registerAction("brightnessEffect", [&](int args, WebArgKeyValue argKey, WebArgKeyValue argValue) {
        // argKey(0) is the action name
        if (argValue(0).toInt() == -1) {
            removeXledState(XLED_STATE::FXBRIGHT);
        } else {
            setBrightnessEffect(argValue(1).toInt(), static_cast<XledFx::BRIGHTNESSFX>(argValue(0).toInt()));
        }
        return true;
    }, "Brightness FX set.");

    mvp.net.netWeb.registerAction("colorEffect", [&](int args, WebArgKeyValue argKey, WebArgKeyValue argValue) {
        // argKey(0) is the action name
        if (argValue(0).toInt() == -1) {
            removeXledState(XLED_STATE::FXCOLOR);
        } else {
            setColorEffect(argValue(1).toInt(), static_cast<XledFx::COLORFX>(argValue(0).toInt()));
        }
        return true;
    }, "Color FX set.");

    pixels = new Adafruit_NeoPixel(cfgXmoduleLED.ledCount, cfgXmoduleLED.ledPin, NEO_GRB + NEO_KHZ800);

    pixels->begin();
    pixels->clear();
    pixels->setBrightness(cfgXmoduleLED.globalBrightness);
}

void XmoduleLED::loop() {
    if ((adcPin) && (brightnessTimer.justFinished())) {
        measureBrightness();
    }

    if (frameTimer.justFinished()) {
        if ((xledState == XLED_STATE::FXBRIGHT) || (xledState == XLED_STATE::FXFULL)) {
            if (!brightnessFxCalculator.calculate(cfgXmoduleLED.refreshRateFx_Hz, &currentBrightness, cfgXmoduleLED.ledCount)) {
                removeXledState(XLED_STATE::FXBRIGHT);
            }
        }

        if ((xledState == XLED_STATE::FXCOLOR) || (xledState == XLED_STATE::FXFULL)) {
            if (!colorFxCalculator.calculate(cfgXmoduleLED.refreshRateFx_Hz, &currentColors, cfgXmoduleLED.ledCount)) {
                removeXledState(XLED_STATE::FXCOLOR);
            }
        }
        drawLed();
    }
}


///////////////////////////////////////////////////////////////////////////////////

void XmoduleLED::setBrightnessEffect(uint16_t duration_ms, XledFx::BRIGHTNESSFX effect) {
    FxBrightnessContainer fx = xledFx.brightnessFx[effect];
    setBrightnessEffect(duration_ms, std::get<0>(fx), std::get<1>(fx), std::get<2>(fx));
}

void XmoduleLED::setBrightnessEffect(uint16_t duration_ms, boolean useFrames, boolean runEndless, FxBrightnessSetter brightnessSetter) {
    brightnessFxCalculator = XledFx::FxCalculator(duration_ms, runEndless, useFrames, brightnessSetter);
    appendXledState(XLED_STATE::FXBRIGHT);
    resetTimer();
}

void XmoduleLED::setFixedBrightnessIndividual(uint8_t *brightness) {
    removeXledState(XLED_STATE::FXBRIGHT);
    for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; ++i) {
        currentBrightness[i] = brightness[i];
    }
    resetTimer();
}

void XmoduleLED::setFixedBrightnessSync(uint8_t brightness){
    removeXledState(XLED_STATE::FXBRIGHT);
    std::fill_n(currentBrightness, cfgXmoduleLED.ledCount, brightness);
    resetTimer();
}


///////////////////////////////////////////////////////////////////////////////////

void XmoduleLED::setColorEffect(uint16_t duration_ms, XledFx::COLORFX effect) {
    FxColorContainer fx = xledFx.colorFx[effect];
    setColorEffect(duration_ms, std::get<0>(fx), std::get<1>(fx), std::get<2>(fx), std::get<3>(fx));
}

void XmoduleLED::setColorEffect(uint16_t duration_ms, boolean useFrames, boolean runEndless, boolean colorWheel, FxColorSetter colorSetter) {
    colorFxCalculator = XledFx::FxCalculator(duration_ms, colorWheel, runEndless, useFrames, colorSetter);
    appendXledState(XLED_STATE::FXCOLOR);
    resetTimer();
}

void XmoduleLED::setFixedColorIndividual(uint32_t *colors) {
    removeXledState(XLED_STATE::FXCOLOR);
    for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
        currentColors[i] = colors[i];
    }
    resetTimer();
}

void XmoduleLED::setFixedColorRandom(){ 
    removeXledState(XLED_STATE::FXCOLOR);
    for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
        currentColors[i] = Adafruit_NeoPixel::ColorHSV(random(65536), 255, 255);
    }
    resetTimer();
}

void XmoduleLED::setFixedColorSync(uint32_t color){
    removeXledState(XLED_STATE::FXCOLOR);
    std::fill_n(currentColors, cfgXmoduleLED.ledCount, color);
    resetTimer();
}


///////////////////////////////////////////////////////////////////////////////////

void XmoduleLED::drawLed() {
    pixels->clear();
    for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
        // Combine current color and brightness
        uint32_t cC = currentColors[i];
        uint8_t *c = (uint8_t *)&cC;
        for (uint8_t j = 0; j < 4; j++)
            c[j] = (float_t)c[j] * currentBrightness[i] / 255;
        pixels->setPixelColor(i, cC);
    }
    pixels->show();
}

void XmoduleLED::appendXledState(XLED_STATE state) {
    if ((xledState == state) || (xledState == XLED_STATE::FXFULL)) // Nothing to do
        return;
    xledState = static_cast<XLED_STATE>(xledState + state);
}

void XmoduleLED::removeXledState(XLED_STATE state) {
    if (xledState == XLED_STATE::NOFX) // Nothing to do
        return;
    if ((xledState == state) || (xledState == XLED_STATE::FXFULL)){
        xledState = static_cast<XLED_STATE>(xledState - state);
    }
}

void XmoduleLED::resetTimer() {
    if (xledState == XLED_STATE::NOFX) {
        frameTimer.restart(cfgXmoduleLED.refreshRateStatic_s * 1000);
    } else {
        frameTimer.restart(1000/cfgXmoduleLED.refreshRateFx_Hz);
    }
}


///////////////////////////////////////////////////////////////////////////////////

void XmoduleLED::adaptiveGlobalBrightness(uint8_t analogPin, uint8_t analogBits)
{
    adcBits = analogBits;
    if (adcBits == 0)
        adcBits = _helper.adcBits;
    adcPin = analogPin;
}

void XmoduleLED::measureBrightness() {
    // Base brightness 55 + 0..200
    uint16_t newValue = 55 + 250 * analogRead(adcPin) / pow(2, adcBits);
    newValue = constrain(newValue, 0, 255);
    if (analogReading == -1) {
        analogReading = newValue;
    } else {
        analogReading = (9 * analogReading + newValue) / 10; // Moving average over 10 measurements
    }
    
    pixels->setBrightness(analogReading);
    pixels->show();
}

void XmoduleLED::fixedGlobalBrightness(uint8_t globalBrightness) {
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
    String str;
    switch (var) {
        case 100:
            return description;

        // TODO LED count
        case 101:
            return String(cfgXmoduleLED.globalBrightness);

        case 110: // The fx modes
            for (auto const& fx : xledFx.brightnessFxNames) {
                str += "<option value='" + String(fx.first) + "'>";
                str += String(fx.second) + "</option>";
            }
            return str;
        case 112:
            return String(brightnessFxCalculator.duration_ms);

        case 120: // The fx modes
            for (auto const& fx : xledFx.colorFxNames) {
                str += "<option value='" + String(fx.first) + "'>";
                str += String(fx.second) + "</option>";
            }
            return str;
        case 122:
            return String(colorFxCalculator.duration_ms);

        default:
            return "";

    }
}
