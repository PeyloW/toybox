//
//  utility.hpp
//  toybox
//
//  Created by Fredrik on 2024-03-22.
//

#pragma once

#include "core/concepts.hpp"

namespace toybox {
    
    /*
     This file contains a minimal set of functionality from C++ stdlib.
     */

#pragma mark - Byte order helpers
    
#ifdef __M68000__
#   define hton(v)
#else
    template<arithmetic Type>
    requires (sizeof(Type) == 1)
    void hton(Type& value) { }

    template<arithmetic Type>
    requires (sizeof(Type) == 2)
    __forceinline void hton(Type& value) { value = htons(value); }

    template<arithmetic Type>
    requires (sizeof(Type) == 4)
    __forceinline void hton(Type& value) { value = htonl(value); }

    void hton_struct(void* ptr, const char* layout);
    template<class_type T>
    __forceinline void hton(T& value) {
        hton_struct(&value, struct_layout<T>::value);
    }

    template<class Type>
    void hton(Type* buf, size_t count) {
        while (count--) {
            hton(*buf);
            buf++;
        }
    }
    template<class Type, size_t Count>
    __forceinline void hton(Type (&array)[Count]) {
        hton(&array[0], Count);
    }
#endif
    
#pragma mark - Math functions
    
    static inline int sqrti(int x) {
        if (x == 0 || x == 1) return x;
        // Binary search for sqrt(x)
        int low = 1, high = x, result = 1;
        while (low <= high) {
            int mid = low + ((high - low) >> 1);
            int square = mid * mid;
            if (square == x) {
                return mid;
            } else if (square < x) {
                low = mid + 1;
                result = mid;
            } else {
                high = mid - 1;
            }
        }
        return result;
    }

    template<ordered T>
    __forceinline constexpr const T& max(const T& a) {
        return a;
    }
    template<ordered T, typename... Ts>
    __forceinline constexpr const T& max(const T& a, const T& b, const Ts&... rest) {
        const T& m = (a < b) ? b : a;
        return max(m, rest...);
    }
    
    template<ordered T>
    __forceinline constexpr const T& min(const T& a) {
        return a;
    }
    template<ordered T, typename... Ts>
    __forceinline constexpr const T& min(const T& a, const T& b, const Ts&... rest) {
        const T& m = (a < b) ? a : b;
        return min(m, rest...);
    }

#pragma mark - Random numbers
    
    static uint16_t fast_rand_seed = 0xace1u;
    
    static __forceinline uint16_t fast_rand(uint16_t seed) {
        assert(seed != 0 && "Seed must be non-zero");
    #ifdef __m68k__
        asm volatile (
            "lsr.w   #1,%0       \n\t"
            "bcc.s   0f          \n\t"
            "eori.w  #0xB400,%0  \n\t"
            "0:                  \n\t"
            : "+d"(seed)             // in/out in same register
            :                        // no extra inputs
            : "cc"                   // condition codes clobbered
        );
    #else
        uint16_t lsb = seed & 1u;
        seed >>= 1;
        if (lsb) seed ^= 0xB400u;
    #endif
        return seed;
    }

    static __forceinline uint16_t fast_rand() {
        return (fast_rand_seed = fast_rand(fast_rand_seed));
    }
    
    /**
     Blue noise random number series with 256 index repeat. 
     Number are in range 0..63 inclusive.
     */
    static __forceinline int brand(int idx) {
        static constexpr uint8_t s_blue[256] = {
            10,  2, 17, 23, 10, 34,  4, 28, 37,  2, 19,  7,  3,  1,  5, 62,
             0,  8, 55, 46,  1, 61, 19,  0,  1,  9, 45, 25, 34, 16,  0, 24,
            13, 28,  5,  0,  3, 26, 12,  6, 42, 14, 59,  0, 11,  2, 50, 42,
             1, 37, 20, 14, 32,  8,  2, 53, 22,  3,  0, 29,  4, 56, 18,  3,
             0, 57,  2,  6, 50,  0, 38,  0, 31,  8, 17, 39,  1,  6, 32,  9,
            47, 23, 10,  1, 42, 21, 16,  4, 58,  5, 48,  2, 13, 21,  0, 15,
             5, 35,  0, 63,  4,  1, 28, 11,  0,  1, 25,  9, 62, 27, 41,  2,
             8, 29, 18, 12, 26,  7, 53, 14, 43, 19, 36,  0,  4,  0, 54,  1,
            51,  0,  3,  0, 39,  2,  0, 34,  6,  3, 10, 46, 16,  6, 12, 20,
            40, 14, 57, 22, 47, 10,  1, 59, 24,  0, 52, 30,  2, 36, 26,  0,
             9, 32,  2,  6, 16, 31, 20,  4,  1, 15,  7, 21,  1, 60,  7,  3,
             0,  4, 45,  1,  0,  3, 49, 12, 35, 55,  0,  4, 13,  0, 49, 17,
            58, 27, 11, 36, 61,  8, 18,  0, 40, 27,  9, 44, 33,  5, 38, 23,
             0,  7, 19,  0, 24, 29,  2,  0,  6,  3,  1, 17, 25,  2, 11,  1,
            43, 52,  3, 13,  5, 54, 44, 11, 22, 63, 31,  0, 56,  8,  0, 15,
             4, 33,  0, 41,  1,  7,  0, 15, 51,  5,  0, 12, 48, 39, 21, 30
        };
        return s_blue[idx & 0xff];
    };
    
#pragma mark - Hashing
    
    static inline uint16_t fletcher16(uint8_t* data, size_t count, uint16_t start = 0) {
        uint16_t sum1 = start & 0xff;
        uint16_t sum2 = (start >> 8) & 0xff;
        while (count--) {
           sum1 = (sum1 + *data++) % 255;
           sum2 = (sum2 + sum1) % 255;
        }
        return (sum2 << 8) | sum1;
    }
    
#pragma mark - Type packs
    
    namespace detail {
        template<int I, typename T0, typename... Ts>
        struct type_at_impl : type_at_impl<I - 1, Ts...> {};
        template<typename T0, typename... Ts>
        struct type_at_impl<0, T0, Ts...> { using type = T0; };
        
        template<typename T, int I, typename... Ts>
        struct index_of_impl;
        template<typename T, int I>
        struct index_of_impl<T, I> {
            static constexpr int value = -1;
        };
        template<typename T, int I, typename Current, typename... Rest>
        struct index_of_impl<T, I, Current, Rest...> {
            static constexpr int value = is_same<T, Current>::value ? I : index_of_impl<T, I + 1, Rest...>::value;
        };
    }

    template<int I, typename... Ts>
    struct type_at {
        static_assert(I >= 0 && I < static_cast<int>(sizeof...(Ts)), "type_at: index out of bounds");
        using type = typename detail::type_at_impl<I, Ts...>::type;
    };
    
    template<typename T, typename... Ts>
    struct index_of {
        static constexpr int value = detail::index_of_impl<T, 0, Ts...>::value;
        static_assert(value != -1, "index_of: type not found in parameter pack");
    };
    
#pragma mark - Relational operators
    
    namespace rel_ops {
        
        template<equality_comparable T>
        constexpr bool operator!=( const T& lhs, const T& rhs ) { return !(lhs == rhs); }
        
        template<less_than_comparable T>
        constexpr bool operator>( const T& lhs, const T& rhs ) { return rhs < lhs; }
        
        template<less_than_comparable T>
        constexpr bool operator<=( const T& lhs, const T& rhs ) { return !(rhs < lhs); }
        
        template<less_than_comparable T>
        constexpr bool operator>=( const T& lhs, const T& rhs ) { return !(lhs < rhs); }
        
    }
    
#pragma mark - C++ language helpers
    
    template<class C> __forceinline auto begin(C& c) -> decltype(c.begin()) { return c.begin(); };
    template<class C> __forceinline auto begin(const C& c) -> decltype(c.begin()) { return c.begin(); };
    template<class T, size_t N> __forceinline T* begin(T (&array)[N]) { return &array[0]; };
    template<class C> __forceinline auto end(C& c) -> decltype(c.end()) { return c.end(); };
    template<class C> __forceinline auto end(const C& c) -> decltype(c.end()) { return c.end(); };
    template<class T, size_t N> __forceinline T* end(T (&array)[N]) { return &array[N]; };

    template<typename T> constexpr T&& forward(typename remove_reference<T>::type& t) {
        return static_cast<T&&>(t);
    }
    template<typename T> constexpr T&& forward(typename remove_reference<T>::type&& t) {
        return static_cast<T&&>(t);
    }
    template<typename T> constexpr typename remove_reference<T>::type&& move(T&& t) {
        return static_cast<typename remove_reference<T>::type&&>(t);
    }
    
    template<typename T>
    void swap(T& a, T& b) {
        T t = move(a);
        a = move(b);
        b = move(t);
    }

    template<class T, class... Args>
    constexpr T* construct_at(T* p, Args&&... args) {
        if constexpr (is_trivially_constructible<T, Args...>::value) {
            *p = T(forward<Args>(args)...);
            return p;
        } else {
            return new (static_cast<void*>(p)) T(forward<Args>(args)...);
        }
    }
    template<class T> __forceinline void destroy_at(T* p) {
        if constexpr (!is_trivially_destructible<T>::value) {
            p->~T();
        }
    }
    
    template<forward_iterator I>
    void destroy(I first, I last) {
        if constexpr (!is_trivially_destructible<typename iterator_traits<I>::value_type>::value) {
            while (first != last) {
                destroy_at(&*first);
                ++first;
            }
        }
    }
    
    template <class T>
    constexpr T* launder(T* p) {
        return __builtin_launder(p);
    }
    
#if TOYBOX_HOST
    namespace detail {
        static constexpr int32_t MAX_PTRS = 4096;
        int32_t idx;
        static int32_t register_pointer(void* p) {
            if (p == nullptr) return 0;
            for (int32_t i = 1; i < s_next_idx; ++i) {
                if (s_pointers[i] == p) return i;
            }
            hard_assert(s_next_idx < MAX_PTRS && "short_ptr_c pool exhausted");
            s_pointers[s_next_idx] = p;
            return s_next_idx++;
        }
        static inline void* s_pointers[MAX_PTRS] = {};
        static inline int32_t s_next_idx = 1;  // 0 reserved for nullptr
    }
#endif
    
    template<pointer_type P, typename T>
    requires (sizeof(T) == 4)
    P to_pointer_cast(T o) {
#if TOYBOX_HOST
        return detail::s_pointers[reinterpret_cast<int>(o)];
#else
        return reinterpret_cast<P>(o);
#endif
    }

    template<typename T, pointer_type P>
    requires (sizeof(T) == 4)
    T from_pointer_cast(P p) {
#if TOYBOX_HOST
        return detail::register_pointer(reinterpret_cast<void*>(p));
#else
        return reinterpret_cast<T>(p);
#endif
    }

#pragma mark - Helper classes

    // Lightweight non-owning function reference, similar to std::function_ref.
    // Client must ensure referenced callable outlives function_c
    template<typename>
    class function_c;

    template<typename R, typename... Args>
    class function_c<R(Args...)> {
    public:
        constexpr function_c() : _invoker(nullptr), _target(nullptr) {}
        constexpr function_c(R(*func)(Args...))
            : _invoker(invoke_func_ptr), _target(reinterpret_cast<void*>(func))
        {}
        template<typename F>
        constexpr function_c(F& func)
            : _invoker(invoke_functor<F>), _target(static_cast<void*>(&func))
        {}
        
        __forceinline R operator()(Args... args) const {
            return _invoker(_target, static_cast<Args&&>(args)...);
        }
        
        __forceinline operator bool() const { return _invoker != nullptr; }
    private:
        // Type-erased invoke function
        using invoke_ptr_t = R(*)(void*, Args...);
        static R invoke_func_ptr(void* obj, Args... args) {
            auto func = reinterpret_cast<R(*)(Args...)>(obj);
            return func(static_cast<Args&&>(args)...);
        }
        template<typename F>
        static R invoke_functor(void* obj, Args... args) {
            const auto& func = *static_cast<F*>(obj);
            return func(static_cast<Args&&>(args)...);
        }
        invoke_ptr_t _invoker;
        void* _target;
    };
    
    // Base class enforcing no copy constructor or assignment.
    class nocopy_c {
    public:
        __forceinline bool operator==(const nocopy_c& other) const {
            return this == &other;
        }
    protected:
        constexpr nocopy_c() {}
        nocopy_c(const nocopy_c&) = delete;
        nocopy_c& operator=(const nocopy_c&) = delete;
    };

    template<typename... Ts>
    struct aligned_membuf_s {
        static constexpr size_t size  = max(sizeof(Ts)...);
        static constexpr size_t align = max(alignof(Ts)...);
        alignas(align) uint8_t data[size] = {0};
        void* addr() { return &data; }
        const void* addr() const { return &data; }
        template<int I = 0>
        typename type_at<I, Ts...>::type* ptr() {
            return reinterpret_cast<typename type_at<I, Ts...>::type*>(&data);
        }
        template<int I = 0>
        const typename type_at<I, Ts...>::type* ptr() const {
            return reinterpret_cast<const typename type_at<I, Ts...>::type*>(&data);
        }
    };
    
    template<class T1, class T2>
    class pair_c {
    public:
        constexpr pair_c(const T1& f, const T2& s) : first(f), second(s) {}
        T1 first;
        T2 second;
    };

    struct empty_t {};

    template< class T1, class T2 >
    __forceinline pair_c<T1, T1> make_pair( T1&& f, T2&& s) { return pair_c<T1, T2>(forward<T1>(f), forward<T2>(s)); }

    template<integral Int>
    Int* create_multable(int count, Int mul) {
        Int* t = (Int*)_malloc(sizeof(Int) * count);
        for (int i = 0; i < count; i++) {
            t[i] = i * mul;
        }
        return t;
    }
    
}
