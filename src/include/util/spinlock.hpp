// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Spinlock
// ========================================

#pragma once
#ifndef UTIL_SPINLOCK_HPP
#define UTIL_SPINLOCK_HPP

namespace util
{
    class Spinlock {
        private:
        volatile bool locked = false;
        
        public:
        void lock() {
            while (__atomic_test_and_set(&locked, __ATOMIC_ACQUIRE)) {
                asm volatile("pause" ::: "memory");
            }
        }
        
        void unlock() {
            __atomic_clear(&locked, __ATOMIC_RELEASE);
        }
        
        bool try_lock() {
            return !__atomic_test_and_set(&locked, __ATOMIC_ACQUIRE);
        }
    };
} // namespace util
    
#endif // UTIL_SPINLOCK_HPP
