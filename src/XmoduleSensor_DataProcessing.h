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

#include <ArduinoJson.h>

#include "Config.h"
#include "Helper.h"


struct DataProcessing : public CfgStructJsonInterface {
    DataProcessing() { cfgName = "cfgDataProcessing"; }

    // template <typename T>
    // struct ProcessingArray {
    //     uint8_t dataValueSize = 0;
    //     int8_t defaultValue;
    //     T *values;

    //     ProcessingArray() {}
    //     ProcessingArray(uint8_t _dataValueSize, int8_t _defaultValue) {
    //         dataValueSize = _dataValueSize;
    //         defaultValue = _defaultValue;

    //         delete [] values;
    //         values = new T[dataValueSize];
    //         reset();
    //     }

    //     void reset() {
    //         for (uint8_t i = 0; i < dataValueSize; i++)
    //             values[i] = static_cast<T>(defaultValue);
    //     }


    //     bool importFromJsonArray(JsonArray &jsonArray) {
    //         // Make sure size is correct to not have memory issues
    //         if (jsonArray.size() != dataValueSize)
    //             return false;
    //         // Assign values
    //         uint8_t i = 0;
    //         for (JsonVariant value : jsonArray)
    //             values[i++] = value.as<T>(); // .as<T>() not defined for some types?
    //         return true;
    //     }

    //     void set(T* _values) {
    //         for (uint8_t i = 0; i < dataValueSize; i++)
    //             values[i] = _values[i];
    //     }

    // };

    // Float conversion, int = float * 10^exponent
    // ProcessingArray<int8_t> sampleToIntExponent;
    // Offset, shift data in y direction
    // ProcessingArray<int32_t> offset;
    // // Scaling, strech data in y direction
    // ProcessingArray<float_t> scaling;

    Helper::NumberArray<int32_t> offset;
    Helper::NumberArray<float_t> scaling;
    Helper::NumberArray<int8_t> sampleToIntExponent;

    void initDataValueSize(uint8_t dataValueSize) {
        // sampleToIntExponent = ProcessingArray<int8_t>(dataValueSize, 0);
        // offset = ProcessingArray<int32_t>(dataValueSize, 0);
        // scaling = ProcessingArray<float_t>(dataValueSize, 1);

        offset.lateInit(dataValueSize, 0);
        scaling.lateInit(dataValueSize, 1);
        sampleToIntExponent.lateInit(dataValueSize, 0);
    }

    void exportToJson(JsonDocument &jsonDoc) {
        if (offset.isDefault()) {
            jsonDoc.remove("offset");
        } else {
            JsonArray jsonArray = jsonDoc.createNestedArray("offset");
            offset.loopArray([&](int32_t& value, uint8_t i) { jsonArray.add(value); });
        }
        if (scaling.isDefault()) {
            jsonDoc.remove("scaling");
        } else {
            JsonArray jsonArray = jsonDoc.createNestedArray("scaling");
            scaling.loopArray([&](float_t& value, uint8_t i) { jsonArray.add(value); });
        }
    }

    bool importFromJson(JsonDocument &jsonDoc) {
        JsonArray jsonArray;
        // Assigns values only if varName exists
        if (jsonDoc.containsKey("offset") && jsonDoc["offset"].is<JsonArray>()) {
            jsonArray = jsonDoc["offset"].as<JsonArray>();

            // Make sure size is correct to not have memory issues
            if (jsonArray.size() != offset.value_size)
                return false;
            
            // Assign values
            offset.loopArray([&](int32_t& value, uint8_t i) { value = jsonArray[i].as<int32_t>(); });
        }
        if (jsonDoc.containsKey("scaling") && jsonDoc["scaling"].is<JsonArray>()) {
            jsonArray = jsonDoc["scaling"].as<JsonArray>();

            // Make sure size is correct to not have memory issues
            if (jsonArray.size() != scaling.value_size)
                return false;

            // Assign values
            scaling.loopArray([&](float_t& value, uint8_t i) { value = jsonArray[i].as<float_t>(); });
        }
        return true;
    }

    void setOffset(int32_t* offsetMeasurement) {
        // OFFSET = -1 * sum/times
        offset.loopArray([&](int32_t& value, uint8_t i) { value = - offsetMeasurement[i]; });
    };

    int32_t scalingTargetValue = 0;
    uint8_t scalingTargetIndex = 0;

    void setScaling(int32_t* scalingMeasurement) {
        // SCALING = TARGETVALUE / (sum/times + OFFSET)
        scaling.loopArray([&](float_t& value, uint8_t i) {
            if (i == scalingTargetIndex) {
                value = (float_t)scalingTargetValue / (scalingMeasurement[i] + offset.values[i])  ;
            }
        });
    };
};
