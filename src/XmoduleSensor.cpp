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

    if (cfgXmoduleSensor.dataValueCount == 0) {
        mvp.logger.write(CfgLogger::Level::ERROR, "Data value count is zero.");
        return;
    }

    // Read config
    mvp.config.readCfg(cfgXmoduleSensor);
    mvp.config.readCfg(dataProcessing);

    if (cfgXmoduleSensor.reportingInterval > 0)
        sensorDelay.start(cfgXmoduleSensor.reportingInterval);
};

void XmoduleSensor::loop() {
    // Call only when there is something to do
    if (!newDataStored)
        return;
    newDataStored = false;

    // Act only if remaining is 0: was never started or just finished
    if (sensorDelay.remaining() == 0) {
        if (sensorDelay.justFinished()) // || !sensorDelay.isRunning()
            sensorDelay.repeat();

        // Output data to serial and/or network
        mvp.logger.writeCSV(CfgLogger::Level::DATA, currentMeasurementScaled().values, cfgXmoduleSensor.dataValueCount, cfgXmoduleSensor.dataMatrixColumnCount);
   }
}


//////////////////////////////////////////////////////////////////////////////////

void XmoduleSensor::contentModuleNetWeb() {
    mvp.net.netWeb.sendFormatted("<h1>Sensor Module</h1>");

    // Sensor info
    mvp.net.netWeb.sendFormatted("\
        <h3>Sensor</h3> <ul> \
        <li>Product: %s </li>  \
        <li>Description: %s </li> </ul>",
        cfgXmoduleSensor.infoName.c_str(), cfgXmoduleSensor.infoDescription.c_str());

    // Settings
    mvp.net.netWeb.sendFormatted("\
        <h3>Data Handling</h3> <ul> \
        <li>Sample averaging:<br> <form action='/save' method='post'> <input name='sampleAveraging' value='%d' type='number' min='1' max='255'> <input type='submit' value='Save'> </form> </li> \
        <li>Averaging of offset and scaling measurements:<br> <form action='/save' method='post'> <input name='averagingOffsetScaling' value='%d' type='number' min='1' max='255'> <input type='submit' value='Save'> </form> </li> \
        <li>Reporting interval, minimum time for fast sensors, zero is ignore:<br> <form action='/save' method='post'> <input name='reportingInterval' value='%d' type='number' min='0' max='65535'> [ms] <input type='submit' value='Save'> </form> </li> </ul>",
        cfgXmoduleSensor.sampleAveraging, cfgXmoduleSensor.averagingOffsetScaling, cfgXmoduleSensor.reportingInterval);

    // Table for offset, scaling, float2int
    mvp.net.netWeb.sendFormatted("<h3>Sensor Details</h3> <table> <tr> <td>#</td> <td>Type</td> <td>Unit</td> <td>Offset</td><td>Scaling</td><td>Float to Int exp. 10<sup>x</sup></td> </tr>");
    for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
        // Type, units, default and current offset/scaling
        mvp.net.netWeb.sendFormatted("<tr> <td>%d</td> <td>%s</td> <td>%s</td> <td>%d</td> <td>%.2e</td> <td>%d</td> </tr>",
            i+1, cfgXmoduleSensor.sensorTypes[i].c_str(), cfgXmoduleSensor.sensorUnits[i].c_str(), dataProcessing.offset.values[i], dataProcessing.scaling.values[i], dataProcessing.sampleToIntExponent.values[i]);
    }
    mvp.net.netWeb.sendFormatted("\
        <tr> <td colspan='3'></td> \
        <td valign='bottom'> <form action='/start' method='post' onsubmit='return confirm(`Measure offset?`);'> <input name='measureOffset' type='hidden'> <input type='submit' value='Measure offset'> </form> </td> \
        <td> <form action='/start' method='post' onsubmit='return confirm(`Measure scaling?`);'> <input name='measureScaling' type='hidden'> Value number #<br> <input name='valueNumber' type='number' min='1' max='%d'><br> Target setpoint<br> <input name='targetValue' type='number'><br> <input type='submit' value='Measure scaling'> </form> </td> \
        <td></td> </tr>",
        cfgXmoduleSensor.dataValueCount);
    mvp.net.netWeb.sendFormatted(" \
        <tr> <td colspan='3'></td> \
        <td> <form action='/start' method='post' onsubmit='return confirm(`Reset offset?`);'> <input name='resetOffset' type='hidden'> <input type='submit' value='Reset offset'> </form> </td> \
        <td> <form action='/start' method='post' onsubmit='return confirm(`Reset scaling?`);'> <input name='resetScaling' type='hidden'> <input type='submit' value='Reset scaling'> </form> </td> \
        <td></td> </tr> </table>",
        cfgXmoduleSensor.dataValueCount);
};

bool XmoduleSensor::editCfgNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg) {
    // Try to update cfg, save if successful
    boolean success = cfgXmoduleSensor.updateFromWeb(argName(0), arg(0));
    if (success)
        mvp.config.writeCfg(cfgXmoduleSensor);
    return success;
}

bool XmoduleSensor::startActionNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg) {
    boolean success = true;
    switch (mvp.helper.hashStringDjb2(argName(0).c_str())) {

        case mvp.helper.hashStringDjb2("measureOffset"):
            measureOffset();
            mvp.net.netWeb.responseRedirect("Measuring offset, this may take a few seconds ...");
            break;

        case mvp.helper.hashStringDjb2("measureScaling"):
            if ( (args == 3) &&  (argName(1) == "valueNumber") && (argName(2) == "targetValue") ) {
                if (measureScaling(arg(1).toInt(), arg(2).toInt()))
                    mvp.net.netWeb.responseRedirect("Measuring scaling, this may take a few seconds ...");
                else // Input parameter check has failed
                    success = false;
            }
            break;

        case mvp.helper.hashStringDjb2("resetOffset"):
            resetOffset();
            mvp.net.netWeb.responseRedirect("Offset reset.");
            break;

        case mvp.helper.hashStringDjb2("resetScaling"):
            resetScaling();
            mvp.net.netWeb.responseRedirect("Scaling reset.");
            break;

        default: // Keyword not found
            success = false;
    }

    return success;
}


//////////////////////////////////////////////////////////////////////////////////

void XmoduleSensor::measurementHandler(int32_t *newSample) {
    dataCollection.addSample(newSample);

    if (dataCollection.avgCycleFinished) {
        if (offsetRunning || scalingRunning)
            measureOffsetScalingFinish();
        else // normal measurement
            newDataStored = true;
    }
}

//////////////////////////////////////////////////////////////////////////////////

Helper::NumberArray<int32_t> XmoduleSensor::currentMeasurementRaw() {
    Helper::NumberArray<int32_t> currentMeasurementRaw = Helper::NumberArray<int32_t>(cfgXmoduleSensor.dataValueCount, 0);
    currentMeasurementRaw.loopArray([&](int32_t& value, uint8_t i) {
        value = dataCollection.dataStore.getLatest()[i];
    });
    return currentMeasurementRaw;
}

Helper::NumberArray<int32_t> XmoduleSensor::currentMeasurementScaled() {
    Helper::NumberArray<int32_t> currentMeasurementScaled = Helper::NumberArray<int32_t>(cfgXmoduleSensor.dataValueCount, 0);
    currentMeasurementScaled.loopArray([&](int32_t& value, uint8_t i) {
        // SCALED = (RAW + offset) * scaling
        value = (dataCollection.dataStore.getLatest()[i] + dataProcessing.offset.values[i]) * dataProcessing.scaling.values[i];
    });
    return currentMeasurementScaled;
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
    dataProcessing.scalingTargetIndex = valueNumber - 1;
    dataProcessing.scalingTargetValue = targetValue;

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
        dataProcessing.setOffset(dataCollection.dataStore.getLatest());
    } else if (scalingRunning) {
        dataProcessing.setScaling(dataCollection.dataStore.getLatest());
    } else {
        mvp.logger.write(CfgLogger::Level::ERROR, "Offset/Scaling measurement finished without running.");
        return;
    }
    offsetRunning = false;
    scalingRunning = false;

    // Save
    mvp.config.writeCfg(dataProcessing);
    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Offset/Scaling measurement done in %d ms.", millis() - dataCollection.avgStartTime );

    // Restart data collection with new averaging
    dataCollection.setAveragingCountPtr(&cfgXmoduleSensor.sampleAveraging);

    // Restart interval, if set
    if (cfgXmoduleSensor.reportingInterval > 0)
        sensorDelay.start(cfgXmoduleSensor.reportingInterval);
}

void XmoduleSensor::resetOffset() {
    // dataProcessing.offset.reset();
    dataProcessing.offset.resetValues();
    mvp.config.writeCfg(dataProcessing);
}

void XmoduleSensor::resetScaling() {
    // dataProcessing.scaling.reset();
    dataProcessing.scaling.resetValues();
    mvp.config.writeCfg(dataProcessing);
}
