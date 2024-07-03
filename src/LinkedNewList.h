


#ifndef MVP3000_HELPERASDX
#define MVP3000_HELPERASDX

#include <Arduino.h>


template <typename T>
struct LinkedNewList {

    LinkedNewList(uint16_t _max_size) : max_size(_max_size) { }

    struct Node {
        T* dataStruct;
        Node* prev;
        Node* next;

        Node() {
            dataStruct = new T();
        }
        ~Node() { // Make sure to also free memory within the dataStruct
            delete dataStruct;
        }
    };

    uint16_t size;
    uint16_t max_size;

    Node* head = nullptr;
    Node* tail = nullptr;

    T* getHeadData() { return head->dataStruct; }
    T* getTailData() { return tail->dataStruct; }

    void appendNode() {
        if (size >= max_size) {
            removeFirst();
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
     * Clears the linked list.
     */
    void clear() {
        while (head != nullptr) {
            removeFirst();
        }
    }

    /**
     * Removes the first/head/oldest element from the linked list.
     */
    void removeFirst() {
        if (head == nullptr) {
            return;
        }
        Node* temp = head;
        head = head->next; // Move the head pointer to the second node
        // delete temp->dataStruct; // The dataStruct either has a destructor or contains only values           // Everything has a destructor, not needed???
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



template <typename T>
struct DataStructValue {
    T data;
};

template <typename T>
struct LinkedListValue : LinkedNewList<DataStructValue<T>> {
    LinkedListValue(uint16_t _max_size) : LinkedNewList<DataStructValue<T>>(_max_size) { }

    void append(T data) {
        // Add node to list and assign the data to its datastruct
        // Using this-> as base class/function is templated
        this->appendNode();
        this->getTailData()->data = data;
    }
};


template <typename T>
struct DataStructArray {
    T* data;

    ~DataStructArray() {
        delete[] data;
    }
    void lateInit(uint8_t size) {
        data = new T[size];
    }
};

template <typename T>
struct LinkedListArray : LinkedNewList<DataStructArray<T>> {
    LinkedListArray(uint16_t _max_size) : LinkedNewList<DataStructArray<T>>(_max_size) { }

    void append(T* data, uint8_t size) {
        // Add node to list and assign the data to its datastruct
        // Using this-> as base class/function is templated
        this->appendNode();
        this->getTailData()->lateInit(size);
        for (uint8_t i = 0; i < size; i++) {
            this->getTailData()->data[i] = data[i];
        }
    }
};



#endif
