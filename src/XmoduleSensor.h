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

    // Settings that are not knownn during creating of this config but need init before anything works

    // Number of values recorded per measurement, for example one each for temperature, humidity, pressure -> 3
    uint8_t dataValueCount = 0;

    void initValueCount(uint8_t _dataValueCount) {
        dataValueCount = _dataValueCount;

        // Dynamically reduce stored measurements depending on values measured                                                      // TODO
        // Overhead factor to take other memory uses into account
        // cfgXmoduleSensor.dataStoreLength = dataStoreMax / cfgXmoduleSensor.dataValueCount / 1.2;
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


struct DataProcessing : public cfgStructJsonInterface {
    DataProcessing() { cfgName = "cfgDataProcessing"; } // cfgDataProcessing

    uint8_t dataValueCount = 0;

    template <typename T>
    struct ProcessingArray {
        int8_t defaultValue;
        uint8_t dataValueCount = 0;
        T *value;

        ProcessingArray(){}
        ProcessingArray(uint8_t _dataValueCount, int8_t _defaultValue) { 
            dataValueCount = _dataValueCount;
            defaultValue = _defaultValue;

            delete [] value;
            value = new T[dataValueCount];
            reset();
        }

        void reset() {
            for (uint8_t i = 0; i < dataValueCount; i++)
                value[i] = static_cast<T>(defaultValue);
        }

        bool exportToJsonArray(JsonArray &jsonArray) {
            boolean isCustom = false;
            for (uint8_t i = 0; i < dataValueCount; i++) {
                jsonArray.add(value[i]);
                if (value[i] != defaultValue) // Check if values are non-default
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
            for (JsonVariant _value : jsonArray)
                value[i++] = _value.as<T>(); // .as<T>() not defined for uint16_t and some others
            return true;
        }

        void set(T* _value) {
            for (uint8_t i = 0; i < dataValueCount; i++)
                value[i] = _value[i];
        }

    };

    // Float conversion, int = float * 10^exponent
    ProcessingArray<int8_t> sampleToIntExponent;
    // Offset, shift data in y direction
    ProcessingArray<int32_t> offset;
    // Scaling, strech data in y direction
    ProcessingArray<float_t> scaling;

    void initValueCount(uint8_t _dataValueCount) {
        dataValueCount = _dataValueCount;

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



class XmoduleSensor : public Xmodule {
    private:
        CfgXmoduleSensor cfgXmoduleSensor;
        DataProcessing dataProcessing;

        // Measurement storage 2D array
        int32_t **dataStore;
        // Measurement timestamp 1D array
        int32_t *dataStoreTime;

        // Position in array, is one ahead after the measurement
        uint8_t dataStoreHead;
        // uint8_t circularIndex(uint8_t value) { return (cfgXmoduleSensor.dataStoreLength + value) % cfgXmoduleSensor.dataStoreLength; };
        // int32_t *currentMeasurement() { return dataStore[circularIndex(dataStoreHead - 1)]; }
        // int32_t currentMeasurementMillis() { return dataStoreTime[circularIndex(dataStoreHead - 1)]; }

        // Total number of (likely averaged) measurements stored
        uint32_t measurementCount;

        // Data statistics
        int32_t *dataMax;
        int32_t *dataMin;
        
        // Temporary data/time storage for averaging
        int32_t *avgDataSum;
        int32_t avgDataTime;
        uint8_t avgDataHead;

        boolean newDataStored;

        // Measure offset and scaling
        boolean offsetRunning = false;
        boolean scalingRunning = false;
        int32_t scalingTargetValue;
        uint8_t scalingValueIndex;

        millisDelay sensorDelay;

        void initDataStore();

        void addSampleOffsetScaling(int32_t *newSample);


    public:
        // Constructor with value count to re-init arrays
        XmoduleSensor(uint8_t valueCount) {
            cfgXmoduleSensor.initValueCount(valueCount);
            dataProcessing.initValueCount(valueCount);
        };

        void setup();
        void loop();
        void netWebContentModule();
        bool editCfgNetWeb(int args, std::function<String(int)> argName, std::function<String(int)> arg);

        ///

        void addSample(float_t *newSample);
        void addSample(int32_t *newSample);

        void measureOffset();
        bool measureScaling(uint8_t valueNumber, int32_t targetValue);
        void resetOffset();
        void resetScaling();

        // bool isPeriodic(uint8_t length);

};

#endif
