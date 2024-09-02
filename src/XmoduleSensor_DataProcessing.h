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

#ifndef MVP3000_XMODULESENSOR_DATAPROCESSING
#define MVP3000_XMODULESENSOR_DATAPROCESSING

#include <Arduino.h>
#include <ArduinoJson.h>

#include "Config_JsonInterface.h"
#include "_Helper.h"


struct DataProcessing : public JsonInterface {

    // Data processing for sensor values:
    // Exponent is fixed in code, offset and scaling are stored, tare is forgotten after reboot
    // { [ ( raw * pow10(exponent) ) + offset ] * scaling } + tare

    NumberArrayLateInit<int32_t> offset;
    NumberArrayLateInit<int8_t> sampleToIntExponent;
    NumberArrayLateInit<float_t> scaling;
    NumberArrayLateInit<int32_t> tare;

    int32_t scalingTargetValue = 0;
    uint8_t scalingTargetIndex = 0;

    DataProcessing() : JsonInterface("cfgDataProcessing") { }

    void initDataValueSize(uint8_t dataValueSize) {
        sampleToIntExponent.lateInit(dataValueSize, 0);
        offset.lateInit(dataValueSize, 0);
        scaling.lateInit(dataValueSize, 1);
        tare.lateInit(dataValueSize, 0);
    }


//////////////////////////////////////////////////////////////////////////////////

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


//////////////////////////////////////////////////////////////////////////////////

    void setOffset(int32_t* offsetMeasurement) {
        // OFFSET = -1 * RAW
        offset.loopArray([&](int32_t& value, uint8_t i) { value = - offsetMeasurement[i]; });
    };

    void setSampleToIntExponent(int8_t *_sampleToIntExponent) { 
        sampleToIntExponent.loopArray([&](int8_t& value, uint8_t i) { value = _sampleToIntExponent[i]; } );
    };

    void setScaling(int32_t* scalingMeasurement) {
        // SCALING = TARGETVALUE / (RAW + OFFSET)
        scaling.loopArray([&](float_t& value, uint8_t i) {
            if (i == scalingTargetIndex) {
                value = (float_t)scalingTargetValue / (scalingMeasurement[i] + offset.values[i])  ;
            }
        });
    };

    void setScalingTarget(uint8_t valueIndex, int32_t targetValue) {
        scalingTargetIndex = valueIndex;
        scalingTargetValue = targetValue;
    };

    void setTare(int32_t* lastMeasurement) {
        // TARE = -1 * ( (lastRAW - OFFSET) * SCALING )
        tare.loopArray([&](int32_t& value, uint8_t i) { value = - ( (lastMeasurement[i] - offset.values[i]) * scaling.values[i] ); });
    };


//////////////////////////////////////////////////////////////////////////////////

    template <typename T>
    int32_t* applySampleToIntExponent(T *newSample) {
        int32_t* decimalShiftedSample = new int32_t[sampleToIntExponent.value_size];
        sampleToIntExponent.loopArray([&](int8_t& value, uint8_t i) {
            decimalShiftedSample[i] = nearbyintf( (float_t)pow10(value) * newSample[i] );
        });
        return decimalShiftedSample;
    };

    void applyProcessing(NumberArrayLateInit<int32_t> &values) {
        // Apply offset and scaling to array
        values.loopArray([&](int32_t& value, uint8_t i) {
            value = applyProcessing(value, i);
        });
    };

    int32_t applyProcessing(int32_t value, uint8_t i) {
        // Apply offset and scaling to single value
        // SCALED = (RAW + OFFSET) * SCALING + TARE
        return nearbyintf( ( (value + offset.values[i]) * scaling.values[i] ) + tare.values[i] );
    };

};

#endif