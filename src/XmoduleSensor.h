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

#include "XmoduleSensor_DataCollection.h"
#include "XmoduleSensor_DataProcessing.h"


struct CfgXmoduleSensor : CfgJsonInterface {

    // Modifiable settings saved to SPIFF

    uint16_t sampleAveraging = 10; // Before values are reported
    uint16_t averagingOffsetScaling = 25;
    uint16_t reportingInterval = 0; // [ms], set to 0 to ignore

    CfgXmoduleSensor() : CfgJsonInterface("cfgXmoduleSensor") {
        addSetting<uint16_t>("sampleAveraging", &sampleAveraging, [](uint16_t x) { return (x == 0) ? false : true; }); // at least 1
        addSetting<uint16_t>("averagingOffsetScaling", &averagingOffsetScaling, [](uint16_t x) { return (x == 0) ? false : true; }); // at least 1
        addSetting<uint16_t>("reportingInterval", &reportingInterval, [](uint16_t _) { return true; });
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

class XmoduleSensor : public Xmodule {

    public:
        CfgXmoduleSensor cfgXmoduleSensor;

        // Constructor to re-init arrays for changed value count
        XmoduleSensor(uint8_t valueCount) {
            cfgXmoduleSensor.initValueCount(valueCount);
            dataProcessing.initDataValueSize(valueCount);
            dataCollection.initDataValueSize(valueCount); // Averaging can change during operation
        };


        void setup() override;
        void loop() override;

        /**
         * @brief Add a new sample array to the sensor module.
         * 
         * This method is used to add a new sample array to the sensor module.
         * 
         * @tparam T The numeric type of the sample, typically int or float.
         * @param newSample The new sample array to add.
         */
        template <typename T>
        void addSample(T *newSample)  {
            // Shift data by decimals
            int32_t decimalShiftedSample[cfgXmoduleSensor.dataValueCount];
            for (uint8_t i = 0; i < cfgXmoduleSensor.dataValueCount; i++) {
                decimalShiftedSample[i] = nearbyintf( newSample[i] * pow10(dataProcessing.sampleToIntExponent.values[i]) );
            }
            measurementHandler(decimalShiftedSample);
        };

        NumberArray<int32_t> currentMeasurementRaw();
        NumberArray<int32_t> currentMeasurementScaled();

        void measureOffset();
        bool measureScaling(uint8_t valueNumber, int32_t targetValue);
        void resetOffset();
        void resetScaling();

        /**
         * @brief Set the sample to integer conversion exponent.
         * 
         * This method shifts the decimal point of the sample values by the given exponent.
         * 
         * @param _sampleToIntExponent The exponent array to shift the decimal point of the sample values.
         */
        void setSampleToIntExponent(int8_t *_sampleToIntExponent) { 
            dataProcessing.sampleToIntExponent.loopArray([&](int8_t& value, uint8_t i) { value = _sampleToIntExponent[i]; } );
        };

    private:

        DataProcessing dataProcessing;
        DataCollection dataCollection = DataCollection(&cfgXmoduleSensor.sampleAveraging);

        millisDelay sensorDelay;

        // Flag to indicate new data is stored
        boolean newDataStored;

        // Offset and scaling
        boolean offsetRunning = false;
        boolean scalingRunning = false;
        int32_t scalingTargetValue;
        uint8_t scalingValueIndex;

        void measurementHandler(int32_t *newSample);

        void measureOffsetScalingFinish();
};

#endif
