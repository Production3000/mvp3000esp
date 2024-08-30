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

    // Define web page
    webPageXmodule = new NetWeb::WebPage(uri, R"===(
<!DOCTYPE html> <html lang='en'>
<head> <title>MVP3000 - Device ID %0%</title>
    <script>function promptId(f) { f.elements['deviceId'].value = prompt('WARNING! Confirm with device ID.'); return (f.elements['deviceId'].value == '') ? false : true ; }</script>
    <style>table { border-collapse: collapse; border-style: hidden; } table td { border: 1px solid black; ; padding:5px; } input:invalid { background-color: #eeccdd; }</style> </head>
<body> <h2>MVP3000 - Device ID %0%</h2>
    <p><a href='/'>Home</a></p>
<h3>%1%</h3> <ul>
    <li>Product: %2% </li>
    <li>Description: %3% </li> </ul>
<h3>Data Handling</h3> <ul>
    <li>Sample averaging:<br> <form action='/save' method='post'> <input name='sampleAveraging' value='%12%' type='number' min='1' max='255'> <input type='submit' value='Save'> </form> </li>
    <li>Averaging of offset and scaling measurements:<br> <form action='/save' method='post'> <input name='averagingOffsetScaling' value='%13%' type='number' min='1' max='255'> <input type='submit' value='Save'> </form> </li>
    <li>Reporting minimum interval for fast sensors, 0 to ignore:<br> <form action='/save' method='post'> <input name='reportingInterval' value='%13%' type='number' min='0' max='65535'> [ms] <input type='submit' value='Save'> </form> </li> </ul>
<h3>Data Interface</h3> <ul>
    <li>Data storage: %11%</li>
    <li>Current data: <a href='/sensordata'>/sensordata</a> </li>
    <li>Live websocket: TODO </li>
    <li>CSV data: <a href='/sensordatasscaled'>/sensordatasscaled</a>, <a href='/sensordatasraw'>/sensordatasraw</a> </li> </ul>
<h3>Sensor Details</h3> <table>
    <tr> <td>#</td> <td>Type</td> <td>Unit</td> <td>Offset</td><td>Scaling</td><td>Float to Int exp. 10<sup>x</sup></td> </tr>
    %30%
    <tr> <td colspan='3'></td>
        <td valign='bottom'> <form action='/start' method='post' onsubmit='return confirm(`Measure offset?`);'> <input name='measureOffset' type='hidden'> <input type='submit' value='Measure offset'> </form> </td>
        <td> <form action='/start' method='post' onsubmit='return confirm(`Measure scaling?`);'> <input name='measureScaling' type='hidden'> Value number #<br> <input name='valueNumber' type='number' min='1' max='%21%'><br> Target setpoint<br> <input name='targetValue' type='number'><br> <input type='submit' value='Measure scaling'> </form> </td>
        <td></td> </tr>
    <tr> <td colspan='3'></td>
        <td> <form action='/start' method='post' onsubmit='return confirm(`Reset offset?`);'> <input name='resetOffset' type='hidden'> <input type='submit' value='Reset offset'> </form> </td>
        <td> <form action='/start' method='post' onsubmit='return confirm(`Reset scaling?`);'> <input name='resetScaling' type='hidden'> <input type='submit' value='Reset scaling'> </form> </td>
        <td></td> </tr> </table>
<p>&nbsp;</body></html>
        )===" ,[&](const String& var) -> String {
            // IMPORTANT: Make sure there is no additional % symbol in the
            if (!mvp.helper.isValidInteger(var)) {
                mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Non-integer placeholder in template: %s (check for any unencoded percent symbol)", var.c_str());
                return var;
            }

            String str;
            switch (var.toInt()) {
                case 0:
                    return String(ESPX.getChipId());

                case 1:
                    return description.c_str();
                case 2:
                    return cfgXmoduleSensor.infoName.c_str();
                case 3:
                    return cfgXmoduleSensor.infoDescription.c_str();

                case 11:
                    return String(dataCollection.linkedListSensor.getSize()) + "/" + String(dataCollection.linkedListSensor.getMaxSize()) + " (" + (dataCollection.linkedListSensor.getAdaptiveSize() ? "adaptive" : "fixed") + ")";
                case 12:
                    return String(cfgXmoduleSensor.sampleAveraging);
                case 13:
                    return String(cfgXmoduleSensor.averagingOffsetScaling);
                case 14:
                    return String(cfgXmoduleSensor.reportingInterval);

                case 21:
                    return String(cfgXmoduleSensor.dataValueCount);

                case 30: // Sensor details: type, unit, offset, scaling, float to int exponent
                    char message[128];
                    for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
                        snprintf(message, sizeof(message), "<tr> <td>%d</td> <td>%s</td> <td>%s</td> <td>%d</td> <td>%.2f</td> <td>%d</td> </tr>", 
                            i+1, cfgXmoduleSensor.sensorTypes[i].c_str(), cfgXmoduleSensor.sensorUnits[i].c_str(), dataCollection.processing.offset.values[i], dataCollection.processing.scaling.values[i], dataCollection.processing.sampleToIntExponent.values[i]);
                        str += message;
                    }
                    return str;

                default:
                    break;
            }
            mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Unknown placeholder in template: %s", var.c_str());
            return var;
        });
    // Register web page
    mvp.net.netWeb.registerPage(*webPageXmodule);

    // Register config
    mvp.net.netWeb.registerCfg(&cfgXmoduleSensor);

    // Register actions
    mvp.net.netWeb.registerAction("measureOffset", NetWeb::WebActionList::ResponseType::MESSAGE, [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
        measureOffset();
        return true;
    }, "Measuring offset, this may take a few seconds ...");
    mvp.net.netWeb.registerAction("measureScaling", NetWeb::WebActionList::ResponseType::MESSAGE, [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
        if ( (args == 3) &&  (argKey(1) == "valueNumber") && (argKey(2) == "targetValue") ) {
            if (measureScaling(argValue(1).toInt(), argValue(2).toInt())) {
                return true;
            }
        }
        return false;
    }, "Measuring scaling, this may take a few seconds ...");
    mvp.net.netWeb.registerAction("resetOffset", NetWeb::WebActionList::ResponseType::MESSAGE, [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
        resetOffset();
        return true;
    }, "Offset reset.");
    mvp.net.netWeb.registerAction("resetScaling", NetWeb::WebActionList::ResponseType::MESSAGE, [&](int args, std::function<String(int)> argKey, std::function<String(int)> argValue) {
        resetScaling();
        return true;
    }, "Scaling reset.");


    // Define current data interface, and register it
    webPageXmoduleDataLive = NetWeb::WebPage(uri + "data", [&](uint8_t *buffer, size_t maxLen, size_t index)-> size_t {
        // Single measurement should be sent out on first call
        // There are cases of a few byte small buffers, but hopefully not during the first call, and even if we just dont care
        if (index > 0) {
            return 0;
        }
        return webPageCsvResponseFiller(buffer, maxLen, index, [&]() -> String {
            return dataCollection.linkedListSensor.getLatestAsCsv(cfgXmoduleSensor.dataMatrixColumnCount, &dataCollection.processing);
        });

    }, "text/plain");
    mvp.net.netWeb.registerPage(webPageXmoduleDataLive);

    // Define CSV data interface, and register it
    webPageXmoduleDatasRaw = NetWeb::WebPage(uri + "datasraw", [&](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        // The index relates only to string position, and does allow to select the next measurement
        // Workaround is a bookmark in the linked list
        if (index == 0) {
            dataCollection.linkedListSensor.setBookmark(0, false, true); // Start with the oldest data
        }
        return webPageCsvResponseFiller(buffer, maxLen, index, [&]() -> String {
            return dataCollection.linkedListSensor.getBookmarkAsCsv(cfgXmoduleSensor.dataMatrixColumnCount, nullptr);
        });

    }, "application/octet-stream");
    mvp.net.netWeb.registerPage(webPageXmoduleDatasRaw);

    webPageXmoduleDatasScaled = NetWeb::WebPage(uri + "datasscaled", [&](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        // The index relates only to string position, and does allow to select the next measurement
        // Workaround is a bookmark in the linked list
        if (index == 0) {
            dataCollection.linkedListSensor.setBookmark(0, false, true); // Start with the oldest data
        }
        return webPageCsvResponseFiller(buffer, maxLen, index, [&]() -> String {
            return dataCollection.linkedListSensor.getBookmarkAsCsv(cfgXmoduleSensor.dataMatrixColumnCount, &dataCollection.processing);
        });

    }, "application/octet-stream");
    mvp.net.netWeb.registerPage(webPageXmoduleDatasScaled);


};

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

        // Output data to serial and/or network
        mvp.logger.write(CfgLogger::Level::DATA, dataCollection.linkedListSensor.getLatestAsCsvNoTime(cfgXmoduleSensor.dataMatrixColumnCount, &dataCollection.processing).c_str() );
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
        dataCollection.processing.setOffset(dataCollection.linkedListSensor.getNewestData()->values);               // TODO move to collection / processing
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
    dataCollection.setAveragingCountPtr(&cfgXmoduleSensor.sampleAveraging);

    // Restart interval, if set
    if (cfgXmoduleSensor.reportingInterval > 0)
        sensorDelay.start(cfgXmoduleSensor.reportingInterval);
}

void XmoduleSensor::resetOffset() {
    dataCollection.processing.offset.resetValues();
    mvp.config.writeCfg(dataCollection.processing);
}

void XmoduleSensor::resetScaling() {
    dataCollection.processing.scaling.resetValues();
    mvp.config.writeCfg(dataCollection.processing);
}


//////////////////////////////////////////////////////////////////////////////////

size_t XmoduleSensor::webPageCsvResponseFiller(uint8_t* buffer, size_t maxLen, size_t index, std::function<String()> stringFunc) {
    // We assume the buffer is large enough for at least a single row
    // It would be quite the effort to reliably split a row into multiple calls

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