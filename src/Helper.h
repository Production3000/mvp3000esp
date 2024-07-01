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

#ifndef MVP3000_HELPER
#define MVP3000_HELPER

#include <Arduino.h>


class Helper {
    public:
        bool isValidDecimal(String str, boolean limitToInteger = false);
        bool isValidDecimal(const char *str, boolean limitToInteger) { return isValidDecimal(String(str), limitToInteger); };
        bool isValidInteger(String str) { return isValidDecimal(str, true); };
        bool isValidInteger(const char *str) { return isValidDecimal(String(str), true); };

        String millisToTime(uint32_t total_ms);
        String upTime() { return millisToTime(millis()); };

        // The djb2 hash function by Dan Bernstein to use in switch statement
        // constexpr needs to be defined in .h
        constexpr uint32_t hashStringDjb2(const char* str, uint8_t h = 0) {
            return !str[h] ? 5381 : (hashStringDjb2(str, h+1) * 33) ^ str[h];
        };



        /**
         * A simple number array implementation for the MVP3000 framework.
         * 
         * Its provides looping functionality, clear/default value function, and a check if it is not all default values.
         * It can be intialized late with a different size and reset value.
         * 
         * @tparam T The type of data to be stored in the array.
         */
        template <typename T>
        class NumberArray {
            public:

                T* values;
                T defaultValue;
                uint8_t value_size;

                /**
                 * Default constructor. The array is empty and has no size.
                 * 
                 * @param _valueSize The size of the array.
                 * @param _defaultValue The value to be used to initialize the values of the array.
                 */
                NumberArray() : value_size(0), defaultValue(0) { }
                NumberArray(uint8_t _valueSize, T _defaultValue = 0) : value_size(_valueSize), defaultValue(_defaultValue) {
                    values = new T[_valueSize];
                    resetValues();
                }

                ~NumberArray() {
                    delete[] values;
                }

                /**
                 * (Re-)initializes the array with the given size and reset value during runtime.
                 *
                 * @param _valueSize The size of the array.
                 * @param _defaultValue The value to be used to initialize the values in the array.
                 */
                void lateInit(uint8_t _valueSize, T _defaultValue = 0) {
                    value_size = _valueSize;
                    defaultValue = _defaultValue;
                    delete [] values;
                    values = new T[value_size];
                    resetValues();
                }

                /**
                 * Initializes the value in the array with the reset value.
                 */
                void resetValues() {
                    loopArray([this](T& value, uint8_t i) {
                        values[i] = defaultValue;
                    });
                }

                /**
                 * Loops through all elements in the linked list starting from oldest and calls the given callback function.
                 * The callback function can be a captive lambda function.
                 *
                 * @param callback The callback function to be called for each element.
                 */
                void loopArray(std::function<void(T&, uint8_t)> callback) {
                    for (uint8_t i = 0; i < value_size; i++) {
                        callback(values[i], i);
                    }
                }

                /**
                 * Checks if the array only contains the default reset value.
                 */
                bool isDefault() {
                    bool isDefault = true;
                    loopArray([&](T& value, uint8_t i) {
                        if (value != defaultValue) {
                            isDefault = false;
                        }
                    });
                    return isDefault;
                }
        };



        /**
         * A simple linked list implementation for the MVP3000 framework.
         * 
         * The linked list is a generic class that can store any type of data, as pointer or value.
         * The linked list has a maximum size limit. If the limit is reached, the oldest element is removed automatically or appending stops.
         * The linked list can also check if it contains a specific value and move the value to the end of the list.
         * 
         * @tparam T The type of data to be stored in the linked list.
         * @tparam StoreByPointer If true, the linked list stores pointers to the data. If false, the linked list stores the data directly. Default is false.
         */
        template <typename T, bool StoreByPointer = false>
        class LinkedList {
            private:
                struct Node {
                    typename std::conditional<StoreByPointer, T*, T>::type data;
                    Node* prev;
                    Node* next;

                    Node() {}
                    Node(uint8_t _valueSize) {
                        data = new T[_valueSize];
                    }

                    ~Node() {
                        if (StoreByPointer) {
                            // Compiler throws an error for StoreByPointer = false even though it should never be called
                            // reinterpret_cast probably does nothing for StoreByPointer = true
                            delete[] reinterpret_cast<T*>(data);
                        }
                    }
                };

                Node* head; // Pointer to the first-added node
                Node* tail; // Pointer to the last-added node
                uint16_t size;
                uint16_t max_size;

                /**
                 * Appends a node to the linked list. Removes the oldest node if the list is already full.
                 *
                 * @param newNode The new node to be added to the linked list.
                 */
                void appendNode(Node* newNode) {
                    if (size >= max_size) {
                        removeFirst();
                    }

                    newNode->prev = nullptr;
                    newNode->next = nullptr;

                    if (size == 0) {
                        head = tail = newNode;
                    } else {
                        tail->next = newNode;
                        newNode->prev = tail;
                        tail = newNode;
                    }

                    size++;
                }

            public:
                /**
                 * Preferred constructor with a maximum list size limit.
                 * 
                 * @param _max_size The maximum size of the linked list. Default is 10.
                 */
                LinkedList() : head(nullptr), tail(nullptr), size(0), max_size(10) {}
                LinkedList(uint16_t _max_size) : head(nullptr), tail(nullptr), size(0), max_size(_max_size) {}

                ~LinkedList() {
                    clear();
                }

                typename std::conditional<StoreByPointer, T*, T>::type getFirst() { return head->data; }
                typename std::conditional<StoreByPointer, T*, T>::type getLatest() { return tail->data; }

                /**
                 * Appends a new value to the linked list. Removes the oldest element if the list is full.
                 *
                 * @param value The value to be added to the linked list.
                 */
                void append(const T& value) {
                    if (StoreByPointer) // Only values
                        return;

                    // Append new node and assign value
                    appendNode(new Node());
                    tail->data = value;
                }
                void append(NumberArray<T>& newValues) {
                    if (!StoreByPointer) // Only pointers
                        return;

                    // Append new node and copy values
                    appendNode(new Node(newValues.value_size));
                    newValues.loopArray([&](T& value, uint8_t i) {
                        tail->data[i] = value;
                    });
                }

                /**
                 * Move element to the end of the list if it exists, otherwise appends it to the list
                 *
                 * @param value The value to be added to the linked list.
                 */
                void appendOrMoveToLast(const T& value) {
                    if (StoreByPointer) // Only values
                        return;

                    if (contains(value)) {
                        moveToLast(value);
                    } else {
                        append(value);
                    }
                }

                /**
                 * Clears the linked list.
                 */
                void clear() {
                    while (head != nullptr) {
                        removeFirst();
                    }
                }

                /**
                 * Checks if the linked list contains the given value.
                 * 
                 * @param value The value to be checked.
                 */
                bool contains(const T& value) const {
                    if (StoreByPointer)
                        return false;

                    Node* current = head;
                    while (current != nullptr) {
                        if (current->data == value) {
                            return true;
                        }
                        current = current->next;
                    }
                    return false;
                }

                /**
                 * Returns the size of the linked list.
                 */
                uint32_t getSize() const {
                    return size;
                }

                /**
                 * Loops through all elements in the linked list and calls the given callback function.
                 * The callback function can be a captive lambda function.
                 * 
                 * @param callback The callback function to be called for each element.
                 * @param reverse If true, the list is looped through in reverse order from latest/tail to first/head entry. Default is false.
                 */       
                void loopList(std::function<void(T&, uint16_t)> callback, bool reverse = false) {
                    // The above allows to call with captive lambda, loopList([&](T& value, uint16_t index) { ... });
                    // This one only allows non-captive lambdas: loopList(void (*callback)(T&, uint16_t))
                    Node* current = (reverse) ? tail : head;
                    uint16_t i = 0;
                    while (current != nullptr) {
                        callback(current->data, i++);
                        current = (reverse) ? current->prev : current->next;
                    }
                }

                /**
                 * Moves the node with the given value to the end of the list.
                 * 
                 * @param value The value to be moved.
                 */
                void moveToLast(const T& value) {
                    if (StoreByPointer) // Only values
                        return;
                        
                    if (head == nullptr) {
                        return;
                    }

                    Node* current = head;
                    while (current != nullptr) {
                        if (current->data == value) {
                            // Found the node with the given value
                            if (current == tail) {
                                // The node is already the last node
                            } else {
                                // Remove the node from its current position
                                if (current == head) {
                                    head = current->next; // Move the head pointer to the second node
                                    head->prev = nullptr; // Remove the prev pointer from the new head
                                } else {
                                    // Neither head nor tail
                                    current->prev->next = current->next; // Link the previous node directly to the next node
                                    current->next->prev = current->prev; // Link the next node directly to the previous node
                                }
                                // Insert the node at the end
                                tail->next = current; // Set the next pointer of the old tail to the node
                                current->prev = tail; // Set the previous pointer of the node to the old tail
                                current->next = nullptr; // Remove the next pointer of the node
                                tail = current; // Move the tail pointer to the node
                            }
                            // Done
                            return;
                        }
                        current = current->next;
                    }
                }
                
                /**
                 * Removes the first element from the linked list.
                 */
                void removeFirst() {
                    if (head == nullptr) {
                        return;
                    }
                    Node* temp = head;
                    head = head->next; // Move the head pointer to the second node
                    delete temp;

                    if (head != nullptr) {
                        // Remove the prev pointer from the new head
                        head->prev = nullptr; 
                    } else {
                        // If no node remains, the tail needs to be set to nullptr as well
                        tail = nullptr;
                    }

                    size--;
                }
        };


        
};

#endif