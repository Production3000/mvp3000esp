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



// HsvColor RgbToHsv(RgbColor rgb)
// {
//     HsvColor hsv;
//     unsigned char rgbMin, rgbMax;

//     rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
//     rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);
    
//     hsv.v = rgbMax;
//     if (hsv.v == 0)
//     {
//         hsv.h = 0;
//         hsv.s = 0;
//         return hsv;
//     }

//     hsv.s = 255 * long(rgbMax - rgbMin) / hsv.v;
//     if (hsv.s == 0)
//     {
//         hsv.h = 0;
//         return hsv;
//     }

//     if (rgbMax == rgb.r)
//         hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
//     else if (rgbMax == rgb.g)
//         hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
//     else
//         hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

//     return hsv;
// }


void rgb2hsv(uint32_t currentColor, uint16_t& h, uint8_t& s, uint8_t& v) {
    uint8_t r = (uint8_t)(currentColor >> 16);
    uint8_t g = (uint8_t)(currentColor >> 8);
    uint8_t b = (uint8_t)currentColor;

    // Convert RGB to HSV
    uint8_t rgbMax = (r > g) ? r : g;
    rgbMax = (rgbMax > b) ? rgbMax : b;

    if (rgbMax == 0) { // Black
        h = 0;
        s = 0;
        v = 0;
        return;
    }
    v = rgbMax;

    uint8_t rgbMin = (r < g) ? r : g;
    rgbMin = (rgbMin < b) ? rgbMin : b;
    float_t delta = rgbMax - rgbMin;
    if (delta < 1) { // == 0 Gray
        h = 0;
        s = 0;
        return;
    }
    s = 255 * (float_t)delta / v;

    if (rgbMax == r)
        h = nearbyintf(10922.5 * float_t(g - b) / delta); // Red centered at 65535/0 rollover
    else if (rgbMax == g)
        h = nearbyintf(21845.0 + (10922.5 * float_t(b - r) / delta));
    else
        h = nearbyintf(43690.0 + (10922.5 * float_t(r - g) / delta));
};

std::map<BRIGHTNESSFX, FxContainer> brightnessFx = {
    { FADE_IN, std::make_tuple(false, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t* currentBrightness) { return timingPosition / 256; }) },
    { FADE_OUT, std::make_tuple(false, true, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t* currentBrightness) { return 255 - timingPosition / 256; }) },
    { BLINK, std::make_tuple(false, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t* currentBrightness) { return (timingPosition > 32767) ? 0 : 255; }) },
    { PULSE_FULL, std::make_tuple(false, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t* currentBrightness) { return 2 * abs((int8_t)(timingPosition / 256)); }) },
    { PULSE_HALF, std::make_tuple(false, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t* currentBrightness) { return 128 + abs((int8_t)(timingPosition / 256)); }) },
    { RND_SYNC, std::make_tuple(true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t* currentBrightness) { return (led == 0) ? random(256) : currentBrightness[0]; }) },
    { RND_SPARKLE, std::make_tuple(true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t* currentBrightness) { return random(256); }) },
    { RND_WALK, std::make_tuple(true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t* currentBrightness) { int16_t temp = currentBrightness[led] + random(-4, 5); return constrain(temp, 0, 255); }) },
    { WAVE_FWD, std::make_tuple(false, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t* currentBrightness) { return 255.0/2 * (1 + sin(2 * PI * led / ledcount - 2 * PI * timingPosition / 65535)); }) },
    { WAVE_BWD, std::make_tuple(false, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint8_t* currentBrightness) { return 255.0/2 * (1 + sin(2 * PI * led / ledcount + 2 * PI * timingPosition / 65535)); }) },
};

uint32_t color1 = Adafruit_NeoPixel::Color(255, 0, 0);
uint32_t color2 = Adafruit_NeoPixel::Color(0, 0, 255);

std::map<COLORFX, FxColorContainer> colorFx = {
    { TRANSITION, std::make_tuple(false, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t* currentColor) {                     // TODO how to get the colors in ???
        uint8_t r1 = color1 >> 16;
        uint8_t g1 = color1 >> 8;
        uint8_t b1 = color1;
        uint8_t r2 = color2 >> 16;
        uint8_t g2 = color2 >> 8;
        uint8_t b2 = color2;
        uint8_t r = r1 + (r2 - r1) * abs(float_t(timingPosition) / 65535 - 0.5) *2;
        uint8_t g = g1 + (g2 - g1) * abs(float_t(timingPosition) / 65535 - 0.5) *2;
        uint8_t b = b1 + (b2 - b1) * abs(float_t(timingPosition) / 65535 - 0.5) *2;
        return Adafruit_NeoPixel::Color(r ,g, b); }) },
    { RND_SYNC_LOUD, std::make_tuple(true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t* currentColor) { return (led == 0) ? Adafruit_NeoPixel::ColorHSV(random(65536)) : currentColor[0]; }) },
    { RND_SYNC_PASTEL, std::make_tuple(true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t* currentColor) { return (led == 0) ? Adafruit_NeoPixel::Color(random(256), random(256), random(256)) : currentColor[0]; }) },
    { RND_SPARKLE_LOUD, std::make_tuple(true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t* currentColor) { return Adafruit_NeoPixel::ColorHSV(random(65536)); }) },
    { RND_SPARKLE_PASTEL, std::make_tuple(true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t* currentColor) { return Adafruit_NeoPixel::Color(random(256), random(256), random(256)); }) },
    { RND_WALK_LOUD, std::make_tuple(true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t* currentColor) {
        uint16_t h;
        uint8_t s, v;
        rgb2hsv(currentColor[led], h, s, v);
        return Adafruit_NeoPixel::ColorHSV(h + random(-512, 513)); }) },
    { RND_WALK_PASTEL, std::make_tuple(true, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t* currentColor) {
        int16_t r = (uint8_t)(currentColor[led] >> 16) + random(-4, 5);
        int16_t g = (uint8_t)(currentColor[led] >> 8) + random(-4, 5);
        int16_t b = (uint8_t)currentColor[led] + random(-4, 5);
        return Adafruit_NeoPixel::Color(constrain(r, 0, 255), constrain(g, 0, 255), constrain(b, 0, 255)); }) },
    { RAINBOW_SYNC, std::make_tuple(false, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t* currentColor) { return Adafruit_NeoPixel::ColorHSV(timingPosition); }) },
    { RAINBOW_WAVE_FWD, std::make_tuple(false, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t* currentColor) { return Adafruit_NeoPixel::ColorHSV(65535 * led / (ledcount - 1) + timingPosition); }) },
    { RAINBOW_WAVE_BWD, std::make_tuple(false, false, [](uint8_t led, uint8_t ledcount, uint16_t timingPosition, uint32_t* currentColor) { return Adafruit_NeoPixel::ColorHSV(65535 * led / (ledcount - 1) - timingPosition); }) },
};


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

void XmoduleLED::setBrightnessEffect(uint16_t duration_ms, BRIGHTNESSFX effect) {
    FxContainer fx = brightnessFx[effect];
    setBrightnessEffect(duration_ms, std::get<0>(fx), std::get<1>(fx), std::get<2>(fx));
}

void XmoduleLED::setBrightnessEffect(uint16_t duration_ms, boolean onlyOnNewCycle, boolean runOnlyOnce, FxBrightnessSetter brightnessSetter) {
    appendXledState(XLED_STATE::FXBRIGHT);
    fxBrightnessDuration_ms = duration_ms;
    fxBrightnessOnlyOnNewCycle = onlyOnNewCycle;
    fxBrightnessRunOnlyOnce = runOnlyOnce;
    fxBrightnessSetter = brightnessSetter;
    resetTimer();
}

void XmoduleLED::setColorEffect(uint16_t duration_ms, COLORFX effect) {
    FxColorContainer fx = colorFx[effect];
    setColorEffect(duration_ms, std::get<0>(fx), std::get<1>(fx), std::get<2>(fx));
}

void XmoduleLED::setColorEffect(uint16_t duration_ms, boolean onlyOnNewCycle, boolean runOnlyOnce, FxColorSetter colorSetter) {
    appendXledState(XLED_STATE::FXCOLOR);
    fxColorDuration_ms = duration_ms;
    fxColorOnlyOnNewCycle = onlyOnNewCycle;
    fxColorRunOnlyOnce = runOnlyOnce;
    fxColorSetter = colorSetter;
    resetTimer();
}


void XmoduleLED::calculateColorEffect() {
    uint16_t timingStep = nextTimingStep(cfgXmoduleLED.refreshRateFx_Hz, fxColorDuration_ms);            /// TODO calc step depending on effect and global refresh rate

    // Fx can be repeating or single run
    // There are two duration types: 1) many gradual steps during duration cycle (fade, wheel) or 2) a single change per duration cycle (blink, random color change).
    if (fxColorOnlyOnNewCycle) {
        if (fxColorTimingPosition >= timingStep) {
            fxColorTimingPosition += timingStep;
            return;
        }
    }

    for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
        // if (fxColorSetter != nullptr)
        //     currentColors[i] = fxColorSetter(i, fxBrightnessTimingPosition, currentColors[i]);
        if (fxColorSetter != nullptr)
            currentColors[i] = fxColorSetter(i, cfgXmoduleLED.ledCount, fxColorTimingPosition, currentColors);

    }

    if (fxColorTimingPosition >= std::numeric_limits<uint16_t>::max() - timingStep) {
        if (fxColorRunOnlyOnce) {
            removeXledState(XLED_STATE::FXCOLOR);
        }
    }

    fxColorTimingPosition += timingStep;
}


void XmoduleLED::calculateBrightnessEffect() {
    uint16_t timingStep = nextTimingStep(cfgXmoduleLED.refreshRateFx_Hz, fxBrightnessDuration_ms);

    // Fx can be repeating or single run
    // There are two duration types: 1) many gradual steps during duration cycle (fade, wheel) or 2) a single change per duration cycle (blink, random color change).
    if (fxBrightnessOnlyOnNewCycle) {
        if (fxBrightnessTimingPosition >= timingStep) {
            fxBrightnessTimingPosition += timingStep;
            return;
        }
    }

    for (uint8_t i = 0; i < cfgXmoduleLED.ledCount; i++) {
        // if (fxColorSetter != nullptr)
        //     currentColors[i] = fxColorSetter(i, fxBrightnessTimingPosition, currentColors[i]);
        if (fxBrightnessSetter != nullptr)
            currentBrightness[i] = fxBrightnessSetter(i, cfgXmoduleLED.ledCount, fxBrightnessTimingPosition, currentBrightness);
    }

    if (fxBrightnessTimingPosition >= std::numeric_limits<uint16_t>::max() - timingStep) {
        if (fxBrightnessRunOnlyOnce) {
            removeXledState(XLED_STATE::FXBRIGHT);
        }
    }

    fxBrightnessTimingPosition += timingStep;
}



///////////////////////////////////////////////////////////////////////////////////

void XmoduleLED::adaptiveGlobalBrightness(uint8_t analogPin, uint8_t analogBits)
{
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
