//
//  map.hpp
//  toybox_index
//
//  Created by Fredrik on 2025-12-28.
//

#pragma once

#include "core/algorithm.hpp"
#include "core/initializer_list.hpp"
#include "core/base_buffer.hpp"
#include "core/utility.hpp"

namespace toybox {

    /**
     `map_c` is a minimal implementation of `std::map`.
     When Count > 0: Uses statically allocated backing store for performance.
     When Count == 0: Uses dynamically allocated backing store with automatic growth.
     Elements are sorted, and guaranteed to be continious in memory.
     */
    template<class Key, class Type, int Count>
    class map_c : public nocopy_c,
                  private conditional<Count == 0,
                                      detail::base_buffer_dynamic_c<pair_c<Key,Type>>,
                                      detail::base_buffer_static_c<pair_c<Key,Type>, Count>>::type
    {
        static_assert(is_trivial<Key>::value);
    public:
        using key_type = Key;
        using mapped_type = Type;
        using value_type = pair_c<key_type,mapped_type>;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using iterator = value_type*;
        using const_iterator = const value_type*;

        map_c() : _size(0) {}
        constexpr map_c(initializer_list<value_type> init) : _size(0) {
            this->__ensure_capacity((int)init.size(), _size);
            copy(init.begin(), init.end(), begin());
            _size = (int)init.size();
            sort_keys();
        }
        constexpr map_c(const map_c& o) requires (Count == 0) : _size(0) {
            this->__ensure_capacity(o._size, _size);
            uninitialized_copy(o.begin(), o.end(), begin());
            _size = o._size;
        }
        constexpr map_c(map_c&& o) requires (Count == 0) : _size(o._size) {
            this->__take_ownership(o);
            o._size = 0;
        }
                                            
        ~map_c() {
            clear();
        }
            
        map_c& operator=(const map_c& o) requires (Count == 0) {
            if (this == &o) return *this;
            clear();
            this->__ensure_capacity(o._size, _size);
            uninitialized_copy(o.begin(), o.end(), begin());
            _size = o._size;
            return *this;
        }
        
        map_c& operator=(map_c&& o) requires (Count == 0) {
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
        
        iterator find(const Key& key) {
            auto it = lower_bound(begin(), end(), key, key_comp);
            if (it != end() && it->first == key) {
                return it;
            }
            return end();
        }
        
        const_iterator find(const Key& key) const {
            auto it = lower_bound(begin(), end(), key, key_comp);
            if (it != end() && it->first == key) {
                return it;
            }
            return end();
        }
        
        __forceinline Type& operator[](const Key& key) __pure {
            auto it = lower_bound(begin(), end(), key, key_comp);
            assert(it != end());
            return it->second;
        }
        __forceinline const_reference operator[](const Key& key) const __pure {
            auto it = lower_bound(begin(), end(), key, key_comp);
            assert(it != end());
            return it->second;
        }
        __forceinline reference front() __pure {
            assert(_size > 0 && "Map is empty");
            return *this->__buffer()[0].template ptr<0>();
        }
        __forceinline const_reference front() const __pure {
            assert(_size > 0 && "Map is empty");
            return *this->__buffer()[0].template ptr<0>();
        }
        __forceinline reference back() __pure {
            assert(_size > 0 && "Map is empty");
            return *this->__buffer()[_size - 1].template ptr<0>();
        }
        __forceinline const_reference back() const __pure {
            assert(_size > 0 && "Map is empty");
            return *this->__buffer()[_size - 1].template ptr<0>();
        }

        iterator insert(const_reference value) {
            auto it = insert_at(value.first);
            construct_at(it, value);
            return it;
        }
        
        template<class... Args>
        iterator emplace(const key_type&& key, Args&&... args) {
            auto it = insert_at(key);
            construct_at(it, key, forward<Args>(args)...);
            return it;
        }
        
        reference push_back(const_reference value) {
            this->__ensure_capacity(_size + 1, _size);
            assert(back().first < value.first && "Key is not ascending");
            return *construct_at(this->__buffer()[_size++].template ptr<0>(), value);
        }
        
        template<class... Args>
        reference emplace_back(const key_type&& key, Args&&... args) {
            this->__ensure_capacity(_size + 1, _size);
            assert(back().first < key && "Key is not ascending");
            return *construct_at(this->__buffer()[_size++].template ptr<0>(), key, forward<Args>(args)...);
        }
        
        iterator erase(const_iterator pos) {
            assert(_size > 0 && "Map is empty");
            assert(pos >= begin() && pos < end() && "Invalid erase position");
            destroy_at(pos);
            iterator ins = (iterator)pos;
            move((iterator)pos + 1, end(), ins);
            // Destroy the moved-from duplicate at the old end
            _size--;
            destroy_at(end());
            return ins;
        }

        iterator erase(const key_type& key) {
            auto it = lower_bound(begin(), end(), key, key_comp);
            assert(it != end());
            assert(it->first == key);
            return erase(it);
        }
        
        __forceinline void pop_back() {
            assert(_size > 0 && "Map is empty");
            destroy_at(this->__buffer()[--_size].template ptr<0>());
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
        
        __forceinline int capacity() const __pure {
            return this->__capacity();
        }

        void reserve(int new_cap) requires (Count == 0) {
            this->__ensure_capacity(new_cap, _size);
        }

    private:
        static constexpr auto comp = [](const_reference a, const_reference b)->bool { return a.first < b.first; };
        static constexpr auto key_comp = [](const_reference a, const key_type& b)->bool { return a.first < b; };

        iterator insert_at(const key_type& key) {
            this->__ensure_capacity(_size + 1, _size);
            auto it = lower_bound(begin(), end(), key, key_comp);
            if (it != end()) {
                if (it->first == key) {
                    destroy_at(it);
                } else {
                    uninitialized_move(end() - 1, end(), end());
                    move_backward(it, end() - 1, end());
                    ++_size;
                }
            } else {
                ++_size;
            }
            return it;
        }
        void sort_keys() {
            sort(begin(), end(), comp);
        }
        int _size;
    };
    
}
