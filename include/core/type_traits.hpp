//
//  type_traits.hpp
//  toybox
//
//  Created by Fredrik Olsson on 2024-03-22.
//

#pragma once

#include "core/cincludes.hpp"

namespace toybox {
    
    /*
     This file contains a minimal set of functionality from C++ stdlib.
     */
        
    template<class T, T v>
    struct integral_constant {
        static constexpr T value = v;
    };
    template<bool B>
    struct bool_constant : public integral_constant<bool, B> {};
    using false_type = bool_constant<false>;
    using true_type = bool_constant<true>;
    
    
    template<bool B, class T = void>
    struct enable_if {};
     
    template<class T>
    struct enable_if<true, T> { typedef T type; };
    
#pragma mark - Type categories
    
    template<typename T> struct is_integral : public false_type {};
    template<> struct is_integral<bool> : public true_type {};
    template<> struct is_integral<int8_t> : public true_type {};
    template<> struct is_integral<uint8_t> : public true_type {};
    template<> struct is_integral<int16_t> : public true_type {};
    template<> struct is_integral<uint16_t> : public true_type {};
    template<> struct is_integral<int32_t> : public true_type {};
    template<> struct is_integral<uint32_t> : public true_type {};
    
    template<typename T> struct is_floating_point : public false_type {};
    template<> struct is_floating_point<float> : public true_type {};
    template<> struct is_floating_point<double> : public true_type {};
    
    template<typename T> struct is_arithmetic : public bool_constant<is_integral<T>::value || is_floating_point<T>::value> {};
    
    template<class T> struct is_pointer : false_type {};
    template<class T> struct is_pointer<T*> : true_type {};
    
    template<class T> struct is_reference : false_type {};
    template<class T> struct is_reference<T&> : true_type {};
    template<class T> struct is_reference<T&&> : true_type {};
    
    
#pragma mark - Type modifications
    
    template<class T> struct remove_const { using type = T; };
    template<class T> struct remove_const<const T> { using type = T; };
    
    template<class T> struct remove_reference { using type = T; };
    template<class T> struct remove_reference<T&> { using type = T; };
    template<class T> struct remove_reference<T&&> { using type = T; };
    
    template<class T> struct remove_volatile { using type = T; };
    template<class T> struct remove_volatile<volatile T> { using type = T; };
    
    template <typename T> struct remove_cv { using type = T; };
    template <typename T> struct remove_cv<const T> { using type = T; };
    template <typename T> struct remove_cv<volatile T> { using type = T; };
    template <typename T> struct remove_cv<const volatile T> { using type = T; };

    template <typename T>
    using remove_cvref = remove_cv<typename remove_reference<T>::type>;

    namespace detail {
        template<typename T, typename U = T&&> U declval_imp(int);
        template<typename T> T declval_imp(long);
    }
    template<typename T> auto declval() -> decltype(detail::declval_imp<T>(0));
        
#pragma mark - Relationship and property queries
    
    template<class T, class U> struct is_same : false_type {};
    template<class T> struct is_same<T, T> : true_type {};

    template<bool B, class T, class F>
    struct conditional { using type = T; };
    template<class T, class F>
    struct conditional<false, T, F> { using type = F; };

    template<class T>
    struct is_void : is_same<void, typename remove_const<T>::type> {};
    
    template<typename T> struct next_larger;
    template<> struct next_larger<uint8_t> { using type = uint16_t; };
    template<> struct next_larger<uint16_t> { using type = uint32_t; };
    template<> struct next_larger<uint32_t> { using type = uint64_t; };
    template<> struct next_larger<int8_t> { using type = int16_t; };
    template<> struct next_larger<int16_t> { using type = int32_t; };
    template<> struct next_larger<int32_t> { using type = int64_t; };

#pragma mark - Supported operations

    template<typename T, typename... Args>
    struct is_constructible : bool_constant<__is_constructible(T, Args...)> {};
    template<typename T, typename... Args>
    struct is_trivially_constructible : bool_constant<__is_trivially_constructible(T, Args...)> {};
    template<typename T>
    struct is_default_constructible : bool_constant<__is_constructible(T)> {};
    template<typename T>
    struct is_copy_constructible : bool_constant<__is_constructible(T, const T&)> {};
    template<typename T>
    struct is_move_constructible : bool_constant<__is_constructible(T, T&&)> {};
    
#if defined(__clang__)
    template<typename T>
    struct is_destructible : public bool_constant<__is_destructible(T)> {};
    template<typename T> struct is_trivially_destructible : bool_constant<__is_trivially_destructible(T)> {};
#else
    template<typename T>
    struct is_destructible {
    private:
        template<typename U> static auto test(int) -> decltype(declval<U&>().~U(), true_type{});
        template<typename> static false_type test(...);
    public:
        static constexpr bool value = decltype(test<T>(0))::value;
    };
    template<typename T> struct is_trivially_destructible : bool_constant<is_destructible<T>::value && __has_trivial_destructor(T)> {};
#endif
    
    template<typename T> struct is_trivially_copyable : public bool_constant<__is_trivially_copyable(T)> {};
  
    template<typename T> struct is_trivial : public bool_constant<__is_trivial(T)> {};

    template<typename T> struct is_polymorphic : public bool_constant<__is_polymorphic(T)> {};
    
    template<typename T> struct has_virtual_destructor : public bool_constant<__has_virtual_destructor(T)> {};

    template<typename T> struct is_standard_layout : public bool_constant<__is_standard_layout(T)> {};
    
#pragma mark - Iterator traits
    
    template<typename I>
    struct indirectly_readable_traits {
        using value_type  = typename I::value_type;
        using reference   = typename I::reference;
        using pointer     = typename I::pointer;
    };

    template<typename T>
    struct indirectly_readable_traits<T*> {
        using value_type  = T;
        using reference   = T&;
        using pointer     = T*;
    };

    template<typename T>
    struct indirectly_readable_traits<const T*> {
        using value_type  = T;
        using reference   = const T&;
        using pointer     = const T*;
    };

    template<typename I>
    struct iterator_traits : indirectly_readable_traits<I> {};
    
#pragma mark - Struct layout helper for EA IFF 85 compliance
    
    template<typename T>
    struct struct_layout;
    
    template<>
    struct struct_layout<int16_t> {
        static constexpr const char* value = "1w";
    };
    template<>
    struct struct_layout<uint16_t> {
        static constexpr const char* value = "1w";
    };
    template<>
    struct struct_layout<int32_t> {
        static constexpr const char* value = "1l";
    };
    template<>
    struct struct_layout<uint32_t> {
        static constexpr const char* value = "1l";
    };
        
}
