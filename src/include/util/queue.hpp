// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Queue data structure
// ========================================

#pragma once
#ifndef UTIL_QUEUE_HPP
#define UTIL_QUEUE_HPP

#include <stddef.h>
#include <mm/slub.hpp>

namespace util
{
    template <typename T>
    class Queue {
    private:
        struct Node {
            T data;
            Node* next;

            Node(const T& val) : data(val), next(nullptr) {}
        };

        Node* head_;
        Node* tail_;
        size_t size_;

    public:
        // Constructor & Destructor
        Queue() : head_(nullptr), tail_(nullptr), size_(0) {}
        
        ~Queue() { 
            clear();
        }

        // Disable copy semantics to prevent accidental massive heap allocations
        Queue(const Queue&) = delete;
        Queue& operator=(const Queue&) = delete;

        // Modifiers
        
        /// @brief Adds an element to the back of the queue.
        /// @return true if memory was allocated successfully, false on OOM
        bool push(const T& val) {
            void* mem = kmalloc(sizeof(Node));
            if (!mem) return false;

            // Placement new
            Node* new_node = new (mem) Node(val);

            if (!tail_) {
                head_ = tail_ = new_node;
            } else {
                tail_->next = new_node;
                tail_ = new_node;
            }
            size_++;
            return true;
        }

        /// @brief Removes an element from the front of the queue.
        void pop() {
            if (!head_) return;
            
            Node* to_delete = head_;
            head_ = head_->next;
            
            // If the queue becomes empty, reset the tail pointer as well
            if (!head_) {
                tail_ = nullptr;
            }

            // Explicitly call the destructor for the stored type T
            to_delete->~Node();
            kfree(to_delete);
            size_--;
        }

        /// @brief Destroys all elements and frees SLUB memory
        void clear() {
            Node* current = head_;
            while (current) {
                Node* next = current->next;
                current->~Node(); // Crucial if T manages its own resources
                kfree(current);
                current = next;
            }
            head_ = tail_ = nullptr;
            size_ = 0;
        }

        // Accessors

        size_t size() const { return size_; }
        bool empty() const { return size_ == 0; }
        
        /// @brief Returns a reference to the front element (first to be popped).
        T& front() { return head_->data; }
        const T& front() const { return head_->data; }

        /// @brief Returns a reference to the back element (last added).
        T& back() { return tail_->data; }
        const T& back() const { return tail_->data; }
    };
}

#endif // UTIL_QUEUE_HPP
