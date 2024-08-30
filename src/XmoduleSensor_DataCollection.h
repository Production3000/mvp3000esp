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

#ifndef MVP3000_XMODULESENSOR_DATACOLLECTION
#define MVP3000_XMODULESENSOR_DATACOLLECTION

#include "_LinkedList.h"
#include "XmoduleSensor_DataCollection_NumberArray.h"
#include "XmoduleSensor_DataProcessing.h"


struct DataCollection {

    /**
     * Data structure to store sensor data and its time.
     */
    struct DataStructSensor : NumberArray<int32_t> {
        uint64_t time;

        /**
         * @brief Constructor for data structure.
         * 
         * @param _time Time of data
         * @param _values Pointer to data array
         * @param _value_size Size of data array
         */
        DataStructSensor(uint64_t _time, int32_t* _values, uint8_t _value_size) : NumberArray<int32_t>(_values, _value_size), time(_time) { }
    };
    
    /**
     * Derived linked list to store sensor data and its time. Grows automatically.
     */
    struct LinkedListSensor : LinkedList3000<DataStructSensor> {
        LinkedListSensor(uint16_t _max_size, boolean _allow_growing) : LinkedList3000<DataStructSensor>(_max_size, _allow_growing) { }

        void append(uint64_t time, NumberArrayLateInit<int32_t> *data) {
            // Create data structure and add node to linked list
            // Using this-> as base class/function is templated
            this->appendNode(new DataStructSensor(time, data->values, data->value_size));
        }

        String getBookmarkAsCsv(uint8_t columnCount, DataProcessing *processing) { return nodeToCSV(bookmark, columnCount, processing); }
        String getLatestAsCsv(uint8_t columnCount, DataProcessing *processing) { return nodeToCSV(tail, columnCount, processing); }
        String getLatestAsCsvNoTime(uint8_t columnCount, DataProcessing *processing) { return nodeToCSV(tail, columnCount, processing, false); }

        String nodeToCSV(Node* node, uint8_t columnCount, DataProcessing *processing, boolean withTime = true) {
            // Return emty string if node is empty
            if (node == nullptr) {
                return "";
            }
            String str = (withTime) ? String(node->dataStruct->time) + ";" : "";
            for (uint8_t i = 0; i < node->dataStruct->value_size; i++) {
                str += (processing == nullptr) ? node->dataStruct->values[i] : processing->applyScaling(node->dataStruct->values[i], i);
                str += (i == node->dataStruct->value_size - 1) || ((i + 1) % columnCount == 0) ? ";" : ",";
            }
            return str;
        }

    };

    DataProcessing processing;

    // Storing of averages with initial limit of 100 is reasonable on ESP8266
    // The list grows automatically if memory is sufficient
    uint16_t dataStoreLength = 100;
    LinkedListSensor linkedListSensor = LinkedListSensor(dataStoreLength, true);

    // Averaging
    NumberArrayLateInit<int32_t> avgDataSum; // Temporary data storage for averaging
    uint16_t *averagingCountPtr; // Pointer to cfgXmoduleSensor
    uint8_t avgCounter = 0; // Counter for averaging
    int64_t avgStartTime = 0; // Time of first measurement in averaging cycle
    boolean avgCycleFinished = false; // Flag for new data added to dataStore

    // Data statistics
    NumberArrayLateInit<int32_t> dataMax;
    NumberArrayLateInit<int32_t> dataMin;


    DataCollection(uint16_t *averagingCount) : averagingCountPtr(averagingCount) { };

    void initDataValueSize(uint8_t dataValueSize) {
        processing.initDataValueSize(dataValueSize);
        // Init all NumberArrayLateInits
        avgDataSum.lateInit(dataValueSize, 0);
        dataMax.lateInit(dataValueSize, std::numeric_limits<int32_t>::min());
        dataMin.lateInit(dataValueSize, std::numeric_limits<int32_t>::max());
    }


//////////////////////////////////////////////////////////////////////////////////

    // Called only when switching from normal measurement to offset/scaling measurement
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
        linkedListSensor.clear();
    }

    template <typename T>
    void addSampleNEW(T *newSample)  {
        int32_t* decimalShiftedSample = processing.applySampleToIntExponent(newSample);
        addSample(decimalShiftedSample);
        delete[] decimalShiftedSample; // IMPORTANT
    };

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

            // Calculate data averages
            avgDataSum.loopArray([&](int32_t& value, uint8_t i) { value = nearbyintf( value / *averagingCountPtr ); } );
            // Store median time and data in linked list
            linkedListSensor.append((uint64_t)nearbyintf( (avgStartTime + millis()) / 2 ), &avgDataSum);

            // Reset temporary values, counters            
            avgDataSum.resetValues();
            avgCounter = 0;
            avgStartTime = 0;
            // Flag new data added for further actions in loop()
            avgCycleFinished = true;
        }
    }
};

#endif