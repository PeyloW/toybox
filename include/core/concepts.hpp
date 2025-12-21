//
//  concepts.hpp
//  toybox
//
//  Created by Fredrik on 2025-10-05.
//

#pragma once

#include "core/type_traits.hpp"

namespace toybox {
    
#pragma mark - Type concepts
    template<typename A, typename B>
    concept same_as = is_same<A, B>::value;
    
    
    template<typename T>
    concept integral = is_integral<T>::value;

    template<typename T>
    concept floating_point = is_floating_point<T>::value;

    template<typename T>
    concept arithmetic = integral<T> || floating_point<T>;
    
    template<typename T>
    concept class_type = __is_class(T);
    
    template<typename T>
    concept enum_type = __is_enum(T);
    
    template<typename T>
    concept pointer_type = is_pointer<T>::value;
    
    template<typename From, typename To>
    concept convertible_to =
    requires(From (&f)()) {
        static_cast<To>(declval<From>());  // convertible explicitly
        To{f()};                                // convertible implicitly
    };
    
    template<typename Base, typename Derived>
    concept base_of = __is_base_of(Base, Derived);
    
    template<typename Derived, typename Base>
    concept derived_from = base_of<Base, Derived> && convertible_to<const volatile Derived*, const volatile Base*>;
    
#pragma mark - Operator concepts
    
    template<typename T>
    concept equality_comparable = requires(const T& a, const T& b) {
        { a == b } -> convertible_to<bool>;
        { a != b } -> convertible_to<bool>;
    };

    template<typename T>
    concept less_than_comparable = requires(const T& a, const T& b) {
        { a < b } -> convertible_to<bool>;
    };
    
    template<typename T>
    concept ordered = equality_comparable<T> && less_than_comparable<T>;
    
    template<typename T, typename U>
    concept convertable_to = requires(T&& x) {
        static_cast<U>(forward<U>(x));
    };
    
    template<typename F, typename... Args>
    concept invocable = requires(F&& f, Args&&... args) {
        { f(static_cast<Args&&>(args)...) };
    };
    template<typename F, typename R, typename... Args>
    concept invocable_r = requires(F&& f, Args&&... args) {
        { f(static_cast<Args&&>(args)...) } -> same_as<R>;
    };
    template<typename F, typename... Args>
    concept predicate = invocable_r<F, bool, Args...>;
    
    template<typename I>
    concept incrementable = requires(I i) {
        { i++ } -> same_as<I>;
        { ++i } -> same_as<I&>;
    };

    template<typename I>
    concept decrementable = requires(I i) {
        { i-- } -> same_as<I>;
        { --i } -> same_as<I&>;
    };
    
    template<typename I>
    concept const_dereferencable = requires(const I ci) {
        { *ci };
    };
    template<typename I>
    concept dereferencable = const_dereferencable<I> &&
    requires(I i, typename indirectly_readable_traits<I>::value_type&& v) {
        { *i = static_cast<typename indirectly_readable_traits<I>::value_type&&>(v) };
    };
    
    template<typename I>
    concept const_random_accessible = const_dereferencable<I> && requires(I i, int n) {
        { i += n } -> same_as<I&>;
        { i +  n } -> same_as<I>;
        { n +  i } -> same_as<I>;
        { i -= n } -> same_as<I&>;
        { i -  n } -> same_as<I>;
        {  i[n]  } -> same_as<decltype(*I{})>;
    };
    template<typename I>
    concept random_accessible = const_random_accessible<I> && dereferencable<I> &&
    requires(I i, int n, typename indirectly_readable_traits<I>::value_type&& v) {
        { i[n] = static_cast<typename indirectly_readable_traits<I>::value_type&&>(v) };
    };
    
#pragma mark - Iterator concepts
    
    template<typename I>
    concept forward_iterator = incrementable<I> && dereferencable<I>;
    template<typename I>
    concept const_forward_iterator = incrementable<I> && const_dereferencable<I>;

    template<typename I>
    concept backward_iterator = decrementable<I> && dereferencable<I>;
    template<typename I>
    concept const_backward_iterator = decrementable<I> && const_dereferencable<I>;
    
    template<typename I>
    concept bidirectional_iterator = forward_iterator<I> && backward_iterator<I>;
    template<typename I>
    concept const_bidirectional_iterator = const_forward_iterator<I> && const_backward_iterator<I>;

    template<typename I>
    concept const_random_access_iterator = const_bidirectional_iterator<I> && const_random_accessible<I>;
    template<typename I>
    concept random_access_iterator = bidirectional_iterator<I> && random_accessible<I>;

}
