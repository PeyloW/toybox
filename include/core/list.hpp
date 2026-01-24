//
//  list.hpp
//  toybox
//
//  Created by Fredrik Olsson on 2024-03-24.
//

#pragma once

#include "core/pool_allocator.hpp"
#include "core/utility.hpp"

namespace toybox {
    
    /**
     A singly-linked forward list with statically allocated node storage.
     Similar to `std::forward_list` but uses a fixed-size allocator for O(1) allocation
     with no heap overhead. All node allocations come from a pool of `Count` pre-reserved slots.
     */
    template<class Type, size_t Count = 16>
    class list_c {
    public:
        using value_type = Type;
        using pointer = value_type* ;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;

        struct detail {
            struct node_s {
                node_s* next;
                value_type value;
                template<class... Args>
                node_s(node_s* next, Args&&... args) : next(next), value(forward<Args>(args)...) {}
                ~node_s() = default;
                void* operator new(size_t count) {
                    assert(allocator::alloc_size >= count && "Allocation size exceeds allocator capacity");
                    return allocator::allocate();
                }
                void operator delete(void* ptr) {
                    allocator::deallocate(ptr);
                }
            };

            template<class TypeI>
            struct iterator_s {
                using value_type = TypeI;
                using pointer = value_type*;
                using reference = value_type&;

                iterator_s() = delete;
                iterator_s(const iterator_s& o) = default;
                iterator_s(node_s* node) : _node(node) {}
                iterator_s(const iterator_s<Type> &other) requires (!same_as<Type, TypeI>) : _node(other._node) {}

                __forceinline reference operator*() const { return _node->value; }
                __forceinline pointer operator->() const { return &_node->value; }
                __forceinline iterator_s& operator++() { _node = _node->next; return *this; }
                __forceinline iterator_s operator++(int) { auto tmp = *this; _node = _node->next; return tmp; }
                __forceinline bool operator==(const iterator_s& o) const { return _node == o._node; }

                node_s* _node;
            };
        };

        using allocator = pool_allocator_c<typename detail::node_s, Count>;
        using iterator = typename detail::template iterator_s<Type>;
        using const_iterator = typename detail::template iterator_s<const Type>;
        
        list_c() { _head = nullptr; }
        list_c(const list_c& o) requires (Count == 0) : _head(nullptr) {
            if (o._head) _copy_from(o);
        }
        list_c(list_c&& o) requires (Count == 0) : _head(o._head) {
            o._head = nullptr;
        }
        ~list_c() { clear(); }

        list_c& operator=(const list_c& o) requires (Count == 0) {
            if (this == &o) return *this;
            clear();
            if (o._head) _copy_from(o);
            return *this;
        }
        list_c& operator=(list_c&& o) requires (Count == 0) {
            if (this == &o) return *this;
            clear();
            _head = o._head;
            o._head = nullptr;
            return *this;
        }

        __forceinline bool empty() const __pure { return _head == nullptr; }
        void clear() {
            auto it = before_begin();
            while (it._node->next) {
                erase_after(it);
            }
        }
        
        __forceinline reference front() __pure { return _head->value; }
        __forceinline const_reference front() const __pure { return _head->value; }

        /// Returns iterator to position before the first element. Required for insert/erase operations.
        iterator before_begin() __pure {
            using node_s = detail::node_s;
            auto before_head = const_cast<node_s**>(&_head);
            return iterator(launder(reinterpret_cast<node_s*>(before_head)));
        }
        /// Returns const iterator to position before the first element. Required for insert/erase operations.
        const_iterator before_begin() const __pure {
            using node_s = detail::node_s;
            auto before_head = const_cast<node_s**>(&_head);
            return const_iterator(launder(reinterpret_cast<node_s*>(before_head)));
        }
        __forceinline iterator begin() __pure { return iterator(_head); }
        __forceinline const_iterator begin() const __pure { return const_iterator(_head); }
        __forceinline iterator end() __pure { return iterator(nullptr); }
        __forceinline const_iterator end() const __pure { return const_iterator(nullptr); }

        /// Returns number of elements. O(n) operation as list must be traversed.
        int size() const __pure {
            int count = 0;
            auto it = before_begin();
            while (it._node->next) {
                ++count;
                ++it;
            }
            return count;
        }
        
        __forceinline void push_front(const_reference value) {
            insert_after(before_begin(), value);
        }
        template<class ...Args>
        __forceinline reference emplace_front(Args&& ...args) {
            return *emplace_after(before_begin(), forward<Args>(args)...);
        }
        iterator insert_after(const_iterator pos, const_reference value) {
            using node_s = detail::node_s;
            assert(owns_node(pos._node) && "Node not owned by this list");
            pos._node->next = new node_s{pos._node->next, value};
            return iterator(pos._node->next);
        }
        template<class ...Args>
        iterator emplace_after(const_iterator pos, Args&& ...args) {
            using node_s = detail::node_s;
            assert(owns_node(pos._node) && "Node not owned by this list");
            pos._node->next = new node_s(pos._node->next, forward<Args>(args)...);
            return iterator(pos._node->next);
        }
        __forceinline void pop_front() {
            erase_after(before_begin());
        }
        iterator erase_after(const_iterator pos) {
            assert(owns_node(pos._node) && "Node not owned by this list");
            auto tmp = pos._node->next;
            pos._node->next = tmp->next;
            delete tmp;
            return iterator(pos._node->next);
        }
        /**
         Moves the element after `it` from `other` to after `pos` in this list.
         No copy or move constructors are called. O(1) operation.
         */
        void splice_after(const_iterator pos, list_c& other, const_iterator it) {
            assert(owns_node(pos._node) && "Node not owned by this list");
            assert(other.owns_node(it._node) && "Node not owned by other list");

            auto tmp = it._node->next;          // Element to splice
            it._node->next = tmp->next;         // Remove from source
            tmp->next = pos._node->next;        // Link to destination's next
            pos._node->next = tmp;              // Link destination to spliced element
        }
        /**
         Moves elements in range (first, last) from `other` to after `pos` in this list.
         No copy or move constructors are called. O(n) operation where n is distance from first to last.
         */
        void splice_after(const_iterator pos, list_c& other, const_iterator first, const_iterator last) {
            assert(owns_node(pos._node) && "Node not owned by this list");
            assert(other.owns_node(first._node) && "First iterator not owned by other list");
            assert((last._node == nullptr || other.owns_node(last._node)) && "Last iterator not owned by other list");
            if (first._node->next == last._node) return;
            auto before_last = first._node;
            while (before_last->next != last._node) {
                before_last = before_last->next;
            }
            auto range_first = first._node->next;     // First element in range to splice
            first._node->next = last._node;           // Remove range from source
            before_last->next = pos._node->next;      // Link range end to dest
            pos._node->next = range_first;            // Link dest to range start
        }
        /// Convenience wrapper for splicing a single element to the front of this list.
        void splice_front(list_c& other, const_iterator it) {
            splice_after(before_begin(), other, it);
        }
        /// Convenience wrapper for splicing a range of elements to the front of this list.
        void splice_front(list_c& other, const_iterator first, const_iterator last) {
            splice_after(before_begin(), other, first, last);
        }
    private:
        void _copy_from(const list_c& o) {
            using node_s = detail::node_s;
            node_s* src = o._head;
            node_s** dst_ptr = &_head;

            while (src) {
                *dst_ptr = new node_s{nullptr, src->value};
                dst_ptr = &(*dst_ptr)->next;
                src = src->next;
            }
        }

        bool owns_node(detail::node_s* node) const __pure {
            using node_s = detail::node_s;
            auto before_head = const_cast<node_s**>(&_head);
            auto n = reinterpret_cast<node_s*>(before_head);
            while (n) {
                if (n == node) return true;
                n = n->next;
            }
            return false;;
        }
        detail::node_s* _head;
    };
    
}
