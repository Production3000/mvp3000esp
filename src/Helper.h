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

        template <typename T>
        class LinkedList {
            private:
                struct Node {
                    T data;
                    Node* prev;
                    Node* next;
                };

                Node* head; // Pointer to the first-added node
                Node* tail; // Pointer to the last-added node
                uint16_t size;
                uint16_t max_size;

            public:
                /**
                 * Default constructor. The linked list has no size limit and will grow until the memory is full.
                 */
                LinkedList() : head(nullptr), tail(nullptr), size(0), max_size(std::numeric_limits<uint16_t>::max()) {}

                // Preferred constructor
                /**
                 * Preferred constructor with a maximum list size limit.
                 * 
                 * @param _max_size The maximum size of the linked list.
                 */
                LinkedList(uint16_t _max_size) : head(nullptr), tail(nullptr), size(0), max_size(_max_size) {}

                // Destructor
                ~LinkedList() {
                    clear();
                }

                /**
                 * Appends a new element to the linked list.
                 *
                 * @param value The value to be added to the linked list.
                 */
                bool append(const T& value) {
                    if (size >= max_size) {
                        return false;
                    } else {
                        Node* newNode = new Node;
                        newNode->data = value;
                        newNode->prev = nullptr;
                        newNode->next = nullptr;

                        if (size == 0) {
                            head = tail = newNode;
                        } else {
                            tail->next = newNode;
                            newNode->prev = tail;
                            tail = newNode;
                        }
                    }

                    size++;
                    return true;
                }

                /**
                 * Appends a new element to the linked list. Removes the oldest element if the list is full.
                 *
                 * @param value The value to be added to the linked list.
                 */
                void appendForce(const T& value) {
                    if (size >= max_size) {
                        removeFirst();
                    }
                    append(value);
                }

                /**
                 * Move element to the end of the list if it exists, otherwise append it to the list
                 *
                 * @param value The value to be added to the linked list.
                 */
                void appendForceOrMoveToLast(const T& value) {
                    if (contains(value)) {
                        moveToLast(value);
                    } else {
                        appendForce(value);
                    }
                }

                /**
                 * Clears the linked list and frees the memory.
                 */
                void clear() {
                    Node* current = head;
                    while (current != nullptr) {
                        Node* temp = current;
                        current = current->next;
                        delete temp;
                    }
                    head = nullptr;
                    size = 0;
                }

                /**
                 * Checks if the linked list contains the given value.
                 * 
                 * @param value The value to be checked.
                 */
                bool contains(const T& value) const {
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
                 * Loops through all elements in the linked list starting from oldest and calls the given callback function.
                 * The callback function can be a captive lambda function.
                 * 
                 * @param callback The callback function to be called for each element.
                 */       
                void loopAll(std::function<void(T&, uint16_t)> callback) {
                    // The above allows to call with captive lambda, loopAll([&](T& value, uint16_t index) { ... });
                    // This one only allows non-captive lambdas: loopAll(void (*callback)(T&, uint16_t))
                    Node* current = head;
                    uint16_t i = 0;
                    while (current != nullptr) {
                        callback(current->data, i++);
                        current = current->next;
                    }
                }

                /**
                 * Loops through all elements in the linked list starting from newest and calls the given callback function.
                 * 
                 * @param callback The callback function to be called for each element.
                 */   
                void loopAllReverse(std::function<void(T&, uint16_t)> callback) {
                    Node* current = tail;
                    uint16_t i = 0;
                    while (current != nullptr) {
                        callback(current->data, i++);
                        current = current->prev;
                    }
                }

                /**
                 * Moves the node with the given value to the end of the list.
                 * 
                 * @param value The value to be moved.
                 */
                void moveToLast(const T& value) {
                    if (head == nullptr) {
                        return;
                    }

                    Node* current = head;
                    while (current != nullptr) {
                        if (current->data == value) {
                            // Found the node with the given value
                            if (current == tail) {
                                // The node is already the last node
                                return;
                            } else {
                                // Remove the node from its current position
                                if (current == head) {
                                    head = current->next;
                                    current->next->prev = nullptr;
                                } else {
                                    // Neither head nor tail
                                    current->prev->next = current->next; // Link the previous node directly to the next node
                                    current->next->prev = current->prev; // Link the next node directly to the previous node
                                }
                                // Insert the node at the end
                                tail->next = current; // Set the next pointer of the current tail to the moved node
                                current->prev = tail; // Set the previous pointer of the moved node to the current tail
                                current->next = nullptr; // Remove the next pointer of the moved node
                                // Set the tail pointer to the moved node
                                tail = current;
                                return;
                            }
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
                    head->prev = nullptr; // Remove the prev pointer from the new head
                    delete temp;

                    size--;
                }

        };
        
};

#endif