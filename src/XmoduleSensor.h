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

        delete [] sensorTypes;
        sensorTypes = new String[dataValueCount];
        delete [] sensorUnits;
        sensorUnits = new String[dataValueCount];
        for (u_int8_t i = 0; i < dataValueCount; i++) {
            sensorTypes[i] = "n/a";
            sensorUnits[i] = "n/a";
        }
    }

    // Fixed settings, restored with reboot to value set at compile

    String infoName = "n/a";
    String infoDescription = "n/a";
    String *sensorTypes;
    String *sensorUnits;
    void setSensorInfo(String _infoName,String _infoDescription, String *_sensorTypes, String *_sensorUnits) {
        setSensorInfo(_infoName, _infoDescription);
        sensorTypes = _sensorTypes;
        sensorUnits = _sensorUnits;
    }
    void setSensorInfo(String _infoName,String _infoDescription) {
        infoName = _infoName;
        infoDescription = _infoDescription;
    }

    // Data is a matrix with a row length, if dataMatrixColumnCount == dataValueCount it is obviously a single row
    // Used for output only
    uint8_t dataMatrixColumnCount = 255;
};


//////////////////////////////////////////////////////////////////////////////////

struct DataProcessing : public CfgStructJsonInterface {
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

    // Storing of averages, empiric maximum length of circular data buffer on ESP8266: 1x float 5000, 2x float 3500
    // More likely much less, max 1000 for single value?
    uint16_t dataStoreLength = 5;
    Helper::LinkedList<int32_t, true> dataStore = Helper::LinkedList<int32_t, true>(dataStoreLength);
    Helper::LinkedList<int32_t> dataStoreTime = Helper::LinkedList<int32_t>(dataStoreLength);;

    // Averaging
    Helper::NumberArray<int32_t> avgDataSum; // Temporary data storage for averaging
    uint16_t *averagingCount; // Pointer to cfgXmoduleSensor
    uint8_t avgCounter; // Counter for averaging
    int32_t avgStartTime; // Time of first measurement in averaging cycle
    boolean avgCycleFinished; // Flag for new data added to dataStore

    // Data statistics
    Helper::NumberArray<int32_t> dataMax;
    Helper::NumberArray<int32_t> dataMin;


    void initValueCount(uint8_t _dataValueCount, uint16_t *_averagingCount) {
        dataValueCount = _dataValueCount;
        averagingCount = _averagingCount;

        // Init all NumberArrays
        avgDataSum = Helper::NumberArray<int32_t>(dataValueCount);
        dataMax = Helper::NumberArray<int32_t>(dataValueCount, std::numeric_limits<int32_t>::min());
        dataMin = Helper::NumberArray<int32_t>(dataValueCount, std::numeric_limits<int32_t>::max());

        reset();
    }

    void setAveragingCount(uint16_t *_averagingCount) {
        averagingCount = _averagingCount;
        reset();
    }

    void reset() {
        // Averaging
        avgDataSum.resetValues();
        dataMax.resetValues();
        dataMin.resetValues();

        // Counters and such
        avgCounter = 0;
        avgStartTime = 0;
        avgCycleFinished = false;

        // Data storage
        dataStore.clear();
        dataStoreTime.clear();
    }

    void addSample(int32_t *newSample) {
        // This is the function to do most of the work

        // Add new values to existing sums, remember max/min extremes
        avgDataSum.loopArray([&](int32_t& value, uint16_t i) { value += newSample[i]; } ); // Add new value for later averaging
        dataMax.loopArray([&](int32_t& value, uint16_t i) { value = max(value, newSample[i]); } ); // All-time max
        dataMin.loopArray([&](int32_t& value, uint16_t i) { value = min(value, newSample[i]); } ); // All-time min

        // Averaging cycle restarted, init
        if (avgCounter == 0) {
            avgStartTime =  millis();
            avgCycleFinished = false;
        }
        // Increment averaging head
        avgCounter++;

        // Check if averaging count is reached
        if (avgCounter >= *averagingCount) {

            // Calculate data and time averages and store
            avgDataSum.loopArray([&](int32_t& value, uint16_t i) { value = nearbyintf( value / *averagingCount ); } );
            dataStore.append(avgDataSum);
            dataStoreTime.append(nearbyintf( (avgStartTime + millis()) / 2 ));

            // Reset temporary values, counters            
            avgDataSum.resetValues();
            avgCounter = 0;
            avgStartTime = 0;
            // Flag new data added for further actions in loop()
            avgCycleFinished = true;
        }
    }
};


//////////////////////////////////////////////////////////////////////////////////

class XmoduleSensor : public Xmodule {
    public:
        CfgXmoduleSensor cfgXmoduleSensor;

        // Constructor to re-init arrays for changed value count
        XmoduleSensor(uint8_t valueCount) {
            cfgXmoduleSensor.initValueCount(valueCount);
            dataProcessing.initValueCount(valueCount);
            dataCollection.initValueCount(valueCount, &cfgXmoduleSensor.sampleAveraging); // Averaging can change during operation
        };

        void setup() override;
        void loop() override;

        void contentModuleNetWeb() override;
        bool editCfgNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg) override;
        bool startActionNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg) override;

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

        Helper::NumberArray<int32_t> currentMeasurementRaw();
        Helper::NumberArray<int32_t> currentMeasurementScaled();

        void measureOffset();
        bool measureScaling(uint8_t valueNumber, int32_t targetValue);
        void resetOffset();
        void resetScaling();

        void setSampleToIntExponent(int8_t *_sampleToIntExponent) { 
            dataProcessing.sampleToIntExponent.set(_sampleToIntExponent);
        };

        // bool isPeriodic(uint8_t length);

    private:
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
