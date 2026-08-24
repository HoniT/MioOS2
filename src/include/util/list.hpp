// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// List data structure
// ========================================

#pragma once
#ifndef UTIL_LIST_HPP
#define UTIL_LIST_HPP

#include <stdint.h>
#include <stddef.h>
#include <mm/slub.hpp>
#include <mm/mm_defs.hpp>

namespace util
{
    template <typename T>
    class List {
    private:
        struct Node {
            T data;
            Node* next;
            Node* prev;

            Node(const T& val) : data(val), next(nullptr), prev(nullptr) {}
        };

        Node* head_;
        Node* tail_;
        size_t size_;

    public:
        /// @brief Basic Forward Iterator for range-based for loops
        class Iterator {
        private:
            Node* current;
        public:
            Iterator(Node* node) : current(node) {}
            Iterator& operator++() { if (current) current = current->next; return *this; }
            bool operator!=(const Iterator& other) const { return current != other.current; }
            T& operator*() { return current->data; }
        };

        Iterator begin() { return Iterator(head_); }
        Iterator end() { return Iterator(nullptr); }

        // Constructor & Destructor

        List() : head_(nullptr), tail_(nullptr), size_(0) {}
        
        ~List() { 
            clear();
        }

        // Disable copy semantics to prevent accidental massive heap allocations
        List(const List&) = delete;
        List& operator=(const List&) = delete;

        // Modifiers
        
        /// @brief Adds an element to the end of the list.
        /// @return true if memory was allocated successfully, false on OOM
        bool push_back(const T& val) {
            void* mem = kmalloc(sizeof(Node));
            if (!mem) return false;

            Node* new_node = new (mem) Node(val);

            if (!tail_) {
                head_ = tail_ = new_node;
            } else {
                tail_->next = new_node;
                new_node->prev = tail_;
                tail_ = new_node;
            }
            size_++;
            return true;
        }

        /// @brief Adds an element to the front of the list.
        bool push_front(const T& val) {
            void* mem = kmalloc(sizeof(Node));
            if (!mem) return false;

            Node* new_node = new (mem) Node(val);

            if (!head_) {
                head_ = tail_ = new_node;
            } else {
                head_->prev = new_node;
                new_node->next = head_;
                head_ = new_node;
            }
            size_++;
            return true;
        }

        void pop_back() {
            if (!tail_) return;
            
            Node* to_delete = tail_;
            tail_ = tail_->prev;
            
            if (tail_) {
                tail_->next = nullptr;
            } else {
                head_ = nullptr;
            }

            to_delete->~Node();
            kfree(to_delete);
            size_--;
        }

        void pop_front() {
            if (!head_) return;

            Node* to_delete = head_;
            head_ = head_->next;

            if (head_) {
                head_->prev = nullptr;
            } else {
                tail_ = nullptr;
            }

            to_delete->~Node();
            kfree(to_delete);
            size_--;
        }

        /// @brief Destroys all elements and frees SLUB memory
        void clear() {
            Node* current = head_;
            while (current) {
                Node* next = current->next;
                current->~Node(); // If T manages resources
                kfree(current);
                current = next;
            }
            head_ = tail_ = nullptr;
            size_ = 0;
        }

        // Accessors

        size_t size() const { return size_; }
        bool empty() const { return size_ == 0; }
        
        T& front() { return head_->data; }
        T& back() { return tail_->data; }
    };
}

#endif // UTIL_LIST_HPP
