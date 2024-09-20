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

#ifndef MVP3000_XMODULESENSOR_DATACOLLECTION_NUMBERARRAY
#define MVP3000_XMODULESENSOR_DATACOLLECTION_NUMBERARRAY

#include <Arduino.h>


template <typename T>
struct NumberArray {

    T* values;
    uint8_t value_size;

    NumberArray(T* _values, uint8_t _value_size) : value_size(_value_size) {
        values = new T[value_size];
        for (uint8_t i = 0; i < value_size; i++) {
            values[i] = _values[i];
        }
    }

    ~NumberArray() {
        delete[] values;
    }
};



/**
 * A simple number array struct for the MVP3000 framework.
 *
 * Its provides looping functionality, clear/default value function, and a check if it is not all default values.
 * It can be intialized late with a different size and reset value.
 *
 * @tparam T The type of data to be stored in the array.
 */
template <typename T>
struct NumberArrayLateInit : NumberArray<T> {

    T defaultValue;

    // Dummy constructor called during creation, initialization is done late
    NumberArrayLateInit() : NumberArray<T>(0, 0) { }

    /**
     * (Re-)initializes the array with the given size and reset value during runtime.
     *
     * @param _valueSize The size of the array.
     * @param _defaultValue The value to be used to initialize the values in the array.
     */
    void lateInit(uint8_t _valueSize, T _defaultValue) {
        this->value_size = _valueSize;
        defaultValue = _defaultValue;
        delete [] this->values;
        this->values = new T[this->value_size];
        resetValues();
    }


    /**
     * Initializes the value in the array with the reset value.
     */
    void resetValues() {
        this->loopArray([this](T& value, uint8_t i) {
            this->values[i] = defaultValue;
        });
    }

    /**
     * Loops through all elements in the linked list starting from oldest and calls the given callback function.
     * The callback function can be a captive lambda function.
     *
     * @param callback The callback function to be called for each element.
     */
    void loopArray(std::function<void(T&, uint8_t)> callback) {
        for (uint8_t i = 0; i < this->value_size; i++) {
            callback(this->values[i], i);
        }
    }

    /**
     * Checks if the array only contains the default reset value.
     */
    bool isDefault() {
        bool isDefault = true;
        this->loopArray([&](T& value, uint8_t i) {
            if (value != defaultValue) {
                isDefault = false;
            }
        });
        return isDefault;
    }
};


#endif