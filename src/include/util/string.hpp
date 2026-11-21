// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// String data structure
// ========================================

#pragma once
#ifndef UTIL_STRING_HPP
#define UTIL_STRING_HPP

#include <stddef.h>
#include <mm/slub.hpp>
#include <lib/string_util.hpp>
#include <lib/mem_util.hpp>

namespace util
{
    class String {
    private:
        char* data_;
        size_t size_;
        size_t capacity_;

        bool ensure_capacity(size_t min_cap) {
            if (min_cap <= capacity_) return true;
            size_t new_cap = (capacity_ == 0) ? 15 : capacity_ * 2;
            return reserve(new_cap < min_cap ? min_cap : new_cap);
        }

    public:
        static constexpr size_t npos = static_cast<size_t>(-1);

        // Iterators for range-based for loops
        char* begin() { return data_; }
        char* end() { return data_ ? data_ + size_ : nullptr; }
        const char* begin() const { return data_; }
        const char* end() const { return data_ ? data_ + size_ : nullptr; }

        // Constructors, Move & Destructor

        String() : data_(nullptr), size_(0), capacity_(0) {}

        String(const char* str) : data_(nullptr), size_(0), capacity_(0) {
            assign(str);
        }

        String(const char* str, size_t len) : data_(nullptr), size_(0), capacity_(0) {
            assign(str, len);
        }

        ~String() {
            if (data_) kfree(data_);
            data_ = nullptr;
            size_ = capacity_ = 0;
        }

        // Disable implicit copy to prevent accidental heap allocations; use clone()
        String(const String&) = delete;
        String& operator=(const String&) = delete;

        String(String&& other) noexcept
            : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
            other.data_ = nullptr;
            other.size_ = other.capacity_ = 0;
        }

        String& operator=(String&& other) noexcept {
            if (this != &other) {
                if (data_) kfree(data_);
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;
                other.data_ = nullptr;
                other.size_ = other.capacity_ = 0;
            }
            return *this;
        }

        String& operator=(const char* str) {
            assign(str);
            return *this;
        }

        /// @brief Explicitly clones this string into a new instance
        String clone() const {
            return String(data_, size_);
        }

        /// @brief Fallible clone into an existing String instance
        bool clone_to(String& out) const {
            return (this == &out) ? true : out.assign(data_, size_);
        }

        // Element Access

        char& operator[](size_t index) { return data_[index]; }
        const char& operator[](size_t index) const { return data_[index]; }

        /// @brief Safe bounds-checked access returning a pointer
        char* at(size_t index) { return (index < size_) ? &data_[index] : nullptr; }
        const char* at(size_t index) const { return (index < size_) ? &data_[index] : nullptr; }

        char& front() { return data_[0]; }
        const char& front() const { return data_[0]; }
        char& back() { return data_[size_ - 1]; }
        const char& back() const { return data_[size_ - 1]; }

        const char* c_str() const { return data_ ? data_ : ""; }
        char* data() { return data_; }
        const char* data() const { return data_; }

        // Capacity

        size_t size() const { return size_; }
        size_t length() const { return size_; }
        size_t capacity() const { return capacity_; }
        bool empty() const { return size_ == 0; }
        explicit operator bool() const { return size_ != 0; }

        /// @brief Pre-allocates SLUB memory for at least new_cap characters
        bool reserve(size_t new_cap) {
            if (new_cap <= capacity_) return true;

            char* new_data = static_cast<char*>(kmalloc(new_cap + 1));
            if (!new_data) return false;

            if (data_) {
                memcpy(new_data, data_, size_);
                kfree(data_);
            }
            new_data[size_] = '\0';
            data_ = new_data;
            capacity_ = new_cap;
            return true;
        }

        // Modifiers

        bool assign(const char* str, size_t len) {
            if (!str || len == 0) {
                clear();
                return true;
            }
            if (!ensure_capacity(len)) return false;

            memmove(data_, str, len);
            size_ = len;
            data_[size_] = '\0';
            return true;
        }

        bool assign(const char* str) {
            return assign(str, strlen(str));
        }

        bool push_back(char ch) {
            if (!ensure_capacity(size_ + 1)) return false;
            data_[size_++] = ch;
            data_[size_] = '\0';
            return true;
        }

        void pop_back() {
            if (size_ == 0) return;
            data_[--size_] = '\0';
        }

        bool append(const char* str, size_t len) {
            if (!str || len == 0) return true;
            if (!ensure_capacity(size_ + len)) return false;

            memcpy(data_ + size_, str, len);
            size_ += len;
            data_[size_] = '\0';
            return true;
        }

        bool append(const char* str) { return append(str, strlen(str)); }

        bool append(const String& other) {
            if (this == &other) {
                size_t orig = size_;
                if (!ensure_capacity(orig * 2)) return false;
                memcpy(data_ + orig, data_, orig);
                size_ = orig * 2;
                data_[size_] = '\0';
                return true;
            }
            return append(other.data_, other.size_);
        }

        /// @brief Inserts a C-string at index pos
        bool insert(size_t pos, const char* str) {
            if (pos > size_) return false;
            size_t len = strlen(str);
            if (len == 0) return true;
            if (!ensure_capacity(size_ + len)) return false;

            memmove(data_ + pos + len, data_ + pos, size_ - pos);
            memcpy(data_ + pos, str, len);
            size_ += len;
            data_[size_] = '\0';
            return true;
        }

        /// @brief Removes len characters starting at pos
        bool erase(size_t pos, size_t len = npos) {
            if (pos > size_) return false;
            if (len > size_ - pos) len = size_ - pos;

            memmove(data_ + pos, data_ + pos + len, size_ - (pos + len));
            size_ -= len;
            if (data_) data_[size_] = '\0';
            return true;
        }

        /// @brief Returns a new owned substring
        String substr(size_t pos, size_t len = npos) const {
            if (pos >= size_) return String();
            size_t max_len = size_ - pos;
            return String(data_ + pos, len < max_len ? len : max_len);
        }

        void clear() {
            size_ = 0;
            if (data_) data_[0] = '\0';
        }

        String& operator+=(char ch) { push_back(ch); return *this; }
        String& operator+=(const char* str) { append(str); return *this; }
        String& operator+=(const String& other) { append(other); return *this; }

        // Inspection & Search

        int compare(const char* other, size_t other_len) const {
            size_t min_len = size_ < other_len ? size_ : other_len;
            for (size_t i = 0; i < min_len; ++i) {
                if (data_[i] != other[i]) {
                    return static_cast<unsigned char>(data_[i]) < static_cast<unsigned char>(other[i]) ? -1 : 1;
                }
            }
            return (size_ < other_len) ? -1 : (size_ > other_len ? 1 : 0);
        }

        int compare(const char* other) const { return compare(other, strlen(other)); }
        int compare(const String& other) const { return compare(other.data_, other.size_); }

        bool starts_with(char ch) const { return size_ > 0 && data_[0] == ch; }
        bool starts_with(const char* prefix) const {
            size_t len = strlen(prefix);
            if (len > size_) return false;
            for (size_t i = 0; i < len; ++i) if (data_[i] != prefix[i]) return false;
            return true;
        }

        bool ends_with(char ch) const { return size_ > 0 && data_[size_ - 1] == ch; }
        bool ends_with(const char* suffix) const {
            size_t len = strlen(suffix);
            if (len > size_) return false;
            for (size_t i = 0; i < len; ++i) if (data_[size_ - len + i] != suffix[i]) return false;
            return true;
        }

        size_t find(char ch, size_t pos = 0) const {
            for (size_t i = pos; i < size_; ++i) if (data_[i] == ch) return i;
            return npos;
        }

        size_t find(const char* needle, size_t pos = 0) const {
            size_t len = strlen(needle);
            if (len == 0) return pos <= size_ ? pos : npos;
            if (len > size_ || pos > size_ - len) return npos;

            for (size_t i = pos; i <= size_ - len; ++i) {
                size_t j = 0;
                while (j < len && data_[i + j] == needle[j]) ++j;
                if (j == len) return i;
            }
            return npos;
        }

        size_t rfind(char ch, size_t pos = npos) const {
            if (size_ == 0) return npos;
            for (size_t i = (pos < size_ ? pos : size_ - 1) + 1; i > 0; --i) {
                if (data_[i - 1] == ch) return i - 1;
            }
            return npos;
        }

        bool contains(char ch) const { return find(ch) != npos; }
        bool contains(const char* needle) const { return find(needle) != npos; }
    };

    // Operators

    inline String operator+(const String& lhs, const String& rhs) {
        String out;
        out.reserve(lhs.size() + rhs.size());
        out.append(lhs);
        out.append(rhs);
        return out;
    }

    inline String operator+(const String& lhs, const char* rhs) {
        String out;
        out.reserve(lhs.size() + strlen(rhs)); // or let append grow
        out.append(lhs);
        out.append(rhs);
        return out;
    }

    inline String operator+(const char* lhs, const String& rhs) {
        String out(lhs);
        out.append(rhs);
        return out;
    }

    inline bool operator==(const String& a, const String& b) { return a.compare(b) == 0; }
    inline bool operator!=(const String& a, const String& b) { return a.compare(b) != 0; }
    inline bool operator<(const String& a, const String& b)  { return a.compare(b) < 0; }
    inline bool operator==(const String& a, const char* b)   { return a.compare(b) == 0; }
    inline bool operator!=(const String& a, const char* b)   { return a.compare(b) != 0; }
}

#endif // UTIL_STRING_HPP
