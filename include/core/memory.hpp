//
//  memory.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-16.
//

#pragma once

#include "core/pool_allocator.hpp"

namespace toybox {

    /*
     This file contains a minimal set of functionality from C++ stdlib.
     */

    namespace detail {
        
        template<typename T>
        class  basic_ptr_c {
        public:
            using element_type = T;
            basic_ptr_c(T* ptr = nullptr) : _ptr(ptr) {}
            
            T* get() const __pure { return _ptr; }
            
            
            __forceinline T* operator->() const __pure  { return _ptr; }
            __forceinline T& operator*() const __pure  { return *(_ptr); }
            __forceinline T& operator[](int i) const __pure  { return _ptr[i]; }
            __forceinline T* operator+(int32_t i) const __pure  { return _ptr + i; }
            
            __forceinline explicit operator bool() const __pure  { return _ptr != nullptr; }
            __forceinline bool operator==(const basic_ptr_c& o) const __pure { return _ptr == o._ptr; }
            __forceinline bool operator==(T* o) const __pure { return _ptr == o; }

        protected:
            T* _ptr;
        };
        static_assert(sizeof(basic_ptr_c<void*>) == sizeof(void*), "basic_ptr_c size mismatch.");

    }
        
    static_assert(!is_polymorphic<nocopy_c>::value);
    template<typename T>
    class  unique_ptr_c : public detail::basic_ptr_c<T>, public nocopy_c {
    public:
        unique_ptr_c(T* ptr = nullptr) : detail::basic_ptr_c<T>(ptr) {}
        ~unique_ptr_c() { cleanup(); }

        unique_ptr_c(unique_ptr_c&& o) {
            this->_ptr = o._ptr;
            o._ptr = nullptr;
        }
        unique_ptr_c& operator=(unique_ptr_c&& o) {
            cleanup();
            this->_ptr = o._ptr;
            o._ptr = nullptr;
            return *this;
        }

        void reset(T* p = nullptr) {
            if (this->_ptr != p) cleanup();
            this->_ptr = p;
        }
        
    private:
        void cleanup() {
            if (this->_ptr) {
                delete this->_ptr;
                this->_ptr = nullptr;
            }
        }
    };
    static_assert(sizeof(unique_ptr_c<void*>) == sizeof(void*), "unique_ptr_c size mismatch.");

    namespace detail {
        struct shared_count_t {
            shared_count_t() : count(1) {}
            uint16_t count;
            void* operator new(size_t count) {
                assert(allocator::alloc_size >= count && "Allocation size exceeds allocator capacity");
                return allocator::allocate();
            }
            void operator delete(void* ptr) {
                allocator::deallocate(ptr);
            }
            using allocator = pool_allocator_c<shared_count_t, 256>;
        };
    }
    
    template<typename T>
    class shared_ptr_c : public detail::basic_ptr_c<T> {
    public:
        shared_ptr_c(T* ptr = nullptr) : detail::basic_ptr_c<T>(ptr), _count(ptr ? new detail::shared_count_t() : nullptr) {}
        ~shared_ptr_c() { cleanup(); }
        shared_ptr_c(const shared_ptr_c& o) : detail::basic_ptr_c<T>(o._ptr), _count(nullptr) {
            take_count(o._count);
        }
        shared_ptr_c(shared_ptr_c&& o) : detail::basic_ptr_c<T>(o._ptr), _count(o._count) {
            o._ptr = nullptr;
            o._count = nullptr;
        }
        shared_ptr_c& operator=(const shared_ptr_c& o) {
            if (this->_ptr != o._ptr) {
                cleanup();
                this->_ptr = o._ptr;
                take_count(o._count);
            }
            return *this;
        }
        shared_ptr_c& operator=(shared_ptr_c&& o) {
            if (this->_ptr != o._ptr) {
                cleanup();
            }
            this->_ptr = o._ptr;
            this->_count = o._count;
            o._ptr = nullptr;
            o._count = nullptr;
            return *this;
        }

        uint16_t use_count() const __pure { return _count != nullptr ? _count->count : 0; }
        void reset(T* p = nullptr) {
            if (this->_ptr != p) cleanup();
            this->_ptr = p;
            _count = p ? new detail::shared_count_t() : nullptr;
        }
        
    private:
        detail::shared_count_t* _count;
        void cleanup() {
            if (_count) {
                _count->count--;
                if (_count->count == 0) {
                    delete this->_ptr;
                    delete _count;
                    this->_ptr = nullptr;
                    _count = nullptr;
                }
            }
        }
        void take_count(detail::shared_count_t* count) {
            this->_count = count;
            if (this->_count) {
                this->_count->count++;
            }
        }
    };
    static_assert(sizeof(shared_ptr_c<void*>) == sizeof(void*) * 2, "shared_ptr_c size mismatch.");

    template<typename U, typename T>
    requires has_virtual_destructor<T>::value
    shared_ptr_c<U>& static_pointer_cast(shared_ptr_c<T>& r) {
        return static_cast<shared_ptr_c<U>&>(r);
    }
    template<typename U, typename T>
    requires has_virtual_destructor<T>::value
    const shared_ptr_c<U>& static_pointer_cast(const shared_ptr_c<T>& r) {
        return static_cast<const shared_ptr_c<U>&>(r);
    }
    template<typename U, typename T>
    requires has_virtual_destructor<T>::value
    shared_ptr_c<U>& reinterpret_pointer_cast(shared_ptr_c<T>& r) {
        return *reinterpret_cast<shared_ptr_c<U>*>(&r);
    }
    template<typename U, typename T>
    requires has_virtual_destructor<T>::value
    const shared_ptr_c<U>& reinterpret_pointer_cast(const shared_ptr_c<T>& r) {
        return *reinterpret_cast<const shared_ptr_c<U>*>(&r);
    }
    template<typename U, typename T>
    requires is_same<typename remove_cv<U>::type, typename remove_cv<T>::type>::value
    shared_ptr_c<U>& const_pointer_cast(const shared_ptr_c<T>& r) {
        return const_cast<shared_ptr_c<U>&>(r);
    }

}
