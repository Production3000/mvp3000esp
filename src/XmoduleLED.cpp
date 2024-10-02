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
    pixels->setBrightness(cfgXmoduleLED.brightness);
}

void XmoduleLED::loop() {
    if ((adcPin) && (brightnessTimer.justFinished())) {
        measureBrightness();
    }

    if (xledState == XLED_STATE::ONDEMAND)
        return;

    if (fxTimer.justFinished()) {
        executeEffect();
    }
}


///////////////////////////////////////////////////////////////////////////////////

void XmoduleLED::adaptiveBrightness(uint8_t analogPin, uint8_t analogBits) {
    adcBits = analogBits;
    if (adcBits == 0)
        _helper.adcBits;
    adcPin = analogPin;
}

void XmoduleLED::measureBrightness() {
    if (analogReading == -1) {
        analogReading = analogRead(adcPin);
    } else {
        analogReading = (2 * analogReading + analogRead(adcPin)) / 3; // Moving average over 3 measurements
    }
    pixels->setBrightness(analogReading*255/(1 << adcBits));
    pixels->show();
}

void XmoduleLED::setBrightness(uint8_t brightness) {
    adcPin = 0;
    pixels->setBrightness(brightness);
    pixels->show();
}


///////////////////////////////////////////////////////////////////////////////////

void XmoduleLED::setLed(CallbackSyncSetter setOnceSyncSetter) {
    uint32_t color = setOnceSyncSetter();
    setLed([color](uint8_t i) { return color; });
}

void XmoduleLED::setLed(CallbackSingleSetter setOnceSingleSetter) {
    pixels->clear();
    for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
        pixels->setPixelColor(i, setOnceSingleSetter(i));
    }
    pixels->show();
}

void XmoduleLED::setLed(CallbackArraySetter setOnceArraySetter) {
    uint32_t* ledArray = new uint32_t[cfgXmoduleLED.ledCount]; // IMPORTANT: Free the memory
    memset(ledArray, 0, cfgXmoduleLED.ledCount * sizeof(uint32_t)); // set zero
    setOnceArraySetter(ledArray, cfgXmoduleLED.ledCount);

    setLed([ledArray](uint8_t i) { return ledArray[i]; });
    delete[] ledArray; // IMPORTANT: Free the memory
}


///////////////////////////////////////////////////////////////////////////////////

void XmoduleLED::setOnDemandCallback(CallbackSyncSetter syncSetter, CallbackSingleSetter singleSetter, CallbackArraySetter arraySetter) {
    xledState = XLED_STATE::ONDEMAND;
    onDemandSyncSetter = syncSetter;
    onDemandSingleSetter = singleSetter;
    onDemandArraySetter = arraySetter;
}

void XmoduleLED::demandLedUpdate() {
    if (onDemandSyncSetter != nullptr) {
        setLed(onDemandSyncSetter);
    } else if (onDemandSingleSetter != nullptr) {
        setLed(onDemandSingleSetter);
    } else {
        setLed(onDemandArraySetter);
    }
}


///////////////////////////////////////////////////////////////////////////////////

void XmoduleLED::setEffect(uint8_t effect) {
    xledState = XLED_STATE::EFFECT;
    fxContainer = xledFx.getFxContainer(effect);
}

void XmoduleLED::setEffect(FxSingleSetter singleSetter, FxSyncSetter syncSetter, uint16_t duration_ms, boolean onlyOnNewCycle) {
    xledState = XLED_STATE::EFFECT;
    fxContainer = FxContainer(singleSetter, syncSetter, duration_ms, onlyOnNewCycle);
}

void XmoduleLED::executeEffect() {
    // The timer is expected to be slower than anticipated, thus round up the timing calculation.
    // step = MAX / (rate * duration) + 1
    uint16_t nextTimingStep = (std::numeric_limits<uint16_t>::max() * 1000) / (cfgXmoduleLED.fxRefreshRate_Hz * fxContainer.duration_ms) + 1;

    // There are two types of effect: gradual change per cycle like a color wheel, or single change per cycle like blinking.
    if (fxContainer.onlyOnNewCycle) {
        if (fxContainer.timingPosition > nextTimingStep) {
            fxContainer.timingPosition += nextTimingStep;
            return;
        }
    }

    pixels->clear();
    if (fxContainer.syncSetter != nullptr) {
        uint32_t color = fxContainer.syncSetter(fxContainer.timingPosition);
        for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
            pixels->setPixelColor(i, color);
        }
    } else {
        for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
            pixels->setPixelColor(i, fxContainer.singleSetter(i, fxContainer.timingPosition));
        }
    }
    pixels->show();

    fxContainer.timingPosition += nextTimingStep;
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
            return String(cfgXmoduleLED.brightness);
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
