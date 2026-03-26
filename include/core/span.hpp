//
//  span.hpp
//  toybox_index
//
//  Created by Fredrik on 2025-12-28.
//

#pragma once

#include "core/cincludes.hpp"

namespace toybox {
    
    template<class Type>
    class span_c {
    public:
        static constexpr int max_size = 0x8000 / sizeof(Type);
        using value_type = Type;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using iterator = value_type*;
        using const_iterator = const value_type*;

        span_c() : _begin(nullptr), _size(0) {}
        span_c(iterator begin, int size) : _begin(begin), _size(size) {}
        span_c(const span_c& o) = default;
        span_c(span_c&& o) = default;
        span_c& operator=(const span_c& o) = default;
        span_c& operator=(span_c&& o) = default;

        __forceinline iterator begin() { return _begin; }
        __forceinline const_iterator begin() const_pure { return _begin; }
        __forceinline iterator end() { return _begin + _size; }
        __forceinline const_iterator end() const_pure { return _begin + _size; }
        __forceinline pointer data() { return _begin; }
        __forceinline const_pointer data() const_pure { return _begin; }
        __forceinline int size() const_pure { return _size; }

        reference operator[](int i) {
            assert(i < _size && "Index out of bounds");
            assert(i >= 0 && "Index must be non-negative");
            __assume_count(i, this->max_size);
            return _begin[i];
        }
        const_reference operator[](int i) const_pure {
            assert(i < _size && "Index out of bounds");
            assert(i >= 0 && "Index must be non-negative");
            __assume_count(i, this->max_size);
            return _begin[i];
        }
        reference front() {
            assert(_size > 0 && "Span is empty");
            return _begin[0];
        }
        const_reference front() const_pure {
            assert(_size > 0 && "Span is empty");
            return _begin[0];
        }
        reference back() {
            assert(_size > 0 && "Span is empty");
            return _begin[_size - 1];
        }
        const_reference back() const_pure {
            assert(_size > 0 && "Span is empty");
            return _begin[_size - 1];
        }

    private:
        iterator _begin;
        int _size;
    };
    
}
