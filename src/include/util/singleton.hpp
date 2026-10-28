// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Singleton base class
// ========================================

#pragma once
#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <mm/slub.hpp>

namespace util
{
    template <typename T>
    class Singleton {
    private:
        static inline T* instance = nullptr;

    protected:
        Singleton() {}
        
        virtual ~Singleton() {} 

    public:
        static T* get_instance() {
            if (instance != nullptr) {
                return instance; 
            }

            void* mem = kmalloc(sizeof(T));
            if (mem == nullptr) {
                return nullptr;
            }

            instance = new(mem) T();
            
            return instance;
        }

        static void destroy() {
            if (instance != nullptr) {
                instance->~T();
                kfree(instance);
                instance = nullptr;
            }
        }
    };

} // namespace util


#endif // SINGLETON_HPP
