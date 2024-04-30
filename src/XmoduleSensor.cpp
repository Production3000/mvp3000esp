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

    // Read config
    mvp.config.readCfg(cfgXmoduleSensor);

    if (cfgXmoduleSensor.dataValueCount == 0) {
        mvp.logger.write(CfgLogger::Level::ERROR, "Data value count is zero.");
        return;
    }

    // This is dynamic allocation, however it is done only once at startup so at least there is no fragmentation over time
    // Memory is not freed, on class destruction:
    //  ~XmoduleSensor() {
    //      for (int i = 0; i < dataStoreLength; i++) { free(dataStore[i]); }
    //      free(dataStore);
    //  }
    dataStore = (int32_t**) malloc(cfgXmoduleSensor.dataStoreLength * sizeof(int32_t*));
    for (uint8_t i = 0; i < cfgXmoduleSensor.dataStoreLength; i++) {
        dataStore[i] = (int32_t*) malloc((cfgXmoduleSensor.dataValueCount) * sizeof(int32_t));
    }

    delete [] dataStoreTime;
    dataStoreTime = new int32_t[cfgXmoduleSensor.dataStoreLength];

    delete [] avgDataSum;
    avgDataSum = new int32_t[cfgXmoduleSensor.dataValueCount];

    // Max/min values
    delete [] dataMax;
    dataMax = new int32_t[cfgXmoduleSensor.dataValueCount];
    delete [] dataMin;
    dataMin = new int32_t[cfgXmoduleSensor.dataValueCount];

    initDataStore();

    // Offset and scaling
    delete [] offset;
    offset = new int32_t[cfgXmoduleSensor.dataValueCount];
    delete [] scaling;
    scaling = new float_t[cfgXmoduleSensor.dataValueCount];

    initOffset();
    initScaling();

    loadOffsetScaling();

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
        
        // Output data to serial or network
        mvp.logger.writeCSV(CfgLogger::Level::DATA, dataStore[dataStoreHead], cfgXmoduleSensor.dataValueCount, cfgXmoduleSensor.dataMatrixColumnCount);
   }
}


//////////////////////////////////////////////////////////////////////////////////

void XmoduleSensor::netWebContentModule() {
    mvp.net.netWeb.sendFormatted("<h1>Module</h1>");

    // Sensor info
    mvp.net.netWeb.sendFormatted("\
        <h3>Sensor</h3> <ul> \
        <li>...: %s </li> </ul>",
        cfgXmoduleSensor.infoDescription);

    // Settings
    mvp.net.netWeb.sendFormatted("\
        <h3>Data Handling</h3> <ul> \
        <li>Sample averaging:<br> <form action='/save' method='post'> <input name='sampleAveraging' value='%d' type='number' min='1' max='255'> <input type='submit' value='Save'> </form> </li> \
        <li>Averaging of offset and scaling measurements:<br> <form action='/save' method='post'> <input name='averagingOffsetScaling' value='%d' type='number' min='1' max='255'> <input type='submit' value='Save'> </form> </li> \
        <li>Reporting interval, minimum time for fast sensors, zero is ignore:<br> <form action='/save' method='post'> <input name='reportingInterval' value='%d' type='number' min='0' max='65535'> [ms] <input type='submit' value='Save'> </form> </li>",
        cfgXmoduleSensor.sampleAveraging, cfgXmoduleSensor.averagingOffsetScaling, cfgXmoduleSensor.reportingInterval);

    // Table for offset, scaling, float2int
    mvp.net.netWeb.sendFormatted("<h3>Sensor, Offset, Scaling</h3> <table> <tr> <td>#</td> <td>Type</td> <td>Unit</td> <td>Offset</td><td>Scaling</td><td>Float to Int exp. 10<sup>x</sup></td> </tr>");
    for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
        // Type, units, default and current offset/scaling
        mvp.net.netWeb.sendFormatted("<tr> <td>%d</td> <td>%s</td> <td>%s</td> <td>%d</td> <td>%.2e</td> <td>%d</td> </tr>",
            i+1, "x", "x", offset[i], scaling[i], cfgXmoduleSensor.floatToIntExponent[i]);
    }
    mvp.net.netWeb.sendFormatted("\
        <tr> <td colspan='3'></td> \
        <td> <form action='/save' method='post' onsubmit='return confirm(`Measure offset?`);'> <input name='measureOffset' type='hidden'> <input type='submit' value='Measure offset'> </form> </td> \
        <td> <form action='/save' method='post' onsubmit='return confirm(`Measure scaling?`);'> <input name='measureScaling' type='hidden'> Value number #<br> <input name='valueNumber' type='number' min='1' max='%d'><br> Target setpoint<br> <input name='targetValue' type='number'><br> <input type='submit' value='Measure scaling'> </form> </td> \
        <td></td> </tr>  <tr> <td colspan='3'></td> \
        <td> <form action='/save' method='post' onsubmit='return confirm(`Reset offset?`);'> <input name='resetOffset' type='hidden'> <input type='submit' value='Reset offset'> </form> </td> \
        <td> <form action='/save' method='post' onsubmit='return confirm(`Reset scaling?`);'> <input name='resetScaling' type='hidden'> <input type='submit' value='Reset scaling'> </form> </td> \
        <td></td> </tr> </table>",
        cfgXmoduleSensor.dataValueCount);
};

bool XmoduleSensor::editCfgNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg) {

    Serial.println(argName(0));
    Serial.println(arg(0));

    // Try to update cfg, save if successful
    bool success = cfgXmoduleSensor.updateFromWeb(argName(0), arg(0));
    if (success) {
        mvp.config.writeCfg(cfgXmoduleSensor);
        mvp.net.netWeb.responseRedirect("Setting saved.");
        return true;
    }


    Serial.println("asd");
    
    success = true;
    switch (mvp.helper.hashStringDjb2(argName(0).c_str())) {                                                             // TODO separate actions
        // Actions
        case mvp.helper.hashStringDjb2("measureOffset"):
            measureOffset();
            mvp.net.netWeb.responseRedirect("Measuring offset, this may take a few seconds ...");
            break;
        case mvp.helper.hashStringDjb2("measureScaling"):
        //     if ( (server.args() == 2) &&  (server.argName(0) == "valueNumber") && (server.argName(1) == "targetValue") ) {
//         // Function checks bounds of valueNumber
//         // if (mvp.sensorHandler.measureScaling(server.arg(0).toInt(), server.arg(1).toInt())) {
//         //     responseRedirect("Measuring scaling, this may take a few seconds ...");
//         //     return;
//         // }
//     }
//     responseRedirect("Input error!");
            mvp.net.netWeb.responseRedirect("Measuring scaling, this may take a few seconds ...");
            break;
        case mvp.helper.hashStringDjb2("resetOffset"):
            resetOffset();
            mvp.net.netWeb.responseRedirect("Offset reset.");
            break;
        case mvp.helper.hashStringDjb2("resetScaling"):
            resetScaling();
            mvp.net.netWeb.responseRedirect("Scaling reset.");
            break;
        default:
            success = false;
            Serial.println("qwe");
    }

    
    Serial.println("yxc");

    return success;
}


//////////////////////////////////////////////////////////////////////////////////

void XmoduleSensor::initDataStore() {
    for (uint8_t i = 0; i < cfgXmoduleSensor.dataStoreLength; i++) {
        for (uint8_t j = 0; j < cfgXmoduleSensor.dataValueCount; j++) {
            dataStore[i][j] = 0;
        }
        dataStoreTime[i] = 0;
    }
    dataStoreHead = 0;
    measurementCount = 0;

    for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
        dataMax[i] = std::numeric_limits<int32_t>::min();
        dataMin[i] = std::numeric_limits<int32_t>::max();

        avgDataSum[i] = 0;
        avgDataTime = 0;
        avgDataHead = 0;
    }

    newDataStored = false;
}

void XmoduleSensor::initOffset() { for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) offset[i] = 0; }
void XmoduleSensor::initScaling() {
    for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
        scaling[i] = 1.0f;
    }
}

void XmoduleSensor::resetOffset() { 
    initOffset();
    // Save changes
    saveOffsetScaling();
 }
void XmoduleSensor::resetScaling() {
    initScaling();
    // Save changes
    saveOffsetScaling();
}

void XmoduleSensor::loadOffsetScaling() {                                                                   // TODO move all this into cfg
    // Load saved values
    if (mvp.config.readFileToJson(cfgFileName)) {
        // Confirm stored array is of correct length, relevant only after flashing the device
        uint8_t storedValueCount;
        mvp.config.cfgReadGetValue("valueCount", storedValueCount);
        if (storedValueCount != cfgXmoduleSensor.dataValueCount) {
            mvp.config.removeCfg(cfgFileName);
            return;
        }

        mvp.config.cfgReadGetValue("offset", offset, storedValueCount);
        mvp.config.cfgReadGetValue("scaling", scaling, storedValueCount);
    }
    // mvp.config.cfgReadClose();
}
void XmoduleSensor::saveOffsetScaling() {
    // Check if values are actually custom
    boolean customOffset = false;
    boolean customScaling = false;
    for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
        if(offset[i] != 0)
            customOffset = true;
        if(scaling[i] != 1.0f)
            customScaling = true;
    }

    if (!customOffset && ! customScaling){
        // All generic values, remove saved config file
        mvp.config.removeCfg(cfgFileName);
        return;
    }

    // mvp.config.cfgWritePrepare();
    mvp.config.cfgWriteAddValue("valueCount", cfgXmoduleSensor.dataValueCount);
    if (customOffset)
        mvp.config.cfgWriteAddValue("offset", offset, cfgXmoduleSensor.dataValueCount);
    if (customScaling)
        mvp.config.cfgWriteAddValue("scaling", scaling, cfgXmoduleSensor.dataValueCount);
    mvp.config.writeJsonToFile(cfgFileName);
}


//////////////////////////////////////////////////////////////////////////////////

void XmoduleSensor::measureOffset() {
    if (offsetRunning || scalingRunning)
        return;

    // Stop interval
    sensorDelay.stop();
    // Clear data
    initDataStore();
    offsetRunning = true;

    mvp.logger.write(CfgLogger::Level::INFO, "Offset measurement started.");
}

bool XmoduleSensor::measureScaling(uint8_t valueNumber, int32_t targetValue) {
    if (offsetRunning || scalingRunning)
        return true;

    // Numbering starts from 1 in the real world!
    if ((valueNumber == 0) || (valueNumber > cfgXmoduleSensor.dataValueCount)) {
        mvp.logger.writeFormatted(CfgLogger::Level::WARNING, "Scaling measurement valueNumber out of bounds.");
        return false;
    }

    scalingTargetValue = targetValue;
    scalingValueIndex = valueNumber - 1;
    
    // Stop interval
    sensorDelay.stop();
    // Clear data
    initDataStore();
    scalingRunning = true;

    mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Scaling measurement of index %d started.", scalingValueIndex);
    return true;
}



void XmoduleSensor::webResetOffset() {
    resetOffset();
    mvp.net.netWeb.responseRedirect("Offset reset.");
}

//////////////////////////////////////////////////////////////////////////////////

void XmoduleSensor::addSample(float_t *newSample) {
    // Convert float to int, shift by decimals
    int32_t newSampleInt[cfgXmoduleSensor.dataValueCount];
    for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
        newSampleInt[i] = newSample[i] * pow10(cfgXmoduleSensor.floatToIntExponent[i]);
    }
    addSample(newSampleInt);
}

void XmoduleSensor::addSample(int32_t *newSample) {
    // This is the function to do most of the work

    if (offsetRunning || scalingRunning) {
        addSampleOffsetScaling(newSample);
        return;
    }

    // Add new values to existing sums, remember max/min extremes
    for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
        int32_t newSampleScaled = nearbyintf((float_t)(newSample[i] + offset[i]) * scaling[i]);
        avgDataSum[i] += newSampleScaled;
        if (newSampleScaled < dataMin[i])
            dataMin[i] = newSampleScaled;
        if (newSampleScaled > dataMax[i])
            dataMax[i] = newSampleScaled;
    }
    // Attach millis as last value
    avgDataTime +=  millis();

    // Increment averaging head
    avgDataHead++;
    
    // Check if averaging count is reached
    if (avgDataHead >= cfgXmoduleSensor.sampleAveraging) {

        // Write and read of the stored values are not simultaneously, but dataStoreHead should always point to
        // the correct/current index. Thus, incrementing and check needs to be done just before the new write.
        // Increment head except for very first measurement
        if (measurementCount > 0)
            dataStoreHead++;
        // Check if end of circular buffer is reached, done at the beginning for any depending data use
        if (dataStoreHead >= cfgXmoduleSensor.dataStoreLength)
            dataStoreHead = 0;
        
        // Calculate and store averages, reset sums and head
        for (uint8_t j = 0; j < cfgXmoduleSensor.dataValueCount; j++) {
            dataStore[dataStoreHead][j] = nearbyintf( avgDataSum[j] / cfgXmoduleSensor.sampleAveraging );
            avgDataSum[j] = 0;
        }
        dataStoreTime[dataStoreHead] = avgDataTime / cfgXmoduleSensor.sampleAveraging;
        avgDataTime = 0;

        // Reset averaging
        avgDataHead = 0;
        measurementCount++;

        // Flag new data added for further actions in loop()
        newDataStored = true;
    }
}

void XmoduleSensor::addSampleOffsetScaling(int32_t *newSample) {

    // Add new values to existing sums, remember max/min extremes
    for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
        avgDataSum[i] += newSample[i];
    }

    // Only store start time
    if (avgDataHead == 0)
        avgDataTime =  millis();

    // Increment averaging head
    avgDataHead++;
    
    // Check if averaging count is reached
    if (avgDataHead >= cfgXmoduleSensor.averagingOffsetScaling) {

        if (offsetRunning) {
            for (uint8_t j = 0; j < cfgXmoduleSensor.dataValueCount; j++) {
                // OFFSET = -1 * sum/times
                offset[j] = - nearbyintf(avgDataSum[j] / cfgXmoduleSensor.averagingOffsetScaling);
            }
            offsetRunning = false;
        }
        if (scalingRunning) {
            // SCALING = TARGETVALUE / (sum/times + OFFSET)
            scaling[scalingValueIndex] = (float_t)scalingTargetValue / ( (avgDataSum[scalingValueIndex] / cfgXmoduleSensor.averagingOffsetScaling) + offset[scalingValueIndex] );
            scalingRunning = false;
            scalingTargetValue = NULL;
            scalingValueIndex = NULL;
        }

        saveOffsetScaling();

        mvp.logger.writeFormatted(CfgLogger::Level::INFO, "Offset/Scaling measurement done in %d ms.", millis() - avgDataTime);

        // Resume normal measurements
        initDataStore();
        // Restart interval, if set
        if (cfgXmoduleSensor.reportingInterval > 0)
            sensorDelay.start(cfgXmoduleSensor.reportingInterval);
    }

}


//////////////////////////////////////////////////////////////////////////////////

// bool XmoduleSensor::isPeriodic(uint8_t length) {
//     // Need at least 3 values to check for equal spacing
//     // Default to true for one and  two measurements
//     if ((measurementCount < length) || (length < 3) || (length > cfgXmoduleSensor.dataStoreLength))
//         return true;

//     uint8_t end = circularIndex(dataStoreHead - 1 -1);
//     uint8_t position = circularIndex(dataStoreHead - length-1);
//     uint8_t nextposition = circularIndex(dataStoreHead - length + 1-1);
    
//     // Initial spacing, max is 65.5 s
//     int32_t spacing = dataStoreTime[nextposition] - dataStoreTime[position];

//     // Check other spacings
//     while (position != end) {
//         position = circularIndex(++position);
//         nextposition = circularIndex(++nextposition);
//         int32_t currentSpacing = dataStoreTime[nextposition] - dataStoreTime[position];
//         // Tolerance of 5% - 5 ms at 10 Hz - 1 ms at 50 Hz
//         if (abs(currentSpacing - spacing) > spacing * 0.05) {
//             mvp.logger.write(CfgLogger::Level::WARNING, "Data not periodic.");
//             return false;
//         }
//     }

//     return true;
// }
