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

#ifndef MVP3000_HELPER_LINKEDLIST
#define MVP3000_HELPER_LINKEDLIST

#include <Arduino.h>

/**
 * @brief A templated linked list implementation for the MVP3000 framework.
 * 
 * The list has a maximum size limit set during initialization. If the limit is reached, the oldest element is automatically removed.
 * 3000 bare: append, clear, loop, getNewest, getOldest, getSize - dataStruct has no requirements
 * 3001 extends bare: appendUnique, findByContent, removeByContent - dataStruct needs equals() method
 * 3010 extends bare: bookmarkByIndex, hasBookmark, moveBookmark - dataStruct has no requirements
 * 3011 combines 3001 and 3010
 * 
 * @tparam T The pre-defined data structure to be stored in the linked list.
 * @param size The maximum element count of the list.
 */
template <typename T>
struct LinkedListNEW3000 {

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

    T* getNewestData() { return tail->dataStruct; }
    T* getOldestData() { return head->dataStruct; }

    uint16_t size;
    uint16_t max_size;

    uint16_t getSize() const { return size; }
    uint16_t getMaxSize() const { return max_size; }


    /**
     * @brief Constructor with a maximum list size limit.
     * 
     * @param size The maximum element count of the list.
     */
    LinkedListNEW3000(uint16_t size) : max_size(size) { }
    // This class is often inherited virtually. In that case its constructor is called directly by the most derived class's constructor
    // To simplify the code in user classes, a default constructor is added here, and the max size is set in the 30xx classes
    LinkedListNEW3000() : max_size(1) { } 

    ~LinkedListNEW3000() {
        clear(); // IMPORTANT: Make sure to also free memory within the dataStruct
    }


    /**
     * @brief Appends a node to the linked list. Removes the oldest/head node if the list is already full.
     * 
     * @param newDataStruct The data structure to be passed on to be stored in the linked list.
     */
    void append(T* newDataStruct) {
        // Check if size limit is reached and cannot be grown, then remove the oldest node
        if (size >= max_size) {
            _removeNode(head);
        }
        // Append the new node
        Node* newNode = new Node(newDataStruct);
        newNode->prev = nullptr;
        newNode->next = nullptr;
        // Set head, tail, increment size
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
     * @brief Clears the linked list.
     */
    void clear() {
        while (head != nullptr) {
            _removeNode(head);
        }
    }

    /**
     * @brief Loops through all elements in the linked list and calls the given callback function.
     * 
     * @param callback The callback function to be called for each node, in lambda format: [&](T*& dataStruct, uint16_t i) { ... } . The data structure is passed by reference, so it can be modified within the lambda function.
     * @param reverse (optional) Start from the newest/tail entry towards the oldest/head entry. Default is false.
     */
    void loop(std::function<void(T*&, uint16_t)> callback, boolean reverse = false) {
        Node* current = (reverse) ? tail : head;
        uint16_t i = 0;
        while (current != nullptr) {
            callback(current->dataStruct, i++);
            current = (reverse) ? current->prev : current->next;
        }
    }

    void _removeNode(Node* existingNode) {
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
 * @brief A templated linked list implementation for the MVP3000 framework.
 * 
 * The list has a maximum size limit set during initialization. If the limit is reached, the oldest element is automatically removed.
 * 3000 bare: append, clear, loop, getNewest, getOldest, getSize - dataStruct has no requirements
 * 3001 extends bare: appendUnique, findByContent, removeByContent - dataStruct needs equals() method
 * 3010 extends bare: bookmarkByIndex, hasBookmark, moveBookmark - dataStruct has no requirements
 * 3011 combines 3001 and 3010
 * 
 * @tparam T The pre-defined data structure to be stored in the linked list.
 * @param size The maximum element count of the list.
 */
template <typename T>
struct LinkedListNEW3001 : virtual LinkedListNEW3000<T> {
    LinkedListNEW3001(uint16_t size) : LinkedListNEW3000<T>(size) { this->max_size = size; }

    using typename LinkedListNEW3000<T>::Node;

    /**
     * @brief Append element to the list or move it to the tail of the list.
     *
     * @param dataStruct The data structure to be appended to the linked list.
     * @param moveToFront (optional) Move the element to the tail/newest of the list. Default is true.
     */
    void appendUnique(T* dataStruct, boolean moveToFront = true) {
        if (moveToFront) {
            // Just delete if exists and create new, this does not allow a counter of how often that happend
            this->_removeNode(findByContent(dataStruct));
            this->append(dataStruct);
        } else {
            // Only append if not exists
            if (!findByContent(dataStruct))
                this->append(dataStruct);
        }
    }

    Node* findByContent(T* dataStruct) {
        Node* current = this->head;
        while (current != nullptr) {
            if (current->dataStruct->equals(dataStruct)) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

    /**
     * @brief Search the nodes for given content and removes the node.
     * 
     * @param dataStruct The data structure to be removed.
     */
    void removeByContent(T* dataStruct) {
        this->_removeNode(this->findByContent(dataStruct));
    }

};


/**
 * @brief A templated linked list implementation for the MVP3000 framework.
 * 
 * The list has a maximum size limit set during initialization. If the limit is reached, the oldest element is automatically removed.
 * 3000 bare: append, clear, loop, getNewest, getOldest, getSize - dataStruct has no requirements
 * 3001 extends bare: appendUnique, findByContent, removeByContent - dataStruct needs equals() method
 * 3010 extends bare: bookmarkByIndex, hasBookmark, moveBookmark - dataStruct has no requirements
 * 3011 combines 3001 and 3010
 * 
 * @tparam T The pre-defined data structure to be stored in the linked list.
 * @param size The maximum element count of the list.
 */
template <typename T>
struct LinkedListNEW3010 : virtual LinkedListNEW3000<T> {
    LinkedListNEW3010(uint16_t size) : LinkedListNEW3000<T>(size) { this->max_size = size; }

    using typename LinkedListNEW3000<T>::Node;

    Node* bookmark = nullptr; // bookmark is a temporary pointer, e.g. for slow looping

    T* getBookmarkData() { return bookmark->dataStruct; }

    /**
     * @brief Check if the bookmark is set.
     * 
     * @return true if the bookmark is set, false if it is nullptr.
     */
    boolean hasBookmark() { return bookmark != nullptr; }

    /**
     * @brief Move the bookmark to the next node.
     * 
     * @return true if the bookmark was moved successfully, false if the end of the list is reached.
     * @param reverse (optional) Move from the newest/tail towards the oldest/head entry. Default is false.
     */
    boolean moveBookmark(boolean reverse = false) { if (bookmark == nullptr) return false; bookmark = (reverse) ? bookmark->prev : bookmark->next; if (bookmark == nullptr) return false; return true; }

    /**
     * @brief Find the node at the given index and bookmark it.
     * 
     * @param index The index of the node to be retrieved, starting from zero.
     * @param reverse (optional) Start from the newest/tail entry towards the oldest/head entry. Default is false.
     * @param noNull (optional) Return head/tail if index is out of bounds. Default is false.
     */
    void bookmarkByIndex(uint16_t index, boolean reverse = false, boolean noNull = false) {
        if (index >= this->size) {
            if (noNull) { // Return the tail/head if index is out of bounds
                bookmark = (reverse) ? this->head : this->tail;
            } else { // Return nullptr if index is out of bounds
                bookmark = nullptr;
            }
        } else { // Find the node at the given index, start from head/tail
            bookmark = (reverse) ? this->tail : this->head;
            for (uint16_t i = 0; i < index; i++) {
                bookmark = (reverse) ? bookmark->prev : bookmark->next;
            }
        }
    }
};


/**
 * @brief A templated linked list implementation for the MVP3000 framework.
 * 
 * The list has a maximum size limit set during initialization. If the limit is reached, the oldest element is automatically removed.
 * 3000 bare: append, clear, loop, getNewest, getOldest, getSize - dataStruct has no requirements
 * 3001 extends bare: appendUnique, findByContent, removeByContent - dataStruct needs equals() method
 * 3010 extends bare: bookmarkByIndex, hasBookmark, moveBookmark, getBookmarkData - dataStruct has no requirements
 * 3011 combines 3001 and 3010
 * 
 * @tparam T The pre-defined data structure to be stored in the linked list.
 * @param size The maximum element count of the list.
 */
template <typename T>
struct LinkedListNEW3011 : LinkedListNEW3001<T>, LinkedListNEW3010<T> {
    LinkedListNEW3011(uint16_t size) : LinkedListNEW3001<T>(size), LinkedListNEW3010<T>(size) { }
};


#endif