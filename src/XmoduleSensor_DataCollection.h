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

#include "Helper.h"


struct DataCollection {

    DataCollection(uint16_t *averagingCount) : averagingCountPtr(averagingCount) { };


    /**
     * Derived linked list to store sensor data and its time.
     */
    struct DataStructSensor {
        uint32_t time;
        int32_t* data;

        ~DataStructSensor() {
            delete[] data; // IMPORTANT: Make sure to also free memory within the dataStruct
        }
        void lateInit(uint8_t size) {
            data = new int32_t[size];
        }
    };

    struct LinkedListSensor : LinkedList<DataStructSensor> {
        LinkedListSensor(uint8_t _max_size) : LinkedList<DataStructSensor>(_max_size) { }

        void append(int32_t* data, uint8_t size, uint32_t time) {
            // Add node to list and assign the data to its datastruct
            // Using this-> as base class/function is templated
            this->appendNode();
            this->getNewestData()->lateInit(size);
            for (uint8_t i = 0; i < size; i++) {
                this->getNewestData()->data[i] = data[i];
            }
            this->getNewestData()->time = time;
        }
    };


    // Storing of averages, empiric maximum length of circular data buffer on ESP8266: 1x float 5000, 2x float 3500
    // More likely much less, max 1000 for single value?
    uint16_t dataStoreLength = 5;
    LinkedListSensor dataStoreSensor = LinkedListSensor(dataStoreLength);

    // Averaging
    NumberArray<int32_t> avgDataSum; // Temporary data storage for averaging
    uint16_t *averagingCountPtr; // Pointer to cfgXmoduleSensor
    uint8_t avgCounter = 0; // Counter for averaging
    int32_t avgStartTime = 0; // Time of first measurement in averaging cycle
    boolean avgCycleFinished = false; // Flag for new data added to dataStore

    // Data statistics
    NumberArray<int32_t> dataMax;
    NumberArray<int32_t> dataMin;


    void initDataValueSize(uint8_t dataValueSize) {
        // Init all NumberArrays
        avgDataSum.lateInit(dataValueSize, 0);
        dataMax.lateInit(dataValueSize, std::numeric_limits<int32_t>::min());
        dataMin.lateInit(dataValueSize, std::numeric_limits<int32_t>::max());
    }

    void setAveragingCountPtr(uint16_t *_averagingCount) {
        averagingCountPtr = _averagingCount;
        reset();
    }

    void reset() {
        // Averaging
        avgDataSum.resetValues();
        dataMax.resetValues();
        dataMin.resetValues();

        // Counters and such
        avgCounter = 0;
        avgStartTime = 0;
        avgCycleFinished = false;

        // Data storage
        dataStoreSensor.clear();
    }

    void addSample(int32_t *newSample) {
        // This is the function to do most of the work

        // Add new values to existing sums, remember max/min extremes
        avgDataSum.loopArray([&](int32_t& value, uint8_t i) { value += newSample[i]; } ); // Add new value for later averaging
        dataMax.loopArray([&](int32_t& value, uint8_t i) { value = max(value, newSample[i]); } ); // All-time max
        dataMin.loopArray([&](int32_t& value, uint8_t i) { value = min(value, newSample[i]); } ); // All-time min

        // Averaging cycle restarted, init
        if (avgCounter == 0) {
            avgStartTime =  millis();
            avgCycleFinished = false;
        }
        // Increment averaging head
        avgCounter++;

        // Check if averaging count is reached
        if (avgCounter >= *averagingCountPtr) {

            // Calculate data and time averages and store
            avgDataSum.loopArray([&](int32_t& value, uint8_t i) { value = nearbyintf( value / *averagingCountPtr ); } );
            dataStoreSensor.append(avgDataSum.values, avgDataSum.value_size, nearbyintf( (avgStartTime + millis()) / 2 ));

            // Reset temporary values, counters            
            avgDataSum.resetValues();
            avgCounter = 0;
            avgStartTime = 0;
            // Flag new data added for further actions in loop()
            avgCycleFinished = true;
        }
    }
};
