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
 * 
 * @tparam T The pre-defined data structure to be stored in the linked list.
 */
template <typename T>
struct LinkedList {

    struct Node {
        T* dataStruct;
        Node* prev;
        Node* next;

        Node() {
            dataStruct = new T();
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
     */
    void appendNode() {
        // Check if size limit is reached and cannot be grown, then remove the oldest node
        if ((size >= max_size) && !growMaxSize()) {
            removeHead();
        }

        Node* newNode = new Node();
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
};

template <typename T>
struct LinkedListValue : LinkedList<DataStructValue<T>> {
    LinkedListValue(uint16_t _max_size, boolean _allow_growing = false) : LinkedList<DataStructValue<T>>(_max_size, _allow_growing) { }

    void append(T data) {
        // Add node to list and assign the data to its datastruct
        // Using this-> as base class/function is templated
        this->appendNode();
        this->getNewestData()->data = data;
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

    ~DataStructArray() {
        delete[] data; // IMPORTANT: Make sure to also free memory within the dataStruct
    }
    void lateInit(uint8_t size) {
        data = new T[size];
    }
};

template <typename T>
struct LinkedListArray : LinkedList<DataStructArray<T>> {
    LinkedListArray(uint16_t _max_size, boolean _allow_growing = false) : LinkedList<DataStructArray<T>>(_max_size, _allow_growing) { }

    void append(T* data, uint8_t size) {
        // Add node to list and assign the data to its datastruct
        // Using this-> as base class/function is templated
        this->appendNode();
        this->getNewestData()->lateInit(size);
        for (uint8_t i = 0; i < size; i++) {
            this->getNewestData()->data[i] = data[i];
        }
    }
};

#endif


                // /**
                //  * Move element to the end of the list if it exists, otherwise appends it to the list
                //  *
                //  * @param value The value to be added to the linked list.
                //  */
                // void appendOrMoveToLast(const T& value) {
                //     if (contains(value)) {
                //         moveToLast(value);
                //     } else {
                //         append(value);
                //     }
                // }

                // /**
                //  * Checks if the linked list contains the given value.
                //  * 
                //  * @param value The value to be checked.
                //  */
                // bool contains(const T& value) const {
                //     if (StoreByPointer)
                //         return false;

                //     Node* current = head;
                //     while (current != nullptr) {
                //         if (current->data == value) {
                //             return true;
                //         }
                //         current = current->next;
                //     }
                //     return false;
                // }


                // /**
                //  * Loops through all elements in the linked list and calls the given callback function.
                //  * The callback function can be a captive lambda function.
                //  * 
                //  * @param callback The callback function to be called for each element.
                //  * @param reverse If true, the list is looped through in reverse order from latest/tail to first/head entry. Default is false.
                //  */       
                // void loopList(std::function<void(T&, uint16_t)> callback, bool reverse = false) {
                //     // The above allows to call with captive lambda, loopList([&](T& value, uint16_t index) { ... });
                //     // This one only allows non-captive lambdas: loopList(void (*callback)(T&, uint16_t))
                //     Node* current = (reverse) ? tail : head;
                //     uint16_t i = 0;
                //     while (current != nullptr) {
                //         callback(current->data, i++);
                //         current = (reverse) ? current->prev : current->next;
                //     }
                // }

                // /**
                //  * Moves the node with the given value to the end of the list.
                //  * 
                //  * @param value The value to be moved.
                //  */
                // void moveToLast(const T& value) {
                //     if (StoreByPointer) // Only values
                //         return;
                        
                //     if (head == nullptr) {
                //         return;
                //     }

                //     Node* current = head;
                //     while (current != nullptr) {
                //         if (current->data == value) {
                //             // Found the node with the given value
                //             if (current == tail) {
                //                 // The node is already the last node
                //             } else {
                //                 // Remove the node from its current position
                //                 if (current == head) {
                //                     head = current->next; // Move the head pointer to the second node
                //                     head->prev = nullptr; // Remove the prev pointer from the new head
                //                 } else {
                //                     // Neither head nor tail
                //                     current->prev->next = current->next; // Link the previous node directly to the next node
                //                     current->next->prev = current->prev; // Link the next node directly to the previous node
                //                 }
                //                 // Insert the node at the end
                //                 tail->next = current; // Set the next pointer of the old tail to the node
                //                 current->prev = tail; // Set the previous pointer of the node to the old tail
                //                 current->next = nullptr; // Remove the next pointer of the node
                //                 tail = current; // Move the tail pointer to the node
                //             }
                //             // Done
                //             return;
                //         }
                //         current = current->next;
                //     }
                // }