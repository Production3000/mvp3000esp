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

#include "Config_JsonInterface.h"
#include "Helper.h"


struct DataProcessing : public JsonInterface {
    DataProcessing() : JsonInterface("cfgDataProcessing") { }

    NumberArray<int32_t> offset;
    NumberArray<float_t> scaling;
    NumberArray<int8_t> sampleToIntExponent;

    int32_t scalingTargetValue = 0;
    uint8_t scalingTargetIndex = 0;

    void initDataValueSize(uint8_t dataValueSize) {
        offset.lateInit(dataValueSize, 0);
        scaling.lateInit(dataValueSize, 1);
        sampleToIntExponent.lateInit(dataValueSize, 0);
    }

    void exportToJson(JsonDocument &jsonDoc) {
        // No need to save if values are default
        if (!offset.isDefault()) {
            JsonArray jsonArray = jsonDoc.createNestedArray("offset");
            offset.loopArray([&](int32_t& value, uint8_t i) { jsonArray.add(value); });
        }
        if (!scaling.isDefault()) {
            JsonArray jsonArray = jsonDoc.createNestedArray("scaling");
            scaling.loopArray([&](float_t& value, uint8_t i) { jsonArray.add(value); });
        }
    }

    bool importFromJson(JsonDocument &jsonDoc) {
        // Assigns values only if varName exists
        if (jsonDoc.containsKey("offset") && jsonDoc["offset"].is<JsonArray>()) {
            JsonArray jsonArray = jsonDoc["offset"].as<JsonArray>();

            // Make sure size is correct to not have memory issues
            if (jsonArray.size() != offset.value_size)
                return false;
            
            // Assign values
            offset.loopArray([&](int32_t& value, uint8_t i) { value = jsonArray[i].as<int32_t>(); });
        }
        if (jsonDoc.containsKey("scaling") && jsonDoc["scaling"].is<JsonArray>()) {
            JsonArray jsonArray = jsonDoc["scaling"].as<JsonArray>();

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

    void setScaling(int32_t* scalingMeasurement) {
        // SCALING = TARGETVALUE / (sum/times + OFFSET)
        scaling.loopArray([&](float_t& value, uint8_t i) {
            if (i == scalingTargetIndex) {
                value = (float_t)scalingTargetValue / (scalingMeasurement[i] + offset.values[i])  ;
            }
        });
    };
};
