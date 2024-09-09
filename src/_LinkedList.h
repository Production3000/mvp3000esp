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

#ifndef MVP3000__LINKEDLIST
#define MVP3000__LINKEDLIST

#include <Arduino.h>

#include "ESPX.h"
#ifdef ESP8266
    extern EspClass ESPX;
#else
    extern EspClassX ESPX;
#endif

/**
 * A templated linked list implementation for the MVP3000 framework.
 * 
 * It can be extended to store any type of pre-defined data structure encapsulating e.g. a single value, an array, or a struct.
 * The list has a maximum size limit set during initialization. If the limit is reached, the oldest element is automatically removed.
 * 
 * Example usages are below: 1) Single values, 2) Arrays of values.
 * 
 * @tparam T The pre-defined data structure to be stored in the linked list.
 */
template <typename T>
struct LinkedList3000 {
    // The name LinkedList is generic and is taken by other libraries, e.g. ESPAsyncWebServer 

    struct Node {
        T* dataStruct;
        Node* prev; // towards head
        Node* next; // towards tail

        Node(T* newDataStruct) {
            dataStruct = newDataStruct;
        }
        ~Node() { // IMPORTANT: Make sure to also free memory within the dataStruct
            delete dataStruct;
        }
    };

    Node* head = nullptr; // head is the oldest
    Node* tail = nullptr; // tail is the newest
    Node* bookmark = nullptr; // bookmark is a temporary pointer, e.g. for slow looping

    T* getNewestData() { return tail->dataStruct; }
    T* getOldestData() { return head->dataStruct; }

    uint16_t size;
    uint16_t max_size;
    boolean allow_growing;

    uint16_t getSize() const { return size; }
    uint16_t getMaxSize() const { return max_size; }
    boolean getAdaptiveSize() const { return allow_growing; }


    /**
     * Constructor with a maximum list size limit.
     * 
     * @param _max_size The maximum size of the linked list.
     * @param _allow_growing If true, the list can grow beyond the maximum size limit depending on available memory. Default is false.
     */
    LinkedList3000(uint16_t _max_size, boolean _allow_growing = false) : max_size(_max_size), allow_growing(_allow_growing) { }

    ~LinkedList3000() {
        clear(); // IMPORTANT: Make sure to also free memory within the dataStruct
    }


    /**
     * @brief Appends an empty node to the linked list. Removes the oldest/head node if the list is already full.
     * 
     * The class is templated, thus use this->appendNode() in derived classes to add a new node.
     * 
     * @param newDataStruct The data structure to be passed on to be stored in the linked list.
     */
    void appendNode(T* newDataStruct) {
        // Check if size limit is reached and cannot be grown, then remove the oldest node
        if ((size >= max_size) && !growMaxSize()) {
            removeNode(head);
        }

        Node* newNode = new Node(newDataStruct);
        newNode->prev = nullptr;
        newNode->next = nullptr;

        if (head == nullptr) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }

        size++;
    }

    /**
     * @brief Move element to the tail/top of the list if it exists, otherwise appends it to the list
     * 
     * The class is templated, thus use this->appendNode() in derived classes to add a new node.
     *
     * @param newDataStruct The data structure to be moved to tail or appended to the linked list.
     */
    void appendNodeOrMoveToTail(T* newDataStruct) {
        Node* existingNode = containsNode(newDataStruct);
        if (existingNode == nullptr) {
            appendNode(newDataStruct);
        } else {
            delete newDataStruct; // IMPORTANT: Delete object to not leak memory
            moveNodeToTail(existingNode);
        }
    }

    /**
     * @brief Gets the node at the given index and bookmarks it.
     * 
     * @param index The index of the node to be retrieved, starting from zero.
     * @param reverse If true, the index is counted from latest/tail to first/head entry. Default is false.
     */
    void setBookmark(uint16_t index, boolean reverse = false, boolean noNull = false) {
        if (index >= size) {
            if (noNull) { // Return the tail/head if index is out of bounds
                bookmark = (reverse) ? head : tail;
            } else { // Return nullptr if index is out of bounds
                bookmark = nullptr;
            }
        } else { // Find the node at the given index, start from head/tail
            bookmark = (reverse) ? tail : head;
            for (uint16_t i = 0; i < index; i++) {
                bookmark = (reverse) ? bookmark->prev : bookmark->next;
            }
        }
    }
    bool moveBookmark() { if(bookmark == nullptr) return false; bookmark = bookmark->next; if(bookmark == nullptr) return false; return true; }

    /**
     * @brief Grows the maximum size of the linked list if enough memory is available.
     * 
     * @return True if the maximum size was increased, otherwise false.
     */
    bool growMaxSize() {
        // This is quite complex...
        // 
        if ((allow_growing) && (ESP.getFreeHeap() > 16384) && (ESPX.getHeapFragmentation() < 50)){
            // 16k free memory seems reasonable for single core ESP8266
            // However, ESP32 has two cores, with seperate heaps ....
            max_size += 5;
            return true;
        }
        return false;
    }

    /**
     * @brief Clears the linked list.
     */
    void clear() {
        while (head != nullptr) {
            removeNode(head);
        }
    }

    /**
     * Checks if the linked list contains the given value.
     * 
     * @param value The value to be checked.
     */
    Node* containsNode(T* newdataStruct) {
        Node* current = head;
        while (current != nullptr) {
            if (current->dataStruct->equals(newdataStruct)) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }
    bool containsNode(T newdataStruct) {
        return containsNode(&newdataStruct) != nullptr;
    }

    /**
     * Searches the nodes for given content and removes the node.
     * 
     * @param dataStruct The data structure to be removed.
     */
    void findRemoveNode(T dataStruct) {
        removeNode(containsNode(&dataStruct));
    }

    /**
     * @brief Loops through all elements in the linked list and calls the given callback function.
     * 
     * The callback function can be a captive lambda function, with its data structure and index as parameters:
     *      loopNodes([&](T*& dataStruct, uint16_t i) { ... });
     * The data structure is passed by reference, so it can be modified within the lambda function.
     * 
     * @param callback The callback function to be called for each node.
     * @param reverse If true, the list is looped from latest/tail to first/head entry. Default is false.
     */       
    void loopNodes(std::function<void(T*&, uint16_t)> callback, boolean reverse = false) {
        // This one only allows non-captive lambdas: loopNodes(void (*callback)(T"&, uint16_t))
        Node* current = (reverse) ? tail : head;
        uint16_t i = 0;
        while (current != nullptr) {
            callback(current->dataStruct, i++);
            current = (reverse) ? current->prev : current->next;
        }
    }

    /**
     * Moves a given node to the tail of the list.
     * 
     * @param existingNode The node to be moved.
     */
    void moveNodeToTail(Node* existingNode) {
        // The node is already the last node
        if (existingNode == tail) {
            return;
        } 
    
        // Remove the node from its current position
        if (existingNode == head) {
            head = existingNode->next; // Move the head pointer to the second node
            head->prev = nullptr; // Remove the prev pointer from the new head
        } else {
            // Neither head nor tail
            existingNode->prev->next = existingNode->next; // Link the previous node directly to the next node
            existingNode->next->prev = existingNode->prev; // Link the next node directly to the previous node
        }
        // Insert the node at the end
        tail->next = existingNode; // Set the next pointer of the old tail to the node
        existingNode->prev = tail; // Set the previous pointer of the node to the old tail
        existingNode->next = nullptr; // Remove the next pointer of the node
        tail = existingNode; // Move the tail pointer to the node
    }

    /**
     * Removes a given node from the list.
     * 
     * @param existingNode The node to be removed.
     */
    void removeNode(Node* existingNode) {
        if (existingNode == nullptr) {
            return;
        }

        if (existingNode == head) {
            head = existingNode->next; // Move the head pointer to the second node
            if (head != nullptr) {
                head->prev = nullptr; // Remove the prev pointer from the new head
            } else { // If no node remains, the tail needs to be set to nullptr as well
                tail = nullptr;
            }
        } else if (existingNode == tail) {
            tail = existingNode->prev; // Move the tail pointer to the second last node
            tail->next = nullptr; // Remove the next pointer from the new tail
        } else {
            existingNode->prev->next = existingNode->next; // Link the previous node directly to the next node
            existingNode->next->prev = existingNode->prev; // Link the next node directly to the previous node
        }

        delete existingNode; // IMPORTANT: Make sure to also free memory within the dataStruct
        size--;
    }
};



/**
 * A templated linked list implementation to hold a single value.
 * 
 * It is a derived class from the LinkedList3000 class and uses a data structure to store single values.
 * 
 * @tparam T The type of data to be stored, e.g. int, float.
 */
template <typename T>
struct DataStructValue {
    T data;

    DataStructValue(T _data) : data(_data) { }
    // ~DataStructValue() { } // No need to free memory for single values

    /**
     * @brief Compares the data of two data structures.
     * 
     * @param newDataStruct The data structure to be compared.
     * @return True if the data is equal, otherwise false.
     */
    bool equals(DataStructValue<T>* newDataStruct) {
        return data == newDataStruct->data;
    }
};

template <typename T>
struct LinkedListValue : LinkedList3000<DataStructValue<T>> {
    LinkedListValue(uint16_t _max_size, boolean _allow_growing = false) : LinkedList3000<DataStructValue<T>>(_max_size, _allow_growing) { }

    /**
     * @brief Appends a single value to the linked list.
     * 
     * @param data The value to be appended.
     */
    void appendData(T data) {
        // Create data structure and add node to linked list
        // Using this-> as base class/function is templated
        this->appendNode(new DataStructValue<T>(data));
    }

    /**
     * @brief Appends a single value to the linked list or moves it to the top if it already exists.
     * 
     * @param data The value to be appended or moved to the top.
     */
    void appendDataOrMoveToTop(T data) {
        // IMPORTANT: the dataStruct is deleted in the function if not appended
        this->appendNodeOrMoveToTail(new DataStructValue<T>(data)); 
    }

    bool containsData(T data) {
        return this->containsNode(DataStructValue<T>(data));
    }

    /**
     * @brief Loops through all elements in the linked list and calls the given callback function with the VALUE and its index.
     * 
     * @param callback The callback function to be called for each value.
     */
    void loopDatas(std::function<void(T, uint16_t)> callback, bool reverse = false) {
        this->loopNodes([&](DataStructValue<T>*& dataStruct, uint16_t i) { callback(dataStruct->data, i); }, reverse);
    }

    /**
     * @brief Removes a single value from the linked list.
     * 
     * @param data The value to be removed.
     */
    void removeData(T data) {
        this->findRemoveNode(DataStructValue<T>(data));
    }
};



/**
 * A templated linked list implementation to hold an array of values.
 * 
 * It is a derived class from the LinkedList3000 class and uses a data structure to store arrays of values.
 * 
 * @tparam T The type of array-data to be stored, e.g. int, char.
 */
template <typename T>
struct DataStructArray {
    T* data;
    uint8_t size;

    DataStructArray(T* _data, uint8_t _size) : size(_size) {
        data = new T[size];
        for (uint8_t i = 0; i < size; i++) {
            data[i] = _data[i];
        }
    }
    ~DataStructArray() {
        delete[] data; // IMPORTANT: Make sure to also free memory within the dataStruct
    }

    /**
     * @brief Compares the data of two data structures.
     * 
     * @param newDataStruct The data structure to be compared.
     * @return True if the data is equal, otherwise false.
     */
    bool equals(DataStructArray<T>* newDataStruct) {
        if (size != newDataStruct->size) {
            return false;
        }
        for (uint8_t i = 0; i < size; i++) {
            if (data[i] != newDataStruct->data[i]) {
                return false;
            }
        }
        return true;
    }
};

template <typename T>
struct LinkedListArray : LinkedList3000<DataStructArray<T>> {
    LinkedListArray(uint16_t _max_size, boolean _allow_growing = false) : LinkedList3000<DataStructArray<T>>(_max_size, _allow_growing) { }

    /**
     * @brief Appends an array of values to the linked list.
     * 
     * @param data The array of values to be appended.
     * @param size The size of the array.
     */
    void appendData(T* data, uint8_t size) {
        // Create data structure and add node to linked list
        // Using this-> as base class/function is templated
        this->appendNode(new DataStructArray<T>(data, size));
    }
};

#endif
