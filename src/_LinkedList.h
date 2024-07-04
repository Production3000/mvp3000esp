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

/**
 * A templated linked list implementation for the MVP3000 framework.
 * 
 * It can be extended to store any type of pre-defined data structure encapsulating e.g. a single value, an array, or a struct.
 * The list has a maximum size limit set during initialization. If the limit is reached, the oldest element is automatically removed.
 * Example usages are below.
 * 
 * @tparam T The pre-defined data structure to be stored in the linked list.
 */
template <typename T>
struct LinkedList {

    struct Node {
        T* dataStruct;
        Node* prev;
        Node* next;

        Node(T* newDataStruct) {
            dataStruct = newDataStruct;
        }
        ~Node() { // IMPORTANT: Make sure to also free memory within the dataStruct
            delete dataStruct;
        }
    };

    Node* head = nullptr; // head is the oldest
    Node* tail = nullptr; // tail is the newest

    T* getOldestData() { return head->dataStruct; }
    T* getNewestData() { return tail->dataStruct; }

    uint16_t size;
    uint16_t max_size;
    boolean allow_growing;

    uint8_t getSize() const { return size; }


    /**
     * Constructor with a maximum list size limit.
     * 
     * @param _max_size The maximum size of the linked list.
     * @param _allow_growing If true, the list can grow beyond the maximum size limit depending on available memory. Default is false.
     */
    LinkedList(uint16_t _max_size, boolean _allow_growing = false) : max_size(_max_size), allow_growing(_allow_growing) { }

    ~LinkedList() {
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
            removeHead();
        }

        Node* newNode = new Node(newDataStruct);
        newNode->prev = nullptr;
        newNode->next = nullptr;

        if (head == nullptr) {
            head = newNode;
            tail = newNode;
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
        Node* existingNode = contains(newDataStruct);
        if (existingNode == nullptr) {
            appendNode(newDataStruct);
        } else {
            moveToTail(existingNode);
        }
    }

    /**
     * @brief Grows the maximum size of the linked list if enough memory is available.
     * 
     * @return True if the maximum size was increased, otherwise false.
     */
    bool growMaxSize() {
        if ((allow_growing) && (ESP.getFreeHeap() > 16384)) {
            // 16k free memory seems reasonable, to leave room vor web and other stuff
            // An additional 10 elements seem also reasonable
            max_size += 10;
            return true;
        }
        return false;
    }

    /**
     * @brief Clears the linked list.
     */
    void clear() {
        while (head != nullptr) {
            removeHead();
        }
    }

    /**
     * Checks if the linked list contains the given value.
     * 
     * @param value The value to be checked.
     */
    Node* contains(T* newdataStruct) {
        Node* current = head;
        while (current != nullptr) {
            if (current->dataStruct->equals(newdataStruct)) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

    /**
     * @brief Loops through all elements in the linked list and calls the given callback function.
     * 
     * The callback function can be a captive lambda function, with its data structure and index as parameters:
     *      loopList([&](T*& dataStruct, uint16_t i) { ... });
     * The data structure is passed by reference, so it can be modified within the lambda function.
     * 
     * @param callback The callback function to be called for each node.
     * @param reverse If true, the list is looped through in reverse order from latest/tail to first/head entry. Default is false.
     */       
    void loopList(std::function<void(T*&, uint16_t)> callback, bool reverse = false) {
        // This one only allows non-captive lambdas: loopList(void (*callback)(T"&, uint16_t))
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
    void moveToTail(Node* existingNode) {
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
     * @brief Removes the head/oldest element from the linked list.
     */
    void removeHead() {
        if (head == nullptr) {
            return;
        }
        
        Node* temp = head;
        head = head->next; // Move the head pointer to the second node
        delete temp; // IMPORTANT: Make sure to also free memory within the dataStruct

        if (head != nullptr) {
            head->prev = nullptr; // Remove the prev pointer from the new head
        } else { // If no node remains, the tail needs to be set to nullptr as well
            tail = nullptr;
        }

        size--;
    }

};



/**
 * A templated linked list implementation to hold a single value.
 * 
 * It is a derived class from the LinkedList class and uses a data structure to store single values.
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
struct LinkedListValue : LinkedList<DataStructValue<T>> {
    LinkedListValue(uint16_t _max_size, boolean _allow_growing = false) : LinkedList<DataStructValue<T>>(_max_size, _allow_growing) { }

    /**
     * @brief Appends a single value to the linked list.
     * 
     * @param data The value to be appended.
     */
    void append(T data) {
        // Create data structure and add node to linked list
        // Using this-> as base class/function is templated
        this->appendNode(new DataStructValue<T>(data));
    }

    /**
     * @brief Appends a single value to the linked list or moves it to the top if it already exists.
     * 
     * @param data The value to be appended or moved to the top.
     */
    void appendOrMoveToTop(T data) {
        this->appendNodeOrMoveToTail(new DataStructValue<T>(data));
    }
};



/**
 * A templated linked list implementation to hold an array of values.
 * 
 * It is a derived class from the LinkedList class and uses a data structure to store arrays of values.
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
struct LinkedListArray : LinkedList<DataStructArray<T>> {
    LinkedListArray(uint16_t _max_size, boolean _allow_growing = false) : LinkedList<DataStructArray<T>>(_max_size, _allow_growing) { }

    /**
     * @brief Appends an array of values to the linked list.
     * 
     * @param data The array of values to be appended.
     * @param size The size of the array.
     */
    void append(T* data, uint8_t size) {
        // Create data structure and add node to linked list
        // Using this-> as base class/function is templated
        this->appendNode(new DataStructArray<T>(data, size));
    }
};

#endif
