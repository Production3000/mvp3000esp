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

#include "XmoduleSensor.h"

#include "MVP3000.h"
extern MVP3000 mvp;

#include "_Helper.h"
extern _Helper _helper;


void XmoduleSensor::setup() {
    if (cfgXmoduleSensor.dataValueCount == 0) {
        mvp.logger.write(CfgLogger::Level::ERROR, "Data value count is zero.");
        return;
    }

    // Read config and register config
    mvp.config.readCfg(cfgXmoduleSensor);
    mvp.config.readCfg(dataCollection.processing);
    mvp.net.netWeb.registerCfg(&cfgXmoduleSensor);

    if (cfgXmoduleSensor.reportingInterval > 0)
        reportingTimer.restart(cfgXmoduleSensor.reportingInterval);

    // Register webpage actions
    mvp.net.netWeb.registerAction("measureOffset", [&](int args, WebArgKeyValue argKey, WebArgKeyValue argValue) {
        measureOffset();
        return true;
    }, "Measuring offset, this may take a few seconds ...");

    mvp.net.netWeb.registerAction("measureScaling", [&](int args, WebArgKeyValue argKey, WebArgKeyValue argValue) {
        if ( (args == 3) &&  (argKey(1) == "valueNumber") && (argKey(2) == "targetValue") ) {
            if (measureScaling(argValue(1).toInt(), argValue(2).toInt())) {
                return true;
            }
        }
        return false;
    }, "Measuring scaling, this may take a few seconds ...");

    mvp.net.netWeb.registerAction("resetOffset", [&](int args, WebArgKeyValue argKey, WebArgKeyValue argValue) {
        resetOffset();
        return true;
    }, "Offset reset.");

    mvp.net.netWeb.registerAction("resetScaling", [&](int args, WebArgKeyValue argKey, WebArgKeyValue argValue) {
        resetScaling();
        return true;
    }, "Scaling reset.");

    // Register CSV: latest, raw, scaled
    mvp.net.netWeb.registerFillerPage(uri + "data", [&](AsyncWebServerRequest *request) {
        request->sendChunked("text/html", std::bind(&XmoduleSensor::csvLatestResponseFiller, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    });

    mvp.net.netWeb.registerFillerPage(uri + "datasraw", [&](AsyncWebServerRequest *request) {
        request->sendChunked("application/octet-stream", std::bind(&XmoduleSensor::csvRawResponseFiller, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    });

    mvp.net.netWeb.registerFillerPage(uri + "datasscaled", [&](AsyncWebServerRequest *request) {
        request->sendChunked("application/octet-stream", std::bind(&XmoduleSensor::csvScaledResponseFiller, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    });

    // Register websocket and MQTT
    mvp.net.netWeb.webSockets.registerWebSocket(uriWebSocket, std::bind(&XmoduleSensor::networkCtrlCallback, this, std::placeholders::_1));
    mvp.net.netMqtt.registerMqtt(mqttTopic, std::bind(&XmoduleSensor::networkCtrlCallback, this, std::placeholders::_1));
}

void XmoduleSensor::loop() {
    // IMPORTANT: seprated user control callback, the ESP crashes if a delay() is used there 
    if (callCtrlCallbackNow) {
        networkCtrlUserCallback(networkCtrlCallbackData);
        callCtrlCallbackNow = false;
    }

    // Check flag if there is something to do
    if (!dataCollection.avgCycleFinished)
        return;
    dataCollection.avgCycleFinished = false;;

    // Check if offset or scaling measurement is running
    if (offsetRunning || scalingRunning) {
        measureOffsetScalingFinish();
        return;
    }

    // Check if recording threshold was reached, otherwise just remove the measurement and do nothing
    // Threshold is not checked when appending but here: no need to check for offset/scaling measurements and averaging is already done/noise is lower
    if (cfgXmoduleSensor.thresholdPermilleChange > 0) {
        if (!dataCollection.linkedListSensor.isAboveThreshold(cfgXmoduleSensor.thresholdPermilleChange, cfgXmoduleSensor.thresholdOnlySingleIndex, &dataCollection.processing)) {
            return;
        }
    }

    // Act only if timer a) was never started or b) just finished, otherwise remove measurment
    // This is not done when appending but here to not delay offset/scaling measurements, actual time is not known during averaging
    if (!reportingTimer.justFinished()) {
        dataCollection.linkedListSensor.removeLast();
        return;
    }

    // Output data to serial, websocket, MQTT
    if (cfgXmoduleSensor.outputTargets.isSet(CfgXmoduleSensor::OutputTarget::CONSOLE)) {
        mvp.logger.write(CfgLogger::Level::DATA, dataCollection.linkedListSensor.getLatestAsCsvNoTime(cfgXmoduleSensor.matrixColumnCount, &dataCollection.processing).c_str() );
    }
    if (cfgXmoduleSensor.outputTargets.isSet(CfgXmoduleSensor::OutputTarget::WEBSOCKET)) {
        mvp.net.netWeb.webSockets.printWebSocket(uriWebSocket, dataCollection.linkedListSensor.getLatestAsCsv(cfgXmoduleSensor.matrixColumnCount, &dataCollection.processing));
    }
    if (cfgXmoduleSensor.outputTargets.isSet(CfgXmoduleSensor::OutputTarget::MQTT)) {
        mvp.net.netMqtt.printMqtt(mqttTopic, dataCollection.linkedListSensor.getLatestAsCsv(cfgXmoduleSensor.matrixColumnCount, &dataCollection.processing));
    }
}


//////////////////////////////////////////////////////////////////////////////////

void XmoduleSensor::measureOffset() {
    if (offsetRunning || scalingRunning)
        return;

    // Stop interval
    reportingTimer.stop();

    // Restart data collection with new averaging
    dataCollection.setAveragingCountPtr(&cfgXmoduleSensor.avgCountOffsetScaling);

    offsetRunning = true;
    mvp.logger.write(CfgLogger::Level::INFO, "Offset measurement started.");
}

bool XmoduleSensor::measureScaling(uint8_t valueNumber, int32_t targetValue) {
    if (offsetRunning || scalingRunning)
        return true; // This would likely be a double press, true = 'measurement started' seems to be the better response

    // Numbering starts from 1 in the real world!
    if ((valueNumber == 0) || (valueNumber > cfgXmoduleSensor.dataValueCount)) {
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Scaling measurement valueNumber out of bounds.");
        return false;
    }
    // Set target, convert real-world number to index
    dataCollection.processing.setScalingTarget(valueNumber - 1, targetValue);

    // Stop interval
    reportingTimer.stop();

    // Restart data collection with new averaging
    dataCollection.setAveragingCountPtr(&cfgXmoduleSensor.avgCountOffsetScaling);

    scalingRunning = true;
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Scaling measurement of index %d started.", scalingValueIndex);
    return true;
}

void XmoduleSensor::measureOffsetScalingFinish() {
    // Calculate offset or scaling
    if (offsetRunning) {
        dataCollection.processing.setOffset(dataCollection.linkedListSensor.getNewestData()->values);
    } else if (scalingRunning) {
        dataCollection.processing.setScaling(dataCollection.linkedListSensor.getNewestData()->values);
    } else {
        mvp.logger.write(CfgLogger::Level::ERROR, "Offset/Scaling measurement finished without running.");
        return;
    }
    offsetRunning = false;
    scalingRunning = false;

    // Save
    mvp.config.writeCfg(dataCollection.processing);
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Offset/Scaling measurement done in %d ms.", millis() - dataCollection.avgStartTime );

    // Restart data collection with new averaging
    clearTare();
    dataCollection.setAveragingCountPtr(&cfgXmoduleSensor.avgCountSample);

    // Restart interval
    reportingTimer.restart();
}

void XmoduleSensor::resetOffset() {
    dataCollection.processing.offset.resetValues();
    mvp.config.writeCfg(dataCollection.processing);
    clearTare();
}

void XmoduleSensor::resetScaling() {
    dataCollection.processing.scaling.resetValues();
    mvp.config.writeCfg(dataCollection.processing);
    clearTare();
}

void XmoduleSensor::clearTare() {
    dataCollection.processing.tare.resetValues();
}

void XmoduleSensor::setTare() {
    dataCollection.processing.setTare(dataCollection.linkedListSensor.getNewestData()->values);
}


//////////////////////////////////////////////////////////////////////////////////

void XmoduleSensor::networkCtrlCallback(char* data) {
    // IMPORTANT: any delay() here or in any called function will crash the ESP8266 (ESP32 untested)
    if (strcmp(data, "TARE") == 0) {
        setTare();
        mvp.logger.write(CfgLogger::Level::CONTROL, "Set Tare.");
    } else if (strcmp(data, "CLEAR") == 0) {
        clearTare();
        mvp.logger.write(CfgLogger::Level::CONTROL, "Clear Tare.");
    } else {
        // Check if there is a custom callback defined
        if (networkCtrlUserCallback == nullptr) {
            mvp.logger.writeFormatted(CfgLogger::Level::CONTROL, "Unknown control command '%s' received.", data);
        } else {
            // IMPORTANT: Separate the callback from user/library code because of the delay() issue
            // The callback is called from the loop()
            callCtrlCallbackNow = true;
            networkCtrlCallbackData = data;
            mvp.logger.writeFormatted(CfgLogger::Level::CONTROL, "Unknown control command '%s' received. Trying user callback function.", data);
        }
    }
}


String XmoduleSensor::webPageProcessor(uint8_t var) {
    switch (var) {
        case 101:
            return description;
        case 102:
            return cfgXmoduleSensor.infoName;
        case 103:
            return cfgXmoduleSensor.infoDescription;

        case 111:
            return String(cfgXmoduleSensor.avgCountSample);
        case 112:
            return String(cfgXmoduleSensor.avgCountOffsetScaling);
        case 113:
            return String(cfgXmoduleSensor.reportingInterval);
        case 114:
            return _helper.printFormatted("%d / %d (%s)", dataCollection.linkedListSensor.getSize(), dataCollection.linkedListSensor.getMaxSize(), (dataCollection.linkedListSensor.isAdaptive() ? "adaptive" : "fixed"));
        case 115:
            return String(cfgXmoduleSensor.dataValueCount);
        case 116:
            return String(cfgXmoduleSensor.thresholdPermilleChange);
        case 117:
            return String(cfgXmoduleSensor.thresholdOnlySingleIndex);

        case 120: // Split the long string into multiple rows
            webPageProcessorCount = 0;
        case 121:
            webPageProcessorCount++;
            // Sensor details: type, unit, offset, scaling, float to int exponent - placeholder for next row
            return _helper.printFormatted("<tr> <td>%d</td> <td>%s</td> <td>%s</td> <td>%d</td> <td>%.2f</td> <td>%d</td> </tr>%s",
                webPageProcessorCount,
                cfgXmoduleSensor.sensorTypes[webPageProcessorCount - 1],
                cfgXmoduleSensor.sensorUnits[webPageProcessorCount - 1],
                dataCollection.processing.offset.values[webPageProcessorCount - 1],
                dataCollection.processing.scaling.values[webPageProcessorCount - 1],
                dataCollection.processing.sampleToIntExponent.values[webPageProcessorCount - 1],
                (webPageProcessorCount < cfgXmoduleSensor.dataValueCount) ? "%121%" : "");

        default:
            return "";
    }
}

size_t XmoduleSensor::csvLatestResponseFiller(uint8_t *buffer, size_t maxLen, size_t index) {
    return csvExtendedResponseFiller(buffer, maxLen, index, true, [&]() -> String {
        return dataCollection.linkedListSensor.getLatestAsCsv(cfgXmoduleSensor.matrixColumnCount, &dataCollection.processing);
    });
}

size_t XmoduleSensor::csvRawResponseFiller(uint8_t *buffer, size_t maxLen, size_t index) {
    return csvExtendedResponseFiller(buffer, maxLen, index, false, [&]() -> String {
        return dataCollection.linkedListSensor.getBookmarkAsCsv(cfgXmoduleSensor.matrixColumnCount, nullptr);
    });
}

size_t XmoduleSensor::csvScaledResponseFiller(uint8_t *buffer, size_t maxLen, size_t index) {
    return csvExtendedResponseFiller(buffer, maxLen, index, false, [&]() -> String {
        return dataCollection.linkedListSensor.getBookmarkAsCsv(cfgXmoduleSensor.matrixColumnCount, &dataCollection.processing);
    });
}

size_t XmoduleSensor::csvExtendedResponseFiller(uint8_t* buffer, size_t maxLen, size_t index, boolean firstOnly, std::function<String()> stringFunc) {
    // We assume the buffer is large enough for at least the first single row
    // It would be quite the effort to reliably split a row into multiple calls

    if (firstOnly && index > 0) {
        // Single measurement should be sent out on first call
        // There are cases of a few byte small buffers, but hopefully not during the first call, and even if we just dont care
        return 0;
    }
    if (!firstOnly && (index == 0)) {
        // The index relates only to string position, and does allow to select the next measurement
        // Workaround is a bookmark in the linked list
        dataCollection.linkedListSensor.bookmarkByIndex(0, false, true); // Start with the oldest data
    }

    size_t pos = 0;
    while (true) {
        // Prepare CSV string
        String str = stringFunc();
        if (str.length() == 0) {
            break; // Empty string, should not happen
        }
        str += "\n";
        uint16_t strLen = str.length();

        // Make sure there is enough space in the buffer
        if (pos + strLen >= maxLen) {
            if (pos == 0) {
                // Well, this should not happen, buffer full even before the first iteration of the loop
                // But it does! The buffer is often just a few (<10) bytes long
                // This is actually so common, it is not even worth an info message
                // mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Web-CSV buffer too small for data: %d < %d.", maxLen, strLen);
                // WORKAROUND: Return a single space to indicate there is more data. The next buffer will likely be larger.
                if (maxLen > 0) {
                    memcpy(buffer, " ", 1);
                    pos++;
                } // we do not catch maxLen = 0, but that should really really not happen!
            }
            break;
        }

        // Copy string to the position in the buffer and increment position/total length
        memcpy(buffer + pos, str.c_str(), strLen);
        pos += strLen;

        // Exit if this was the last measurement
        if (!dataCollection.linkedListSensor.moveBookmark()) {
            break;
        }
        // Exit if the next measurement would (probably, with some margin) not fit
        if (pos + 1.2 * strLen > maxLen) {
            break;
        }
    }

    return pos;
}