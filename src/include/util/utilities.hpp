// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Different types of miscelanious utilities
// ========================================

#pragma once
#ifndef UTILITIES_HPP
#define UTILITIES_HPP

namespace util {
    // Minimal implementation of std::remove_reference
    template< class T > struct remove_reference      { typedef T type; };
    template< class T > struct remove_reference<T&>  { typedef T type; };
    template< class T > struct remove_reference<T&&> { typedef T type; };

    // Minimal implementation of std::forward
    template< class T >
    constexpr T&& forward( typename remove_reference<T>::type& t ) noexcept {
        return static_cast<T&&>(t);
    }

    template< class T >
    constexpr T&& forward( typename remove_reference<T>::type&& t ) noexcept {
        return static_cast<T&&>(t);
    }
}

#endif // UTILITIES_HPP
