//
//  sparse_vector.hpp
//  toybox
//
//  Created by Fredrik on 2026-02-05.
//

#pragma once

#include "core/vector.hpp"

namespace toybox {
 
    namespace detail {
        template<typename T>
        struct identifier {};
                
        template<typename T>
        requires requires(T t) { { t.first } -> toybox::convertible_to<int>; }
        struct identifier<T> {
            auto& operator()(T& p) const { return p.first; }
            auto operator()(const T& p) const { return p.first; }
        };
        
        template<typename T>
        requires requires(T t) { { t.id } -> toybox::convertible_to<int>; }
        struct identifier<T> {
            auto& operator()(T& t) const { return t.id; }
            auto operator()(const T& t) const { return t.id; }
        };
        
        template<typename T>
        concept is_identifier = requires(T t, identifier<T> id) {
            { id(t) } -> toybox::convertible_to<int>;
        };
        
    }

    /**
     `sparse_vector_c` is a sparse vector with O(1) access, insert, and erase.
     The indexed order of values is not guaranteed to be the same as the
     iteration order of values. Highest index is guaranteed to be less than Count.
     */
    template<typename Type, int Count>
    class sparse_vector_c : public nocopy_c {
    public:
        static constexpr int max_size = vector_c<Type, Count>::max_size;
        using value_type = Type;
        using pointer = value_type* ;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using iterator = typename vector_c<Type, Count>::iterator;
        using const_iterator = typename vector_c<Type, Count>::const_iterator;
        
        constexpr sparse_vector_c() = default;
        constexpr sparse_vector_c(initializer_list<Type> init) : _values(init) {
            _to_dense.resize((int)init.size());
            if constexpr (!detail::is_identifier<Type>) {
                _to_sparse.resize((int)init.size());
            }
            for (int i = 0; i < init.size(); i++) {
                __assume_count(i, max_size);
                _to_dense[i] = i;
                if constexpr (detail::is_identifier<Type>) {
                    set_sparse_key_at(i, i);
                } else {
                    _to_sparse[i] = i;
                }
            }
        }
                        
        __forceinline auto begin() __pure { return _values.begin(); }
        __forceinline auto end() __pure { return _values.end(); }
        __forceinline auto begin() const __pure { return _values.begin(); }
        __forceinline auto end() const __pure { return _values.end(); }
        __forceinline auto data() { return _values.data(); }
        __forceinline auto data() const { return _values.data(); }
        __forceinline int size() const { return _values.size(); }
        
        __forceinline reference operator[](int key) __pure {
            __assume_count(key, max_size);
            return _values[_to_dense[key]];
        }
        
        __forceinline const_reference operator[](int key) const __pure {
            __assume_count(key, max_size);
            return _values[_to_dense[key]];
        }
        
        __forceinline reference front() __pure { return _values.front(); }
        __forceinline const_reference front() const __pure { return _values.front(); }
        __forceinline reference back() __pure { return _values.back(); }
        __forceinline const_reference back() const __pure { return _values.back(); }

        __forceinline int index_of(const_iterator pos) const __pure {
            assert(pos >= begin() && pos < end());
            if constexpr (detail::is_identifier<Type>) {
                return detail::identifier<Type>{}(*pos);
            } else {
                return sparse_key_at(pos - begin());
            }
        }

        int push_back(const_reference object) {
            const int key = alloc_sparse_key();
            _values.push_back(object);
            set_sparse_key_at(size() - 1, key);
            return key;
        }
        
        template<typename... Args>
        reference emplace_back(Args&&... args) {
            const int key = alloc_sparse_key();
            _values.emplace_back(forward<Args>(args)...);
            set_sparse_key_at(size() - 1, key);
            return _values.back();
        }
        
        void erase(int key) {
            __assume_count(key, max_size);
            const int idx      = _to_dense[key];
            const int last_idx = size() - 1;
            const int last_key = sparse_key_at(last_idx);
            
            swap(_values[idx], _values[last_idx]);
            swap(_to_dense[key], _to_dense[last_key]);
            
            if constexpr (detail::is_identifier<Type>) {
                _values.pop_back();
                _to_dense[key] = _free_head;
                _free_head = key;
            } else {
                swap(_to_sparse[idx], _to_sparse[last_idx]);
                _values.pop_back();
            }
        }
        
        __forceinline void erase(const_iterator pos) {
            assert(pos >= begin() && pos < end());
            const int i = pos - _values.begin();
            erase(sparse_key_at(i));
        }
        
        __forceinline void clear() {
            _values.clear();
            if constexpr (detail::is_identifier<Type>) {
                _free_head = no_free;
            }
        }
        
        __forceinline void reserve(int n) {
            _values.reserve(n);
            _to_dense.reserve(n);
            if constexpr (!detail::is_identifier<Type>) {
                _to_sparse.reserve(n);
            }
        }

    private:
        static constexpr int no_free = -1;
        
        // Helper to initialize _free_head properly
        static constexpr auto init_free_head() {
            if constexpr (detail::is_identifier<Type>) {
                return no_free;
            } else {
                return empty_t{};
            }
        }
        
        __forceinline int sparse_key_at(int dense_idx) const {
            __assume_count(dense_idx, max_size);
            if constexpr (detail::is_identifier<Type>) {
                return detail::identifier<Type>{}(_values[dense_idx]);
            } else {
                return _to_sparse[dense_idx];
            }
        }
        
        __forceinline void set_sparse_key_at(int dense_idx, int key) {
            __assume_count(dense_idx, max_size);
            __assume_count(key, max_size);
            if constexpr (detail::is_identifier<Type>) {
                detail::identifier<Type>{}(_values[dense_idx]) = key;
            } else {
                _to_sparse[dense_idx] = key;
            }
        }
        
        __forceinline int alloc_sparse_key() {
            if constexpr (detail::is_identifier<Type>) {
                if (_free_head != no_free) {
                    const int key = _free_head;
                    __assume_count(key, max_size);
                    _free_head = _to_dense[key];
                    __assume_count(size(), max_size);
                    _to_dense[key] = size();
                    return key;
                }
                const int key = _to_dense.size();
                _to_dense.push_back(size());
                return key;
            } else {
                if (_to_sparse.size() > size()) {
                    __assume_count(size(), max_size);
                    const int key = _to_sparse[size()];
                    __assume_count(key, max_size);
                    _to_dense[key] = size();
                    return key;
                }
                const int key = _to_sparse.size();
                __assume_count(key, max_size);
                _to_sparse.push_back(key);
                __assume_count(size(), max_size);
                _to_dense.push_back(size());
                return key;
            }
        }
        
        vector_c<Type, Count> _values;
        vector_c<int, Count>  _to_dense;
        
        [[no_unique_address]]
        typename conditional<detail::is_identifier<Type>, empty_t, vector_c<int, Count>>::type _to_sparse{};
        
        [[no_unique_address]]
        typename conditional<detail::is_identifier<Type>, int, empty_t>::type _free_head = init_free_head();

    };
    
}
