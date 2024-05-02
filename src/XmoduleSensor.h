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

#ifndef MVP3000_XMODULESENSOR
#define MVP3000_XMODULESENSOR

#include <Arduino.h>
#include <ArduinoJson.h>
#include <millisDelay.h> // https://github.com/PowerBroker2/SafeString

#include "Config.h"
#include "Xmodule.h"


struct CfgXmoduleSensor : Cfg {

    // Modifiable settings saved to SPIFF

    uint16_t sampleAveraging = 10; // Before values are reported
    uint16_t averagingOffsetScaling = 25;
    uint16_t reportingInterval = 0; // [ms], set to 0 to ignore

    CfgXmoduleSensor() {
        cfgName = "cfgXmoduleSensor";
        addSetting("sampleAveraging", &sampleAveraging, [&](uint16_t _x) { if (_x == 0) return false; else sampleAveraging = _x; return true; }); // at least 1
        addSetting("averagingOffsetScaling", &averagingOffsetScaling, [&](uint16_t _x) { if (_x == 0) return false; else averagingOffsetScaling = _x; return true; }); // at least 1
        addSetting("reportingInterval", &reportingInterval, [&](uint16_t _x) { reportingInterval = _x; return true; });
    };

    // Settings that are not known during creating of this config but need init before anything works

    // Number of values recorded per measurement, for example one each for temperature, humidity, pressure -> 3
    uint8_t dataValueCount = 0;

    void initValueCount(uint8_t _dataValueCount) {
        dataValueCount = _dataValueCount;
    }

    // Fixed settings, restored with reboot to value set at compile

    String infoDescription = "TODO";

    // Storing of averages, empiric maximum length of circular data buffer on ESP8266: 1x float 5000, 2x float 3500
    // More likely much less, max 1000 for single value?
    uint16_t dataStoreLength = 10;

    // Used for output only
    // Data is a matrix with a row length, if dataMatrixColumnCount == dataValueCount it is obviously a single row
    uint8_t dataMatrixColumnCount = 255;
};


//////////////////////////////////////////////////////////////////////////////////

struct DataProcessing : public cfgStructJsonInterface {
    DataProcessing() { cfgName = "cfgDataProcessing"; }

    template <typename T>
    struct ProcessingArray {
        uint8_t dataValueCount = 0;
        int8_t defaultValue;
        T *values;

        ProcessingArray(){}
        ProcessingArray(uint8_t _dataValueCount, int8_t _defaultValue) {
            dataValueCount = _dataValueCount;
            defaultValue = _defaultValue;

            delete [] values;
            values = new T[dataValueCount];
            reset();
        }

        void reset() {
            for (uint8_t i = 0; i < dataValueCount; i++)
                values[i] = static_cast<T>(defaultValue);
        }

        bool exportToJsonArray(JsonArray &jsonArray) {
            boolean isCustom = false;
            for (uint8_t i = 0; i < dataValueCount; i++) {
                jsonArray.add(values[i]);
                if (values[i] != defaultValue) // Check if values are non-default
                    isCustom = true;
            }
            return isCustom;
        }

        bool importFromJsonArray(JsonArray &jsonArray) {
            // Make sure size is correct to not have memory issues
            if (jsonArray.size() != dataValueCount)
                return false;
            // Assign values
            uint8_t i = 0;
            for (JsonVariant value : jsonArray)
                values[i++] = value.as<T>(); // .as<T>() not defined for some types?
            return true;
        }

        void set(T* _values) {
            for (uint8_t i = 0; i < dataValueCount; i++)
                values[i] = _values[i];
        }

    };

    // Float conversion, int = float * 10^exponent
    ProcessingArray<int8_t> sampleToIntExponent;
    // Offset, shift data in y direction
    ProcessingArray<int32_t> offset;
    // Scaling, strech data in y direction
    ProcessingArray<float_t> scaling;

    void initValueCount(uint8_t dataValueCount) {
        sampleToIntExponent = ProcessingArray<int8_t>(dataValueCount, 0);
        offset = ProcessingArray<int32_t>(dataValueCount, 0);
        scaling = ProcessingArray<float_t>(dataValueCount, 1);
    }

    void exportToJson(JsonDocument &jsonDoc) {
        JsonArray jsonArray = jsonDoc.createNestedArray("offset");
        if (!offset.exportToJsonArray(jsonArray))
            jsonDoc.remove("offset");
        jsonArray = jsonDoc.createNestedArray("scaling");
        if (!scaling.exportToJsonArray(jsonArray))
            jsonDoc.remove("scaling");
    }

    bool importFromJson(JsonDocument &jsonDoc) {
        JsonArray jsonArray;
        // Assigns values only if varName exists
        if (jsonDoc.containsKey("offset") && jsonDoc["offset"].is<JsonArray>()) {
            jsonArray = jsonDoc["offset"].as<JsonArray>();
            if (!offset.importFromJsonArray(jsonArray))
                return false;
        }
        if (jsonDoc.containsKey("scaling") && jsonDoc["scaling"].is<JsonArray>()) {
            jsonArray = jsonDoc["scaling"].as<JsonArray>();
            if (!scaling.importFromJsonArray(jsonArray))
                return false;
        }
        return true;
    }
};


//////////////////////////////////////////////////////////////////////////////////

struct DataCollection {
    DataCollection() { };

    uint8_t dataValueCount = 0;

    uint16_t *averagingCount; // Pointer to cfgXmoduleSensor
    int32_t *offset; // Pointer to dataProcessing
    float_t *scaling; // Pointer to dataProcessing

    // Storing of averages, empiric maximum length of circular data buffer on ESP8266: 1x float 5000, 2x float 3500
    // More likely much less, max 1000 for single value?
    uint16_t dataStoreLength = 10;

    // Measurement storage 2D array
    int32_t **dataStore;
    // Measurement timestamp 1D array
    int32_t *dataStoreTime;

    // Position in array, is one ahead after the measurement
    uint8_t dataStoreHead;
    int32_t *currentMeasurementRaw() { return dataStore[dataStoreHead]; }

    // Only the last measurement is provided as offset-corrected and scaled
    int32_t *currentMeasurementScaled;

    // Total number of (likely averaged) measurements stored
    uint32_t measurementCount;

    // Data statistics
    int32_t *dataMax;
    int32_t *dataMin;

    // Temporary data storage for averaging
    int32_t *avgDataSum;
    uint8_t avgDataHead;

    // Time of first measurement in averaging cycle
    int32_t avgStartTime;

    boolean avgCycleFinished;

    void initValueCount(uint8_t _dataValueCount, uint16_t *_averagingCount, int32_t *_offset, float_t *_scaling) {
        dataValueCount = _dataValueCount;
        averagingCount = _averagingCount;
        offset = _offset;
        scaling = _scaling;

        // Dynamically reduce stored measurements depending on values measured                                                      // TODO
        // Overhead factor to take other memory uses into account
        // dataStoreLength = dataStoreMax / cfgXmoduleSensor.dataValueCount / 1.2;

        dataStore = (int32_t**) malloc(dataStoreLength * sizeof(int32_t*));
        for (uint8_t i = 0; i < dataStoreLength; i++) {
            dataStore[i] = (int32_t*) malloc((dataValueCount) * sizeof(int32_t));
        }

        delete [] dataStoreTime;
        dataStoreTime = new int32_t[dataStoreLength];

        delete [] currentMeasurementScaled;
        currentMeasurementScaled = new int32_t[dataValueCount];

        delete [] avgDataSum;
        avgDataSum = new int32_t[dataValueCount];

        // Max/min values
        delete [] dataMax;
        dataMax = new int32_t[dataValueCount];
        delete [] dataMin;
        dataMin = new int32_t[dataValueCount];

        reset();
    }

    void setAveragingCount(uint16_t *_averagingCount) {
        averagingCount = _averagingCount;
        reset();
    }

    void reset() {
        for (uint8_t i = 0; i < dataStoreLength; i++) {
            for (uint8_t j = 0; j < dataValueCount; j++) {
                dataStore[i][j] = 0;
            }
            dataStoreTime[i] = 0;
        }
        dataStoreHead = 0;
        measurementCount = 0;

        for (uint8_t i = 0; i < dataValueCount; i++) {
            avgDataSum[i] = 0;
            currentMeasurementScaled[i] = 0;
            dataMax[i] = std::numeric_limits<int32_t>::min();
            dataMin[i] = std::numeric_limits<int32_t>::max();
        }
        avgDataHead = 0;
        avgStartTime = 0;
        avgCycleFinished = false;
    }

    void addSample(int32_t *newSample) {
        // This is the function to do most of the work

        // Add new values to existing sums, remember max/min extremes
        for (uint8_t i = 0; i < dataValueCount; i++) {
            avgDataSum[i] += newSample[i];
            dataMin[i] = min(dataMin[i], newSample[i]);
            dataMax[i] = max(dataMax[i], newSample[i]);
        }

        // Restart averaging cycle
        if (avgDataHead == 0) {
            avgStartTime =  millis();
            avgCycleFinished = false;
        }

        // Increment averaging head
        avgDataHead++;

        // Check if averaging count is reached
        if (avgDataHead >= *averagingCount) {

            // Write and read of the stored values are not simultaneously, but dataStoreHead should always point to
            // the correct/current index. Thus, incrementing and check needs to be done just before the new write.
            // Increment head except for very first measurement
            if (measurementCount != 0)
                dataStoreHead++;
            // Check if end of circular buffer is reached, done at the beginning for any depending data use
            if (dataStoreHead >= dataStoreLength)
                dataStoreHead = 0;

            // Calculate and store averages, reset sums and head
            for (uint8_t j = 0; j < dataValueCount; j++) {
                dataStore[dataStoreHead][j] = nearbyintf( avgDataSum[j] / *averagingCount );
                currentMeasurementScaled[j] = nearbyintf( (dataStore[dataStoreHead][j] + offset[j]) * scaling[j] );
                avgDataSum[j] = 0;
            }
            dataStoreTime[dataStoreHead] = nearbyintf( (avgStartTime + millis()) / 2 );
            avgStartTime = 0;

            // Reset averaging
            avgDataHead = 0;
            measurementCount++;

            // Flag new data added for further actions in loop()
            avgCycleFinished = true;
        }
    }
};


//////////////////////////////////////////////////////////////////////////////////

class XmoduleSensor : public Xmodule {
    public:
        // Constructor to re-init arrays for changed value count
        XmoduleSensor(uint8_t valueCount) {
            cfgXmoduleSensor.initValueCount(valueCount);
            dataProcessing.initValueCount(valueCount);
            dataCollection.initValueCount(valueCount, &cfgXmoduleSensor.sampleAveraging, dataProcessing.offset.values, dataProcessing.scaling.values);
        };

        void setup();
        void loop();

        void contentModuleNetWeb();
        bool editCfgNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg);
        bool startActionNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg);

        // Module custom functions

        template <typename T>
        void addSample(T *newSample)  {
            // Shift data by decimals
            int32_t decimalShiftedSample[cfgXmoduleSensor.dataValueCount];
            for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
                decimalShiftedSample[i] = nearbyintf( newSample[i] * pow10(dataProcessing.sampleToIntExponent.values[i]) );
            }
            measurementHandler(decimalShiftedSample);
        };
        void measurementHandler(int32_t *newSample);

        void measureOffset();
        bool measureScaling(uint8_t valueNumber, int32_t targetValue);
        void resetOffset();
        void resetScaling();

        // bool isPeriodic(uint8_t length);

    private:
        CfgXmoduleSensor cfgXmoduleSensor;
        DataProcessing dataProcessing;
        DataCollection dataCollection;

        millisDelay sensorDelay;

        boolean newDataStored;

        // Measure offset and scaling
        boolean offsetRunning = false;
        boolean scalingRunning = false;
        int32_t scalingTargetValue;
        uint8_t scalingValueIndex;

        void measureOffsetCalculate();
        void measureScalingCalculate();
        void measureOffsetScalingFinish();

};

#endif
