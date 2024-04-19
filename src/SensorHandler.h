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

#ifndef MVP3000_SENSORNEWHANDLER
#define MVP3000_SENSORNEWHANDLER

#include <Arduino.h>
#include <millisDelay.h> // https://github.com/PowerBroker2/SafeString


struct CfgSensorHandler {

    // Number of values recorded per measurement, for example one each for temperature, humidity, pressure -> 3
    uint8_t dataValueCount = 0;

    // Storing of averages, empiric maximum length of circular data buffer on ESP8266: 1x float 5000, 2x float 3500
    // More like much less, max 1000 for single value?
    uint16_t dataStoreLength = 10;

    uint8_t sampleAveraging = 10; // Before values are stored                                                                      // TODO set via network
    uint8_t sampleAveragingOffsetScaling = 25;

    // Float conversion, int = float * 10^exponent
    int8_t *floatToIntExponent;

    uint16_t reportingInterval = 0; // [ms], set to 0 to ignore                                                                     // TODO set via network

    String infoDescription = "undefined";

    // Info used for output, data is a matrix with a row length
    // In case of dataMatrixColumnCount == dataValueCount it is obviously a single row
    uint8_t dataMatrixColumnCount = 255;


    // Dummy constructor needed for some reason                                                                                    // TODO can we get rid of this
    CfgSensorHandler() {};

    // Init with value count, no decimal shift for floats with exponent 0 
    CfgSensorHandler(uint8_t _dataValueCount) : dataValueCount(_dataValueCount) {
        // Redefine floatToInt exponent
        delete [] floatToIntExponent;
        floatToIntExponent = new int8_t[dataValueCount];
        for (uint8_t i = 0; i < dataValueCount; i++) {
            floatToIntExponent[i] = 1;
        }

        // Dynamically reduce stored measurements depending on values measured                                                      // TODO
        // Overhead factor to take other memory uses into account
        // cfgSensorHandler.dataStoreLength = dataStoreMax / cfgSensorHandler.dataValueCount / 1.2;
    };


    void setAveraging(uint8_t _sampleAveraging) {
        sampleAveraging = max(_sampleAveraging, (uint8_t)1); // at least 1
    };

    void setFloatToIntExponent(int8_t* _floatToIntExponent) {
        for (uint8_t i = 0; i < dataValueCount; i++) {
            floatToIntExponent[i] = _floatToIntExponent[i];
        }
    };
    void setFloatToIntExponentAll(int8_t _singleFloatToIntExponent) {
        for (uint8_t i = 0; i < dataValueCount; i++) {
            floatToIntExponent[i] = _singleFloatToIntExponent;
        }
    };

    void setMatrixColumnCount(uint8_t _dataMatrixColumnCount) {
        dataMatrixColumnCount = _dataMatrixColumnCount;
    };

};


class SensorHandler {
    private:
        CfgSensorHandler cfgSensorHandler;

        const char *cfgFileName = "cfgSensorHandler";

        // Measurement storage 2D array
        int32_t **dataStore;
        // Measurement timestamp 1D array
        int32_t *dataStoreTime;

        // Position in array, is one ahead after the measurement
        uint8_t dataStoreHead;
        // uint8_t circularIndex(uint8_t value) { return (cfgSensorHandler.dataStoreLength + value) % cfgSensorHandler.dataStoreLength; };
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

        void setup(CfgSensorHandler _cfgSensorHandler);
        void loop();

        void addSample(float_t *newSample);
        void addSample(int32_t *newSample);

        void measureOffset();
        bool measureScaling(uint8_t valueNumber, int32_t targetValue);
        void resetOffset();
        void resetScaling();

        // bool isPeriodic(uint8_t length);

        void printWeb();
        void webResetOffset();

};

#endif