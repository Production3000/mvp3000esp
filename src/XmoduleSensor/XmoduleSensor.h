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

#include "_Xmodule.h"
#include "_Helper_LimitTimer.h"

#include "XmoduleSensor_DataCollection.h"
#include "XmoduleSensor_webpage.h"


typedef std::function<void(const String&)> NetworkCtrlCallback;


struct CfgXmoduleSensor : CfgJsonInterface {

    enum OutputTarget: uint8_t {
        CONSOLE = 0,
        WEBSOCKET = 1,
        MQTT = 2,
    };
    _Helper::MultiBoolSettings<uint8_t> outputTargets = _Helper::MultiBoolSettings<uint8_t>(); // Default is all enabled

    // Modifiable settings saved to SPIFF

    uint8_t avgCountSample = 10; // At least 1
    uint8_t avgCountOffsetScaling = 25; // At least 1
    uint16_t reportingInterval = 0; // [ms], 0 to ignore
    uint8_t thresholdPermilleChange = 0; // 0 to disable
    int16_t thresholdOnlySingleIndex = -1; // Max 255, -1 to apply to all values

    CfgXmoduleSensor() : CfgJsonInterface("cfgXmoduleSensor") {
        addSetting<uint8_t>("avgCountSample", &avgCountSample, [&](const String& s) { uint8_t n = s.toInt(); if (n == 0) return false; avgCountSample = n; return true; } );
        addSetting<uint8_t>("avgCountOffsetScaling", &avgCountOffsetScaling, [&](const String& s) { uint8_t n = s.toInt(); if (n == 0) return false; avgCountOffsetScaling = n; return true; } );
        addSetting<uint16_t>("reportingInterval", &reportingInterval, [&](const String& s) { reportingInterval = s.toInt(); return true; } );
        addSetting<uint8_t>("thresholdPermilleChange", &thresholdPermilleChange, [&](const String& s) { thresholdPermilleChange = s.toInt(); return true; } );
        addSetting<int16_t>("thresholdOnlySingleIndex", &thresholdOnlySingleIndex, [&](const String& s) { int16_t n = s.toInt(); if ((n < -1) || (n > 255)) return false; thresholdOnlySingleIndex = n; return true; } );
    };

    // Settings that are not known during creation of this config within the framework but need init before anything works
    // Fixed settings, restored with reboot to value set at compile

    // Number of values recorded per measurement, for example one each for temperature, humidity, pressure -> 3
    uint8_t dataValueCount = 0;

    const char* infoName;
    const char* infoDescription;
    const char** sensorTypes;
    const char** sensorUnits;

    // Used for output only, matrix data with a row length, if matrixColumnCount >= dataValueCount it is obviously a single row
    uint8_t matrixColumnCount = 255;

    void initValueCount(uint8_t _dataValueCount) {
        dataValueCount = _dataValueCount;

        delete [] sensorTypes;
        sensorTypes = new const char*[dataValueCount];
        delete [] sensorUnits;
        sensorUnits = new const char*[dataValueCount];
    }

    void setSensorInfo(const String& _infoName, const String& _infoDescription, String* _sensorTypes, String* _sensorUnits) {
        infoName = _infoName.c_str();
        infoDescription = _infoDescription.c_str();
        for (u_int8_t i = 0; i < dataValueCount; i++) {
            sensorTypes[i] = _sensorTypes[i].c_str();
            sensorUnits[i] = _sensorUnits[i].c_str();
        }
    }

    void setSensorInfo(const String& _infoName, const String& _infoDescription, const String& _pixelType, const String& _pixelUnit, uint8_t _matrixColumnCount) {
        infoName = _infoName.c_str();
        infoDescription = _infoDescription.c_str();
        for (u_int8_t i = 0; i < dataValueCount; i++) {
            sensorTypes[i] = _pixelType.c_str();
            sensorUnits[i] = _pixelUnit.c_str();
        }
        matrixColumnCount = _matrixColumnCount;
    }
};


//////////////////////////////////////////////////////////////////////////////////

class XmoduleSensor : public _Xmodule {

    public:

        /**
         * @brief Construct a new Sensor Module object.
         *
         * @param valueCount The number of values simultaneously coming from the sensor(s).
         */
        XmoduleSensor(uint8_t valueCount) : _Xmodule("Sensor Module", "/sensor") {
            uriWebSocket = "/wssensor";
            mqttTopic = "sensor";
            cfgXmoduleSensor.initValueCount(valueCount);
            dataCollection.initDataValueSize(valueCount); // Averaging can change during operation
        };


        /**
         * @brief Add new data to the sensor module.
         *
         * @tparam T The numeric type of the sample, typically int or float.
         * @param newSample The new sample array to add.
         */
        template <typename T>
        void addSample(T *newSample)  {
            // This just adds the sample to the data collection for averaging, once count is done further work is done in the Loop()
            dataCollection.addSample(newSample);
        };


        /**
         * @brief Disable data output serial. This does not affect general logging to serial.
         */
        void disableDataToSerial() { cfgXmoduleSensor.outputTargets.change(CfgXmoduleSensor::OutputTarget::CONSOLE, false); };

        /**
         * @brief Disable communication and data output via MQTT.
         */
        void disableMqtt() { cfgXmoduleSensor.outputTargets.change(CfgXmoduleSensor::OutputTarget::MQTT, false); };

        /**
         * @brief Disable communication and data output via WebSocket.
         */
        void disableWebSocket() { cfgXmoduleSensor.outputTargets.change(CfgXmoduleSensor::OutputTarget::WEBSOCKET, false); };


        /**
         * @brief Set data collection to adaptive mode, growing depending on available memory.
         * 
         * The number of measurements stored is limited by the available heap memory on the ESP. Data collection can be set
         * to adaptive mode, growing depending on available memory. This feature could mask memory leaks in other parts of
         * the program and thus lead to stability issues, particularly with fragmented memory.
         */
        void setDataCollectionAdaptive() {
            dataCollection.linkedListSensor.enableAdaptiveGrowing();
        };

        /**
         * @brief Set the wait time between data reports.
         * 
         * @param reportingInterval The interval in milliseconds, 0 to report once new data is available.
         */
        void setReportingInterval(uint16_t reportingInterval) { cfgXmoduleSensor.reportingInterval = reportingInterval; };

        /**
         * @brief Set the reporting threshold in permille change.
         * 
         * @param thresholdPermilleChange The threshold in permille change.
         * @param thresholdOnlySingleIndex (optional) The index of the value to apply the threshold to, -1 for all values (default).
         */
        void setReportingThreshold(uint8_t thresholdPermilleChange, int16_t thresholdOnlySingleIndex = -1) {
            cfgXmoduleSensor.thresholdPermilleChange = thresholdPermilleChange;
            cfgXmoduleSensor.updateSingleValue("thresholdOnlySingleIndex", String(thresholdOnlySingleIndex)); // Use set function as datatype is different to allow -1
        };

        /**
         * @brief Set initial sample averaging count after first compile. This value is superseeded by the user-set/saved value in the web interface.
         * 
         * @param avgCountSample The number of samples to average before reporting.
         */
        void setSampleAveraging(uint8_t avgCountSample) { cfgXmoduleSensor.avgCountSample = avgCountSample; };

        /**
         * @brief Shift the decimal point of the sample values by the given exponent.
         *
         * @param _sampleToIntExponent The exponent array to shift the decimal point of the sample values.
         */
        void setSampleToIntExponent(int8_t *sampleToIntExponent) {
            dataCollection.processing.setSampleToIntExponent(sampleToIntExponent);
        };


        /**
         * @brief Set the sensor information.
         *
         * @param infoName The name of the sensor.
         * @param infoDescription The description of the sensor.
         * @param sensorTypes The types of the sensor values as array.
         * @param sensorUnits The units of the sensor values as array.
         */
        void setSensorInfo(const String& infoName, const String& infoDescription, String* sensorTypes, String* sensorUnits) {
            cfgXmoduleSensor.setSensorInfo(infoName, infoDescription, sensorTypes, sensorUnits);
        };

        /**
         * @brief Set the sensor information for a matrix sensor.
         *
         * @param infoName The name of the sensor.
         * @param infoDescription The description of the sensor.
         * @param pixelType The type of the pixel value.
         * @param pixelUnit The unit of the pixel value.
         * @param matrixColumnCount The number of columns in the data matrix.
         */
        void setSensorInfo(const String& infoName, const String& infoDescription, const String& pixelType, const String& pixelUnit, uint8_t matrixColumnCount) {
            cfgXmoduleSensor.setSensorInfo(infoName, infoDescription, pixelType, pixelUnit, matrixColumnCount);
        };

        /**
         * @brief Set a custom network control callback function to receive control commands from MQTT and WebSocket
         *
         * @param callback The callback function.
         */
        void setNetworkCtrlCallback(NetworkCtrlCallback callback) { networkCtrlCbUserScript = callback; };

    public:

        CfgXmoduleSensor cfgXmoduleSensor;

        void setup() override;
        void loop() override;

        void measureOffset();
        bool measureScaling(uint8_t valueNumber, int32_t targetValue);
        void resetOffset();
        void resetScaling();
        void clearTare();
        void setTare();

    private:

        DataCollection dataCollection = DataCollection(&cfgXmoduleSensor.avgCountSample);

        String uriWebSocket;
        String mqttTopic;

        LimitTimer reportingTimer = LimitTimer(0);

        // Offset and scaling
        boolean offsetRunning = false;
        boolean scalingRunning = false;
        int32_t scalingTargetValue;
        uint8_t scalingValueIndex;

        void measureOffsetScalingFinish();

        void networkCtrlCallback(const String& data); // Callback to receive control commands from MQTT and WebSocket
        NetworkCtrlCallback networkCtrlCbUserScript = nullptr;

        String webPageProcessor(uint8_t var);
        uint8_t webPageProcessorCount;

        size_t csvRawResponseFiller(uint8_t* buffer, size_t maxLen, size_t index);
        size_t csvLatestResponseFiller(uint8_t* buffer, size_t maxLen, size_t index);
        size_t csvScaledResponseFiller(uint8_t* buffer, size_t maxLen, size_t index);
        size_t csvExtendedResponseFiller(uint8_t* buffer, size_t maxLen, size_t index, boolean firstOnly, std::function<String()> stringFunc);

        PGM_P getWebPage() override { return htmlXmoduleSensor; }
};

#endif
