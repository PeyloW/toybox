//
//  math.hpp
//  toybox
//
//  Created by Fredrik on 2025-10-29.
//

#pragma once

#include "core/concepts.hpp"
//#include <math.h>

#ifdef abs
#undef abs
#endif

namespace toybox {

    constexpr float truncf(float x) {
        return (float)(int32_t)x;
    }
    
    constexpr float roundf(float x) {
        return truncf(x + (x < 0 ? -0.5f : 0.5f));
    }
    
    template<integral Int>
    static __forceinline constexpr next_larger<Int>::type mul_fast(Int x, Int y) {
        typename next_larger<Int>::type t = x;
        return t * y;
    }
#if __M68000__
    template<>
    __forceinline constexpr uint32_t mul_fast(uint16_t x, uint16_t y) {
        if consteval {
            uint32_t t = x;
            return t * y;
        } else {
            if (__builtin_constant_p(x) || __builtin_constant_p(y)) {
                uint32_t t = x;
                return t * y;
            } else {
                uint32_t r;
                __asm__ ("mulu %[y],%[r]" : [r]"=d"(r) : "[r]"(x), [y]"dmi"(y));
                return r;
            }
        }
    }
    template<>
    __forceinline constexpr int32_t mul_fast(int16_t x, int16_t y) {
        if consteval {
            int32_t t = x;
            return t * y;
        } else {
            if (__builtin_constant_p(x) || __builtin_constant_p(y)) {
                int32_t t = x;
                return t * y;
            } else {
                int32_t r;
                __asm__ ("muls %[y],%[r]" : [r]"=d"(r) : "[r]"(x), [y]"dmi"(y));
                return r;
            }
        }
    }
#endif
    
    template<integral Int>
    struct div_t {
        Int rem; Int quot;
        constexpr operator Int() const { return quot; }
    };
    template<integral Int>
    static constexpr div_t<Int> div_fast(typename next_larger<Int>::type x, Int y) {
        return div_t<Int>{ static_cast<Int>(x % y), static_cast<Int>(x / y) };
    }
#if __M68000__
    template<>
    __forceinline constexpr div_t<uint16_t> div_fast(uint32_t x, uint16_t y) {
        if consteval {
            return div_t<uint16_t>{ static_cast<uint16_t>(x % y), static_cast<uint16_t>(x / y) };
        } else {
            if (__builtin_constant_p(y)) {
                return div_t<uint16_t>{ static_cast<uint16_t>(x % y), static_cast<uint16_t>(x / y) };
            } else {
                div_t<uint16_t> r;
                __asm__ ("divu %[y],%[r]" : [r]"=d"(r) : "[r]"(x), [y]"dmi"(y));
                return r;
            }
        }
    }
    template<>
    __forceinline constexpr div_t<int16_t> div_fast(int32_t x, int16_t y) {
        if consteval {
            return div_t<int16_t>{ static_cast<int16_t>(x % y), static_cast<int16_t>(x / y) };
        } else {
            if (__builtin_constant_p(y)) {
                return div_t<int16_t>{ static_cast<int16_t>(x % y), static_cast<int16_t>(x / y) };
            } else {
                div_t<int16_t> r;
                __asm__ ("divs %[y],%[r]" : [r]"=d"(r) : "[r]"(x), [y]"dmi"(y));
                return r;
            }
        }
    }
#endif
    
    template<integral Int, int Bits>
    struct base_fix_t {
        using LargerInt = next_larger<Int>::type;
        Int raw;

        constexpr base_fix_t() : raw(0) {};
        constexpr base_fix_t(const base_fix_t& o) = default;
        constexpr base_fix_t(base_fix_t&& o) = default;
        constexpr base_fix_t& operator=(const base_fix_t& o) = default;
        constexpr base_fix_t(Int raw, bool tag) : raw(raw) {};

        template<integral OInt>
        constexpr base_fix_t(OInt v) : raw(static_cast<Int>(v) << Bits) {}
        constexpr base_fix_t(float v) : raw(static_cast<Int>(roundf(v * (static_cast<LargerInt>(1) << Bits)))) {}
        
        constexpr explicit operator bool() const { return raw != 0; }
        template<integral OInt>
        constexpr explicit operator OInt() const { return static_cast<OInt>(raw >> Bits); }
        constexpr explicit operator float() const { return static_cast<float>(raw) / (static_cast<LargerInt>(1) << Bits); }
        
        constexpr base_fix_t operator+(const base_fix_t o) const {
            return base_fix_t(static_cast<Int>(raw + o.raw), true);
        }
        constexpr base_fix_t& operator+=(const base_fix_t o) {
            raw += o.raw;
            return *this;
        }
        template<integral OInt>
        constexpr base_fix_t operator+(OInt o) const {
            return *this + base_fix_t(o);
        }
        template<integral OInt>
        constexpr base_fix_t& operator+=(OInt o) {
            raw += base_fix_t(o).raw;
            return *this;
        }
        
        constexpr base_fix_t operator-(const base_fix_t o) const {
            return base_fix_t(static_cast<Int>(raw - o.raw), true);
        }
        constexpr base_fix_t& operator-=(const base_fix_t o) {
            raw -= o.raw;
            return *this;
        }
        template<integral OInt>
        constexpr base_fix_t operator-(OInt o) const {
            return *this - base_fix_t(o);
        }
        template<integral OInt>
        constexpr base_fix_t& operator-=(OInt o) {
            raw -= base_fix_t(o).raw;
            return *this;
        }

        constexpr base_fix_t operator-() const {
            return base_fix_t(-raw, true);
        }

        constexpr base_fix_t operator*(const base_fix_t o) const {
            const auto r = mul_fast(raw, o.raw);
            return base_fix_t(static_cast<Int>(r >> Bits), true);
        }
        constexpr base_fix_t operator*=(const base_fix_t o) {
            const auto r = mul_fast(raw, o.raw);
            raw = r >> Bits;
            return *this;
        }
        template<integral OInt>
        constexpr base_fix_t operator*(OInt o) const {
            return base_fix_t(raw * static_cast<Int>(o), true);
        }
        template<integral OInt>
        constexpr base_fix_t& operator*=(OInt o) {
            raw *= static_cast<Int>(o);
            return *this;
        }
        constexpr base_fix_t mul(base_fix_t o) const {
            static constexpr auto half = (LargerInt)1 << (Bits - 1);
            const auto r = mul_fast(raw, o.raw);
            return base_fix_t(static_cast<Int>((r + (r >= 0 ? half : -half)) >> Bits), true);
        }
        
        constexpr base_fix_t operator/(const base_fix_t o) const {
            const auto eraw = (LargerInt)raw << Bits;
            const auto r = div_fast(eraw, o.raw);
            return base_fix_t(r.quot, true);
        }
        constexpr base_fix_t operator/=(const base_fix_t o) {
            const auto eraw = (LargerInt)raw << Bits;
            raw = div_fast(eraw, o.raw).quot;
            return *this;
        }
        template<integral OInt>
        constexpr base_fix_t operator/(OInt o) const {
            return base_fix_t(raw / static_cast<Int>(o), true);
        }
        template<integral OInt>
        constexpr base_fix_t& operator/=(OInt o) {
            raw /= static_cast<Int>(o);
            return *this;
        }
        constexpr base_fix_t div(base_fix_t o) const {
            const bool sign_match = ((raw >= 0) == (o.raw >= 0));
            const auto eraw = (LargerInt)raw << Bits;
            const auto half_div = o.raw >> 1;
            // Round to nearest by adding half divisor in the direction of the result
            const auto adjusted = eraw + (sign_match ? half_div : -half_div);
            return base_fix_t(div_fast(adjusted, o.raw), true);
        }
        
        constexpr base_fix_t operator%(const base_fix_t o) const {
            return base_fix_t(raw % o.raw, true);
        }
        constexpr base_fix_t operator%=(const base_fix_t o) {
            raw = raw % o.raw;
            return *this;
        }
        template<integral OInt>
        constexpr base_fix_t operator%(OInt o) const {
            return base_fix_t(raw % (static_cast<Int>(o) << Bits), true);
        }
        template<integral OInt>
        constexpr base_fix_t& operator%=(OInt o) {
            raw %= (static_cast<Int>(o) << Bits);
            return *this;
        }
        template<integral OInt>
        constexpr base_fix_t operator<<(OInt s) const {
            return base_fix_t(raw << s, true);
        }
        template<integral OInt>
        constexpr base_fix_t& operator<<=(OInt s) const {
            raw <<= s;
            return *this;
        }
        template<integral OInt>
        constexpr base_fix_t operator>>(OInt s) const {
            return base_fix_t(raw >> s, true);
        }
        template<integral OInt>
        constexpr base_fix_t& operator>>=(OInt s) const {
            raw >>= s;
            return *this;
        }

        constexpr bool operator==(const base_fix_t o) const {
            return raw == o.raw;
        }
        template<integral OInt>
        constexpr bool operator==(OInt o) const {
            return raw == base_fix_t(o).raw;
        }
        constexpr bool operator<(const base_fix_t o) const {
            return raw < o.raw;
        }
        template<integral OInt>
        constexpr bool operator<(OInt o) const {
            return raw < base_fix_t(o).raw;
        }

        friend constexpr base_fix_t abs(base_fix_t x) {
            return base_fix_t(ABS(x.raw), true);
        }
        friend constexpr base_fix_t trunc(base_fix_t x) {
            return base_fix_t(x.raw & (static_cast<Int>(~0) << Bits), true);
        }
        friend constexpr base_fix_t floor(base_fix_t x) {
            Int truncated = x.raw & (static_cast<Int>(~0) << Bits);
            if (x.raw < 0 && x.raw != truncated) {
                truncated -= (static_cast<Int>(1) << Bits);
            }
            return base_fix_t(truncated, true);
        }
        friend constexpr base_fix_t ceil(base_fix_t x) {
            Int truncated = x.raw & (static_cast<Int>(~0) << Bits);
            if (x.raw > 0 && x.raw != truncated) {
                truncated += (static_cast<Int>(1) << Bits);
            }
            return base_fix_t(truncated, true);
        }
        friend constexpr base_fix_t round(base_fix_t x) {
            static constexpr Int half = static_cast<Int>(1) << (Bits - 1);
            static constexpr Int frac_mask = (static_cast<Int>(1) << Bits) - 1;
            const Int frac = x.raw & frac_mask;
            if (x.raw >= 0) {
                return base_fix_t((x.raw + half) & (static_cast<Int>(~0) << Bits), true);
            } else {
                return base_fix_t((x.raw - half) & (static_cast<Int>(~0) << Bits), true);
            }
        }
        
    };

    using fix16_t = base_fix_t<int16_t, 4>;

    namespace numbers {
        static inline constexpr fix16_t one = fix16_t(1);
        static inline constexpr fix16_t pi = fix16_t(3.1415f);
        static inline constexpr fix16_t pi2x = pi * 2;  // This gets rounded wrong to 6.25, instead of 6.3125, we take the error for consistency.
        static inline constexpr fix16_t pi_2 = pi / 2;
        static inline constexpr fix16_t e = fix16_t(2.7182f);
        static inline constexpr fix16_t log2 = fix16_t(0.6875f);
    }

    fix16_t pow(fix16_t base, fix16_t exp); // Perf: integer exp; O(log n), good to horrible (1-many muls), otherwise O(1), horrible (~19 muls + 17 divs).
    fix16_t sqrt(fix16_t x); // Perf: O(1), bad to horrible (up to 6 divs, typically 2-3)

    fix16_t sin(fix16_t x);  // Perf: O(1), good (1 div)
    fix16_t cos(fix16_t x);  // Perf: O(1), good (1 div via sin)
    fix16_t tan(fix16_t x);  // Perf: O(1), bad (3 divs)

    fix16_t exp(fix16_t x);  // Perf: O(1), horrible (~12 muls + 12 divs)
    fix16_t log(fix16_t x);  // Perf: O(1), horrible (~6 muls + 5 divs)
    
}
