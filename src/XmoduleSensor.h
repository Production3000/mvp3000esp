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
            dataCollection.initDataValueSize(valueCount); // Averaging can change during operation
        };


        void setup() override;
        void loop() override;

        /**
         * @brief Add a new sample array to the sensor module.
         * 
         * @tparam T The numeric type of the sample, typically int or float.
         * @param newSample The new sample array to add.
         */
        template <typename T>
        void addSample(T *newSample)  {
            // This just adds the sample to the data collection for averaging, once count is done further work is done in the Loop()
            dataCollection.addSampleNEW(newSample);
        };

        /**
         * @brief Shift the decimal point of the sample values by the given exponent.
         * 
         * @param _sampleToIntExponent The exponent array to shift the decimal point of the sample values.
         */
        void setSampleToIntExponent(int8_t *sampleToIntExponent) {
            dataCollection.processing.setSampleToIntExponent(sampleToIntExponent);
        };

        void measureOffset();
        bool measureScaling(uint8_t valueNumber, int32_t targetValue);
        void resetOffset();
        void resetScaling();
        void clearTare();
        void setTare();

    private:

        DataCollection dataCollection = DataCollection(&cfgXmoduleSensor.sampleAveraging);

        millisDelay sensorDelay;

        // Offset and scaling
        boolean offsetRunning = false;
        boolean scalingRunning = false;
        int32_t scalingTargetValue;
        uint8_t scalingValueIndex;

        void measureOffsetScalingFinish();


        void webSocketCallback(char* data); // Callback for to receive data from the websocket
        std::function<void(const String &message)> webSocketPrint; // Function to print to the websocket


        String webPageProcessor(const String& var);
        size_t webPageCsvResponseFiller(uint8_t* buffer, size_t maxLen, size_t index, boolean firstOnly, std::function<String()> stringFunc);
        char* webPage = R"===(
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
    <li>Sample averaging:<br> <form action='/save' method='post'> <input name='sampleAveraging' value='%11%' type='number' min='1' max='255'> <input type='submit' value='Save'> </form> </li>
    <li>Averaging of offset and scaling measurements:<br> <form action='/save' method='post'> <input name='averagingOffsetScaling' value='%12%' type='number' min='1' max='255'> <input type='submit' value='Save'> </form> </li>
    <li>Reporting minimum interval for fast sensors, 0 to ignore:<br> <form action='/save' method='post'> <input name='reportingInterval' value='%13%' type='number' min='0' max='65535'> [ms] <input type='submit' value='Save'> </form> </li> </ul>
<h3>Data Interface</h3> <ul>
    <li>Data storage: %14%</li>
    <li>Current data: <a href='/sensordata'>/sensordata</a> </li>
    <li>Live websocket: ws://%15%/wssensor </li>
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
)===";

};

#endif
