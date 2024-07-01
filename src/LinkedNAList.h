
#ifndef MVP3000_HELPERXX
#define MVP3000_HELPERXX

#include <Arduino.h>

#include "Helper.h"


template <typename T>
class LinkedNAList {
    private:
        struct Node {
            T* data;
            Node* prev;
            Node* next;

            Node(uint8_t _valueSize) {
                data = new T[_valueSize];
            }
        };

        Node* head; // Pointer to the first-added node
        Node* tail; // Pointer to the last-added node
        uint16_t size;
        uint16_t max_size;

        T resetValue;
        uint8_t value_size;

    public:

        // Preferred constructor
        /**
         * Preferred constructor with a maximum list size limit.
         * 
         * @param _max_size The maximum size of the linked list.
         */
        LinkedNAList(uint16_t _max_size) : head(nullptr), tail(nullptr), size(0), max_size(_max_size) {}

        // Destructor
        ~LinkedNAList() {
            clear();
        }

        /**
         * Returns the latest/tail element of the linked list.
         */
        Node* getFirst() { return head; }
        Node* getLatest() { return tail; }

        void appendNumericArray(Helper::NumberArray<T>& newValues) {
            if (size >= max_size) {
                removeFirst();
            }

            Node* newNode = new Node(newValues.value_size);

            // Copy values to the new node
            newValues.loopArray([&](T& newValue, uint16_t i) { 
                newNode->data[i] = newValue;
            });

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
         * Loops through all elements in the linked list starting from oldest and calls the given callback function.
         * The callback function can be a captive lambda function.
         * 
         * @param callback The callback function to be called for each element.
         */       
        void loopList(std::function<void(T&, uint16_t)> callback) {
            // The above allows to call with captive lambda, loopList([&](T& value, uint16_t index) { ... });
            // This one only allows non-captive lambdas: loopList(void (*callback)(T&, uint16_t))
            Node* current = head;
            uint16_t i = 0;
            while (current != nullptr) {
                callback(current->data, i++);
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

            delete temp->data;
            delete temp;

            size--;
        }
};

#endif