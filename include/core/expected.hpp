//
//  expected.hpp
//  toybox
//
//  Created by Fredrik on 2025-12-02.
//

#pragma once

#include <errno.h>
#include "core/type_traits.hpp"
#include "core/utility.hpp"

namespace toybox {


    // Helper class for unexpected values, error as errno
    class unexpected_c {
    public:
        unexpected_c() = delete;
        explicit unexpected_c(int err) : _error(err) {}
        
        int error() const { return _error; }
    private:
        int _error;
    };
    
    // Tag for errno-checking constructor
    struct failable_t { explicit failable_t() = default; };
    inline failable_t failable{};
    
    // Similar to std::expected but only uses int for error, based on errno.
    template<typename T>
    class expected_c {
    public:
        // Default constructor
        expected_c() : _value(), _error(0) {}
        
        // Value constructors
        expected_c(const T& value) : _value(value), _error(0) {}
        expected_c(T&& value) : _value(static_cast<T&&>(value)), _error(0) {}
        
        // Error constructor
        expected_c(const unexpected_c& unex) : _error(unex.error()) {}

        // Failable with errno value constructor
        template<typename... Args>
        explicit expected_c(failable_t, Args&&... args) : _error(0) {
            errno = 0;
            construct_at(&_value, static_cast<Args&&>(args)...);
            if (errno != 0) {
                _error = errno;
                if (!is_trivially_destructible<T>::value) {
                    _value.~T();
                }
            }
        }
        
        
        // Copy constructor
        expected_c(const expected_c& other) : _error(other._error) {
            if (_error == 0) {
                construct_at(&_value, other._value);
            }
        }
        
        // Move constructor
        expected_c(expected_c&& other) : _error(other._error) {
            if (_error == 0) {
                construct_at(&_value, move(other._value));
            }
        }
        
        // Destructor
        ~expected_c() {
            if (!is_trivially_destructible<T>::value) {
                if (_error == 0) {
                    _value.~T();
                }
            }
        }
        
        // Observer - error() == 0 means has value
        explicit operator bool() const { return _error == 0; }
        
        // Value access
        T& value() & { return _value; }
        const T& value() const & { return _value; }
        T&& value() && { return static_cast<T&&>(_value); }
        const T&& value() const && { return static_cast<const T&&>(_value); }
        
        T& operator*() & { return _value; }
        const T& operator*() const & { return _value; }
        T&& operator*() && { return static_cast<T&&>(_value); }
        const T&& operator*() const && { return static_cast<const T&&>(_value); }
        
        T* operator->() { return &_value; }
        const T* operator->() const { return &_value; }
        
        // Error access
        int error() const { return _error; }
        
    private:
        union { T _value; };
        int _error;
    };
    
    // Specialization for void
    template<>
    class expected_c<void> {
    public:
        expected_c() : _error(0) {}
        expected_c(const unexpected_c& unex) : _error(unex.error()) {}
        
        expected_c(const expected_c&) = default;
        expected_c(expected_c&&) = default;
        ~expected_c() = default;
        
        explicit operator bool() const { return _error == 0; }
        
        void value() const {}
        void operator*() const {}
        
        int error() const { return _error; }
    private:
        int _error;
    };
    
    template<typename T>
    T& expected_cast(expected_c<T>& exp) {
        hard_assert(exp && "Expected has error");
        return *reinterpret_cast<T*>(&exp);
    }
    
    template<typename T>
    const T& expected_cast(const expected_c<T>& exp) {
        hard_assert(exp && "Expected has error");
        return *reinterpret_cast<const T*>(&exp);
    }
    
    template<typename T>
    T* expected_cast(expected_c<T>* exp) {
        hard_assert(*exp && "Expected has error");
        return reinterpret_cast<T*>(exp);
    }
    
    template<typename T>
    const T* expected_cast(const expected_c<T>* exp) {
        hard_assert(*exp && "Expected has error");
        return reinterpret_cast<const T*>(exp);
    }
    
}
