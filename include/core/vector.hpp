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
                     private conditional<Count == 0,
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
            copy(init.begin(), init.end(), begin());
            _size = (int)init.size();
        }
        constexpr vector_c(const vector_c& o) requires (Count == 0) : _size(0) {
            this->__ensure_capacity(o._size, _size);
            uninitialized_copy(o.begin(), o.end(), begin());
            _size = o._size;
        }
        constexpr vector_c(vector_c&& o) requires (Count == 0) : _size(o._size) {
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
            uninitialized_copy(o.begin(), o.end(), begin());
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
            return this->__buffer()[0].template ptr<0>();
        }
        __forceinline const_iterator begin() const __pure {
            return this->__buffer()[0].template ptr<0>();
        }
        __forceinline iterator end() __pure {
            return this->__buffer()[_size].template ptr<0>();
        }
        __forceinline const_iterator end() const __pure {
            return this->__buffer()[_size].template ptr<0>();
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
                    construct_at(this->__buffer()[i].template ptr<0>());
                }
            } else {
                if constexpr (!is_trivially_destructible<Type>::value) {
                    for (int i = size; i < _size; ++i) {
                        destroy_at(this->__buffer()[i].template ptr<0>());
                    }
                }
            }
            _size = size;
        }

        __forceinline reference operator[](int i) __pure {
            assert(i < _size && "Index out of bounds");
            assert(i >= 0 && "Index must be non-negative");
            return *this->__buffer()[i].template ptr<0>();
        }
        __forceinline const_reference operator[](int i) const __pure {
            assert(i < _size && "Index out of bounds");
            assert(i >= 0 && "Index must be non-negative");
            return *this->__buffer()[i].template ptr<0>();
        }
        __forceinline reference front() __pure {
            assert(_size > 0 && "Vector is empty");
            return *this->__buffer()[0].template ptr<0>();
        }
        __forceinline const_reference front() const __pure {
            assert(_size > 0 && "Vector is empty");
            return *this->__buffer()[0].template ptr<0>();
        }
        __forceinline reference back() __pure {
            assert(_size > 0 && "Vector is empty");
            return *this->__buffer()[_size - 1].template ptr<0>();
        }
        __forceinline const_reference back() const __pure {
            assert(_size > 0 && "Vector is empty");
            return *this->__buffer()[_size - 1].template ptr<0>();
        }

        __forceinline void push_back(const_reference value) {
            this->__ensure_capacity(_size + 1, _size);
            construct_at(this->__buffer()[_size++].template ptr<0>(), value);
        }
        template<class... Args>
        __forceinline reference emplace_back(Args&&... args) {
            this->__ensure_capacity(_size + 1, _size);
            return *construct_at(this->__buffer()[_size++].template ptr<0>(), forward<Args>(args)...);
        }

        iterator insert(const_iterator pos, const_reference value) {
            this->__ensure_capacity(_size + 1, _size);
            assert(pos >= begin() && pos <= end() && "Invalid insert position");
            iterator ins = (iterator)pos;
            // Construct new element at end first (into uninitialized memory)
            construct_at(end(), value);
            _size++;
            // Rotate the new element into position using move-assignment
            if (ins != end() - 1) {
                Type temp = move(*(end() - 1));
                move_backward(ins, end() - 1, end());
                *ins = move(temp);
            }
            return ins;
        }
        __forceinline iterator insert(int at, const_reference value) {
            return insert(begin() + at, forward<value_type>(value));
        }
        template<class... Args>
        iterator emplace(Type* pos, Args&&... args) {
            this->__ensure_capacity(_size + 1, _size);
            assert(pos >= begin() && pos <= end() && "Invalid insert position");
            iterator ins = (iterator)pos;
            // Construct new element at end first (into uninitialized memory)
            construct_at(end(), forward<Args>(args)...);
            _size++;
            // Rotate the new element into position using move-assignment
            if (ins != end() - 1) {
                Type temp = move(*(end() - 1));
                move_backward(ins, end() - 1, end());
                *ins = move(temp);
            }
            return ins;
        }
        template<class... Args>
        __forceinline iterator emplace(int at, Args&&... args) {
            return emplace(begin() + at, forward<Args>(args)...);
        }

        iterator erase(const_iterator pos) {
            assert(_size > 0 && "Vector is empty");
            assert(pos >= begin() && pos < end() && "Invalid erase position");
            destroy_at(pos);
            iterator ins = (iterator)pos;
            move((iterator)pos + 1, end(), ins);
            // Destroy the moved-from duplicate at the old end
            _size--;
            destroy_at(end());
            return ins;
        }
        iterator erase(int at) {
            return erase(begin() + at);
        }
        void clear() {
            if constexpr (is_trivially_destructible<Type>::value) {
                _size = 0;
            } else {
                while (_size) {
                    destroy_at(this->__buffer()[--_size].template ptr<0>());
                }
            }
        }
        __forceinline void pop_back() {
            assert(_size > 0 && "Vector is empty");
            destroy_at(this->__buffer()[--_size].template ptr<0>());
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
