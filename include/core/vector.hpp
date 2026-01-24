//
//  vector.hpp
//  toybox
//
//  Created by Fredrik on 2024-03-22.
//

#pragma once

#include "core/algorithm.hpp"
#include "core/initializer_list.hpp"
#include "core/base_buffer.hpp"

namespace toybox {
    /**
     `vector_c` is a minimal implementation of `std::vector`.
     When Count > 0: Uses statically allocated backing store for performance.
     When Count == 0: Uses dynamically allocated backing store with automatic growth.
     */
    template<class Type, int Count>
    class vector_c : public nocopy_c,
                     public conditional<Count == 0,
                                        detail::base_buffer_dynamic_c<Type>,
                                        detail::base_buffer_static_c<Type, Count>>::type
    {
    public:
        using value_type = Type;
        using pointer = value_type* ;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using iterator = value_type*;
        using const_iterator = const value_type*;
        
        vector_c() : _size(0) {}
        constexpr vector_c(initializer_list<Type> init) : _size(0) {
            this->__ensure_capacity((int)init.size(), _size);
            copyn(init.begin(), (int)init.size(), begin());
            _size = (int)init.size();
        }
        constexpr vector_c(const vector_c& o) requires (Count == 0) : _size(0) {
            this->__ensure_capacity(o._size, _size);
            uninitialized_copyn(o.begin(), o.size(), begin());
            _size = o._size;
        }
        constexpr vector_c(vector_c&& __restrict o) requires (Count == 0) : _size(o._size) {
            this->__take_ownership(o);
            o._size = 0;
        }
                                            
        ~vector_c() {
            clear();
        }
            
        vector_c& operator=(const vector_c& o) requires (Count == 0) {
            if (this == &o) return *this;
            clear();
            this->__ensure_capacity(o._size, _size);
            uninitialized_copyn(o.begin(), o.size(), begin());
            _size = o._size;
            return *this;
        }
        
        vector_c& operator=(vector_c&& o) requires (Count == 0) {
            if (this == &o) return *this;
            clear();
            this->__release_ownership();
            this->__take_ownership(o);
            _size = o._size;
            o._size = 0;
            return *this;
        }
    
        __forceinline iterator begin() __pure {
            return (iterator)data();
        }
        __forceinline const_iterator begin() const __pure {
            return (iterator)data();
        }
        __forceinline iterator end() __pure {
            return (iterator)data() + _size;
        }
        __forceinline const_iterator end() const __pure {
            return (iterator)data() + _size;
        }
        __forceinline pointer data() {
            return this->__buffer()[0].template ptr<0>();
        }
        __forceinline const_pointer data() const {
            return this->__buffer()[0].template ptr<0>();
        }
        __forceinline int size() const __pure { return _size; }
        
        void resize(int size) {
            if (_size == size) return;
            if (_size < size) {
                this->__ensure_capacity(size, _size);
                for (int i = _size; i < size; ++i) {
                    construct_at(&data()[i]);
                }
            } else {
                if constexpr (!is_trivially_destructible<Type>::value) {
                    for (int i = size; i < _size; ++i) {
                        destroy_at(&data()[i]);
                    }
                }
            }
            _size = size;
        }

        __forceinline reference operator[](int i) __pure {
            assert(i < _size && "Index out of bounds");
            assert(i >= 0 && "Index must be non-negative");
            __assume_count(i, this->max_size);
            return data()[i];
        }
        __forceinline const_reference operator[](int i) const __pure {
            assert(i < _size && "Index out of bounds");
            assert(i >= 0 && "Index must be non-negative");
            __assume_count(i, this->max_size);
            return data()[i];
        }
        __forceinline reference front() __pure {
            assert(_size > 0 && "Vector is empty");
            return data()[0];
        }
        __forceinline const_reference front() const __pure {
            assert(_size > 0 && "Vector is empty");
            return data()[0];
        }
        __forceinline reference back() __pure {
            assert(_size > 0 && "Vector is empty");
            __assume_count(_size - 1, this->max_size);
            return data()[_size - 1];
        }
        __forceinline const_reference back() const __pure {
            assert(_size > 0 && "Vector is empty");
            __assume_count(_size - 1, this->max_size);
            return data()[_size - 1];
        }

        __forceinline void push_back(const_reference value) {
            this->__ensure_capacity(_size + 1, _size);
            const int i = _size;
            __assume_count(i, this->max_size);
            construct_at(&data()[i], value);
            _size++;
        }
        template<class... Args>
        __forceinline reference emplace_back(Args&&... args) {
            this->__ensure_capacity(_size + 1, _size);
            __assume_count(_size, this->max_size);
            auto r = construct_at(&data()[_size], forward<Args>(args)...);
            _size++;
            return *r;
        }

        __forceinline iterator insert(const_iterator pos, const_reference value) {
            return insert((int)(pos - begin()), value);
        }
        iterator insert(int at, const_reference value) {
            assert(at >= 0 && at <= (int)_size && "Invalid insert position");
            __assume_count(at, this->max_size);
            if (at == (int)_size) {
                push_back(value);
                return end() - 1;
            }
            this->__ensure_capacity(_size + 1, _size);
            const iterator pos = begin() + at;
            const int count = _size - at - 1;
            construct_at(end(), move(*(end() - 1)));
            _size++;
            if (count > 0) {
                moven_backward(pos + count, count, pos + count + 1);
            }
            *pos = value;
            return pos;
        }
        template<class... Args>
        __forceinline iterator emplace(Type* pos, Args&&... args) {
            return emplace((int)(pos - begin()), forward<Args>(args)...);
        }
        template<class... Args>
        iterator emplace(int at, Args&&... args) {
            assert(at >= 0 && at <= (int)_size && "Invalid emplace position");
            __assume_count(at, this->max_size);
            if (at == (int)_size) {
                emplace_back(forward<Args>(args)...);
                return end() - 1;
            }
            this->__ensure_capacity(_size + 1, _size);
            const iterator pos = begin() + at;
            const int count = _size - at - 1;
            construct_at(end(), move(*(end() - 1)));
            _size++;
            if (count > 0) {
                moven_backward(pos + count, count, pos + count + 1);
            }
            destroy_at(pos);
            construct_at(pos, forward<Args>(args)...);
            return pos;
        }

        iterator erase(const_iterator pos) {
            return erase(pos - begin());
        }
        iterator erase(int at) {
            assert(_size > 0 && "Vector is empty");
            assert(at >= 0 && at < (int)_size && "Invalid erase position");
            __assume_count(at, this->max_size);
            const iterator pos = begin() + at;
            destroy_at(pos);
            _size--;
            const int count = _size - at;
            moven(pos + 1, count, pos);
            destroy_at(end());
            return pos;
        }
        void clear() {
            if constexpr (is_trivially_destructible<Type>::value) {
                _size = 0;
            } else {
                while (_size) {
                    __assume_count(--_size, this->max_size);
                    destroy_at(&data()[--_size]);
                }
            }
        }
        __forceinline void pop_back() {
            assert(_size > 0 && "Vector is empty");
            __assume_count(--_size, this->max_size);
            destroy_at(&data()[--_size]);
        }

        __forceinline int capacity() const __pure {
            return this->__capacity();
        }

        void reserve(int new_cap) requires (Count == 0) {
            this->__ensure_capacity(new_cap, _size);
        }

    private:
        int _size;
    };
    
}
