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



void XmoduleSensor::setup() {
    description = "Sensor Module";
    uri = "/sensor";

    if (cfgXmoduleSensor.dataValueCount == 0) {
        mvp.logger.write(CfgLogger::Level::ERROR, "Data value count is zero.");
        return;
    }

    // Read config
    mvp.config.readCfg(cfgXmoduleSensor);
    mvp.config.readCfg(dataCollection.processing);

    if (cfgXmoduleSensor.reportingInterval > 0)
        sensorDelay.start(cfgXmoduleSensor.reportingInterval);


    // Register sensor module web page
    mvp.net.netWeb.registerPage(uri, webPage, std::bind(&XmoduleSensor::webPageProcessor, this, std::placeholders::_1));

    // Register config to make it web-editable
    mvp.net.netWeb.registerCfg(&cfgXmoduleSensor);

    // Register webpage actions
    mvp.net.netWeb.registerAction("measureOffset", [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
        measureOffset();
        return true;
    }, "Measuring offset, this may take a few seconds ...");
    mvp.net.netWeb.registerAction("measureScaling", [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
        if ( (args == 3) &&  (argKey(1) == "valueNumber") && (argKey(2) == "targetValue") ) {
            if (measureScaling(argValue(1).toInt(), argValue(2).toInt())) {
                return true;
            }
        }
        return false;
    }, "Measuring scaling, this may take a few seconds ...");
    mvp.net.netWeb.registerAction("resetOffset", [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
        resetOffset();
        return true;
    }, "Offset reset.");
    mvp.net.netWeb.registerAction("resetScaling", [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
        resetScaling();
        return true;
    }, "Scaling reset.");

    // Data interface for latest data
    mvp.net.netWeb.registerPage(uri + "data", [&](uint8_t *buffer, size_t maxLen, size_t index)-> size_t {
        return webPageCsvResponseFiller(buffer, maxLen, index, true, [&]() -> String {
            return dataCollection.linkedListSensor.getLatestAsCsv(cfgXmoduleSensor.dataMatrixColumnCount, &dataCollection.processing);
        });
    });

    // Data interface for raw CSV data
    mvp.net.netWeb.registerPage(uri + "datasraw", [&](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        return webPageCsvResponseFiller(buffer, maxLen, index, false, [&]() -> String {
            return dataCollection.linkedListSensor.getBookmarkAsCsv(cfgXmoduleSensor.dataMatrixColumnCount, nullptr);
        });
    }, "application/octet-stream");

    // Data interface for scaled CSV data
    mvp.net.netWeb.registerPage(uri + "datasscaled", [&](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        return webPageCsvResponseFiller(buffer, maxLen, index, false, [&]() -> String {
            return dataCollection.linkedListSensor.getBookmarkAsCsv(cfgXmoduleSensor.dataMatrixColumnCount, &dataCollection.processing);
        });
    }, "application/octet-stream");


    // Register websocket
    webSocketPrint = mvp.net.netWeb.registerWebSocket("/wssensor", std::bind(&XmoduleSensor::networkCtrlCallback, this, std::placeholders::_1));

    // Register MQTT
    mqttPrint = mvp.net.netMqtt.registerMqtt("10381640_sensor", std::bind(&XmoduleSensor::networkCtrlCallback, this, std::placeholders::_1));     // TODO device ID not hardcoded topic please!!!
}

void XmoduleSensor::loop() {
    // Check flag if there is something to do
    if (!dataCollection.avgCycleFinished)
        return;
    dataCollection.avgCycleFinished = false;;

    if (offsetRunning || scalingRunning) {
        measureOffsetScalingFinish();
        return;
    }

    // Act only if remaining is 0: was never started or just finished
    if (sensorDelay.remaining() == 0) {
        if (sensorDelay.justFinished()) // || !sensorDelay.isRunning()
            sensorDelay.repeat();

        // Output data to serial, websocket, MQTT
        mvp.logger.write(CfgLogger::Level::DATA, dataCollection.linkedListSensor.getLatestAsCsvNoTime(cfgXmoduleSensor.dataMatrixColumnCount, &dataCollection.processing).c_str() );
        webSocketPrint(dataCollection.linkedListSensor.getLatestAsCsv(cfgXmoduleSensor.dataMatrixColumnCount, &dataCollection.processing));
        mqttPrint(dataCollection.linkedListSensor.getLatestAsCsv(cfgXmoduleSensor.dataMatrixColumnCount, &dataCollection.processing));
   }
}


//////////////////////////////////////////////////////////////////////////////////

void XmoduleSensor::measureOffset() {
    if (offsetRunning || scalingRunning)
        return;

    // Stop interval
    sensorDelay.stop();

    // Restart data collection with new averaging
    dataCollection.setAveragingCountPtr(&cfgXmoduleSensor.averagingOffsetScaling);

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
    sensorDelay.stop();

    // Restart data collection with new averaging
    dataCollection.setAveragingCountPtr(&cfgXmoduleSensor.averagingOffsetScaling);

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
    dataCollection.setAveragingCountPtr(&cfgXmoduleSensor.sampleAveraging);

    // Restart interval, if set
    if (cfgXmoduleSensor.reportingInterval > 0)
        sensorDelay.start(cfgXmoduleSensor.reportingInterval);
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
    // data can be 'TARE' or 'CLEAR'
    if (strcmp(data, "TARE") == 0) {
        setTare();
        mvp.logger.write(CfgLogger::Level::CONTROL, "Set Tare.");
    } else if (strcmp(data, "CLEAR") == 0) {
        clearTare();
        mvp.logger.write(CfgLogger::Level::CONTROL, "Clear Tare.");
    } else {
        mvp.logger.writeFormatted(CfgLogger::Level::CONTROL, "Unknown command '%s' received.", data);
    }
}

String XmoduleSensor::webPageProcessor(const String& var) {

    String str;
    switch (var.toInt()) {
        case 101:
            return description.c_str();
        case 102:
            return cfgXmoduleSensor.infoName.c_str();
        case 103:
            return cfgXmoduleSensor.infoDescription.c_str();

        case 111:
            return String(cfgXmoduleSensor.sampleAveraging);
        case 112:
            return String(cfgXmoduleSensor.averagingOffsetScaling);
        case 113:
            return String(cfgXmoduleSensor.reportingInterval);
        case 114:
            return String(dataCollection.linkedListSensor.getSize()) + "/" + String(dataCollection.linkedListSensor.getMaxSize()) + " (" + (dataCollection.linkedListSensor.getAdaptiveSize() ? "adaptive" : "fixed") + ")";
        case 115:
            return mvp.net.myIp.toString();

        case 121: // Sensor details: type, unit, offset, scaling, float to int exponent
            char message[128];
            for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
                snprintf(message, sizeof(message), "<tr> <td>%d</td> <td>%s</td> <td>%s</td> <td>%d</td> <td>%.2f</td> <td>%d</td> </tr>", 
                    i+1, cfgXmoduleSensor.sensorTypes[i].c_str(), cfgXmoduleSensor.sensorUnits[i].c_str(), dataCollection.processing.offset.values[i], dataCollection.processing.scaling.values[i], dataCollection.processing.sampleToIntExponent.values[i]);
                str += message;
            }
            return str;
        case 122:
            return String(cfgXmoduleSensor.dataValueCount);

        default:
            return "";
    }
}

size_t XmoduleSensor::webPageCsvResponseFiller(uint8_t* buffer, size_t maxLen, size_t index, boolean firstOnly, std::function<String()> stringFunc) {
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
        dataCollection.linkedListSensor.setBookmark(0, false, true); // Start with the oldest data
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