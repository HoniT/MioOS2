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

    class IrqSpinlock {
    private:
        volatile bool locked = false;
        
    public:
        /// @brief IRQ-safe spinlock lock
        /// @return RFLAGS
        uint64_t lock() {
            uint64_t rflags;
            asm volatile("pushfq; \
                pop %0; \
                cli" : "=rm"(rflags) :: "memory");
            while (__atomic_test_and_set(&locked, __ATOMIC_ACQUIRE)) {
                asm volatile("pause" ::: "memory");
            }
            return rflags;
        }
        
        /// @brief IRQ-safe spinlock unlock
        /// @param rflags RFLAGS to restore from (most commonly returned by IrqSpinlock::lock())
        void unlock(uint64_t rflags) {
            __atomic_clear(&locked, __ATOMIC_RELEASE);

            asm volatile("push %0; \
                popfq" :: "g"(rflags) : "memory", "cc");
        }
        
    };
} // namespace util
    
#endif // UTIL_SPINLOCK_HPP
