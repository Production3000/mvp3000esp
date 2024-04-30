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
#include <millisDelay.h> // https://github.com/PowerBroker2/SafeString

#include "Config.h"
#include "Xmodule.h"


struct CfgXmoduleSensor : Cfg {

    uint16_t sampleAveraging = 10; // Before values are reported
    uint16_t averagingOffsetScaling = 25;
    uint16_t reportingInterval = 0; // [ms], set to 0 to ignore

    // Saved settings
    CfgXmoduleSensor() {
        cfgName = "cfgXmoduleSensor";
        addSetting("sampleAveraging", &sampleAveraging, [&](uint16_t _x) { if (_x == 0) return false; else sampleAveraging = _x; return true; }); // at least 1
        addSetting("averagingOffsetScaling", &averagingOffsetScaling, [&](uint16_t _x) { if (_x == 0) return false; else averagingOffsetScaling = _x; return true; }); // at least 1
        addSetting("reportingInterval", &reportingInterval, [&](uint16_t _x) { reportingInterval = _x; return true; });
    };


    // Number of values recorded per measurement, for example one each for temperature, humidity, pressure -> 3
    uint8_t dataValueCount = 0;

    // Storing of averages, empiric maximum length of circular data buffer on ESP8266: 1x float 5000, 2x float 3500
    // More like much less, max 1000 for single value?
    uint16_t dataStoreLength = 10;


    // Float conversion, int = float * 10^exponent
    int8_t *floatToIntExponent;


    String infoDescription = "undefined";

    // Info used for output, data is a matrix with a row length
    // In case of dataMatrixColumnCount == dataValueCount it is obviously a single row
    uint8_t dataMatrixColumnCount = 255;



    // Only in code, not via web
    
    void setFloatToIntExponent(int8_t* _floatToIntExponent) {
        for (uint8_t i = 0; i < dataValueCount; i++) {
            floatToIntExponent[i] = _floatToIntExponent[i];
        }
    }
    void setFloatToIntExponentAll(int8_t _singleFloatToIntExponent) {
        for (uint8_t i = 0; i < dataValueCount; i++) {
            floatToIntExponent[i] = _singleFloatToIntExponent;
        }
    }

    void setMatrixColumnCount(uint8_t _dataMatrixColumnCount) {
        dataMatrixColumnCount = _dataMatrixColumnCount;
    }

    void initValueCount(uint8_t _dataValueCount) {
        dataValueCount = _dataValueCount;

        // Redefine floatToInt exponent
        delete [] floatToIntExponent;
        floatToIntExponent = new int8_t[dataValueCount];
        for (uint8_t i = 0; i < dataValueCount; i++) {
            floatToIntExponent[i] = 1;
        }

        // Dynamically reduce stored measurements depending on values measured                                                      // TODO
        // Overhead factor to take other memory uses into account
        // cfgXmoduleSensor.dataStoreLength = dataStoreMax / cfgXmoduleSensor.dataValueCount / 1.2;
    }

};



class XmoduleSensor : public Xmodule {
    private:
        CfgXmoduleSensor cfgXmoduleSensor;

        const char *cfgFileName = "cfgXmoduleSensorArray";

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

        // Offset and scaling
        int32_t *offset;
        float_t *scaling;
        boolean offsetRunning = false;
        boolean scalingRunning = false;
        int32_t scalingTargetValue;
        uint8_t scalingValueIndex;

        millisDelay sensorDelay;

        void initDataStore();
        void initOffset();
        void initScaling();

        void loadOffsetScaling();
        void saveOffsetScaling();

        void addSampleOffsetScaling(int32_t *newSample);



    public:
        // Constructor with value count
        XmoduleSensor(uint8_t valueCount) { cfgXmoduleSensor.initValueCount(valueCount); };

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

        void webResetOffset();
};

#endif
