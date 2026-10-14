// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Atomic data structure/variable
// ========================================

#pragma once
#ifndef ATOMIC_HPP
#define ATOMIC_HPP

// Memory ordering constants matching GCC/Clang built-ins
#define ATOMIC_RELAXED 0
#define ATOMIC_CONSUME 1
#define ATOMIC_ACQUIRE 2
#define ATOMIC_RELEASE 3
#define ATOMIC_ACQ_REL 4
#define ATOMIC_SEQ_CST 5

namespace util
{
    template <typename T>
    class Atomic {
    private:
        volatile T _value;

    public:
        constexpr Atomic(T val = 0) : _value(val) {}

        Atomic(const Atomic&) = delete;
        Atomic& operator=(const Atomic&) = delete;

        T load(int mem_order = ATOMIC_SEQ_CST) const {
            return __atomic_load_n(&_value, mem_order);
        }

        void store(T val, int mem_order = ATOMIC_SEQ_CST) {
            __atomic_store_n(&_value, val, mem_order);
        }

        T exchange(T val, int mem_order = ATOMIC_SEQ_CST) {
            return __atomic_exchange_n(&_value, val, mem_order);
        }

        bool compare_exchange(T& expected, T desired, int success_order = ATOMIC_SEQ_CST, int failure_order = ATOMIC_SEQ_CST) {
            return __atomic_compare_exchange_n(&_value, &expected, desired, false, success_order, failure_order);
        }

        T fetch_add(T arg, int mem_order = ATOMIC_SEQ_CST) {
            return __atomic_fetch_add(&_value, arg, mem_order);
        }

        T fetch_sub(T arg, int mem_order = ATOMIC_SEQ_CST) {
            return __atomic_fetch_sub(&_value, arg, mem_order);
        }

        // Operator Overloads

        T operator=(T val) {
            store(val);
            return val;
        }

        operator T() const {
            return load();
        }

        T operator++() {
            return fetch_add(1) + 1;
        }

        T operator++(int) {
            return fetch_add(1);
        }

        T operator--() {
            return fetch_sub(1) - 1;
        }

        T operator--(int) {
            return fetch_sub(1);
        }
    };
} // namespace util


#endif // ATOMIC_HPP
