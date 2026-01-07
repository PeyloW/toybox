//
//  map.hpp
//  toybox
//
//  Created by Fredrik on 2025-12-28.
//

#pragma once

#include "core/algorithm.hpp"
#include "core/concepts.hpp"
#include "core/vector.hpp"

namespace toybox {
    
    namespace detail {
        /// Default comparator using < operator.
        struct default_less_t {
            template<class T>
            __forceinline constexpr bool operator()(const T& a, const T& b) const {
                return a < b;
            }
        };
        
        /// Key extractor for pair-like types with .first member.
        struct first_key_get_t {
            template<class T>
            __forceinline constexpr auto operator()(const T& v) const -> decltype(v.first) {
                return v.first;
            }
        };

        /// Key extractor for types with .id member.
        struct id_key_get_t {
            template<class T>
            __forceinline constexpr auto operator()(const T& v) const -> decltype(v.id) {
                return v.id;
            }
        };
    }
    
    /**
     `map_c` is minimal implementation of `std::flat_map` providing associative access to elements
     of a sorted underlying container. Instead of using key + value containers a single container
     with a get key provider is used.
     
     Container can be a value type (owning) or reference type (non-owning).
     Mutating operations are only available when Container supports them.
     */
    template<class Container, class KeyGet, class KeyCompare = detail::default_less_t>
    requires container<typename remove_reference<Container>::type>
    class map_c {
    private:
        using container_type = typename remove_reference<Container>::type;
        static constexpr bool is_owning = !is_reference<Container>::value;
        static constexpr bool is_const_ref = is_reference<Container>::value && is_const<container_type>::value;
        static constexpr bool is_mutable = mutable_container<container_type> && !is_const_ref;
        static constexpr bool is_copyable = !is_owning || is_copy_constructible<container_type>::value;

    public:
        using value_type = typename container_type::value_type;
        using key_type = decltype(declval<KeyGet>()(declval<const value_type&>()));
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;

        static constexpr KeyGet key_get{};
        static constexpr KeyCompare key_compare{};

        // Constructors - owning
        map_c() requires is_owning = default;
        explicit map_c(container_type&& c) requires is_owning : _container(move(c)) {}

        // Constructor - reference
        explicit map_c(container_type& c) requires (!is_owning) : _container(c) {}

        // Copy operations (enabled for references or copyable containers)
        map_c(const map_c& o) requires is_copyable = default;
        map_c& operator=(const map_c& o) requires is_copyable = default;

        // Move operations
        map_c(map_c&& o) = default;
        map_c& operator=(map_c&& o) = default;
        
        // Iteration - non-const (only for non-const containers)
        __forceinline iterator begin() requires (!is_const_ref) { return _container.begin(); }
        __forceinline iterator end() requires (!is_const_ref) { return _container.end(); }
        __forceinline pointer data() requires (!is_const_ref) { return _container.data(); }

        // Iteration - const
        __forceinline const_iterator begin() const { return _container.begin(); }
        __forceinline const_iterator end() const { return _container.end(); }
        __forceinline const_pointer data() const { return _container.data(); }
        __forceinline int size() const { return _container.size(); }

        // Element access - non-const (only for non-const containers)
        __forceinline reference front() requires (!is_const_ref) { return _container.front(); }
        __forceinline reference back() requires (!is_const_ref) { return _container.back(); }

        // Element access - const
        __forceinline const_reference front() const { return _container.front(); }
        __forceinline const_reference back() const { return _container.back(); }

        // Lookup - non-const (only for non-const containers)
        iterator find(const key_type& key) requires (!is_const_ref) {
            auto it = lower_bound(begin(), end(), key, value_key_comp());
            if (it != end() && key_get(*it) == key) return it;
            return end();
        }

        // Lookup - const
        const_iterator find(const key_type& key) const {
            auto it = lower_bound(begin(), end(), key, value_key_comp());
            if (it != end() && key_get(*it) == key) return it;
            return end();
        }

        // operator[] - non-const (only for non-const containers)
        reference operator[](const key_type& key) requires (!is_const_ref) {
            auto it = find(key);
            assert(it != end() && "Key not found");
            return *it;
        }

        // operator[] - const
        const_reference operator[](const key_type& key) const {
            auto it = find(key);
            assert(it != end() && "Key not found");
            return *it;
        }

        bool contains(const key_type& key) const {
            return find(key) != end();
        }
        
        // Mutating operations - append (caller ensures ascending order)
        reference push_back(const_reference value) requires is_mutable {
            assert(size() == 0 || key_compare(key_get(back()), key_get(value))
                   && "Key must be ascending");
            _container.push_back(value);
            return back();
        }
        
        template<class... Args>
        reference emplace_back(Args&&... args) requires is_mutable {
            auto& ref = _container.emplace_back(forward<Args>(args)...);
            assert(size() <= 1 || key_compare(key_get(*(&ref - 1)), key_get(ref))
                   && "Key must be ascending");
            return ref;
        }
        
        // Mutating operations - sorted insert
        iterator insert(const_reference value) requires is_mutable {
            auto it = lower_bound(begin(), end(), key_get(value), value_key_comp());
            return _container.insert(it, value);
        }
        
        template<class... Args>
        iterator emplace(Args&&... args) requires is_mutable {
            value_type temp{forward<Args>(args)...};
            auto it = lower_bound(begin(), end(), key_get(temp), value_key_comp());
            return _container.emplace(it, move(temp));
        }
        
        // Mutating operations - erase
        iterator erase(const_iterator pos) requires is_mutable {
            return _container.erase(pos);
        }
        
        iterator erase(const key_type& key) requires is_mutable {
            auto it = find(key);
            assert(it != end() && "Key not found");
            return _container.erase(it);
        }
        
        void pop_back() requires is_mutable {
            _container.pop_back();
        }
        
        void clear() requires is_mutable {
            _container.clear();
        }
        
        // Forwarded capacity operations
        int capacity() const
        requires requires(const container_type& c) { { c.capacity() } -> convertible_to<int>; }
        {
            return _container.capacity();
        }
        
        void reserve(int n)
        requires requires(container_type& c, int n) { c.reserve(n); }
        {
            _container.reserve(n);
        }
        
    private:
        // Comparator: compares value's key against a raw key
        static constexpr auto value_key_comp() {
            return [](const_reference v, const key_type& k) {
                return key_compare(key_get(v), k);
            };
        }
        
        Container _container;
    };
    
    /// Convenience alias for map with pair_c<Key, Value> elements and static capacity.
    template<class Key, class Value, int Count>
    using pair_map_c = map_c<vector_c<pair_c<Key, Value>, Count>, detail::first_key_get_t>;

    /// Convenience alias for map with Value elements having .id member as key.
    template<class Value, int Count>
    using id_map_c = map_c<vector_c<Value, Count>, detail::id_key_get_t>;

} // namespace toybox

