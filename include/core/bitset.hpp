//
//  bitset.hpp
//  toybox_index
//
//  Created by Fredrik on 2025-12-18.
//

#pragma once

#include "core/concepts.hpp"

namespace toybox {
    
    template<integral Int>
    class bitset_c {
        friend class reference_c;
    public:
        static constexpr int begin_bit = 0;
        static constexpr int end_bit = sizeof(Int) * 8;
        
        class reference_c {
            friend class bitset_c;
        public:
            constexpr operator bool() const { return (_bitset._raw & _mask) != 0; }
            constexpr reference_c& operator=(bool v) {
                if (v) {
                    _bitset._raw |= _mask;
                } else {
                    _bitset._raw &= ~_mask;
                }
                return *this;
            }
        private:
            constexpr reference_c(bitset_c& bs, int bit) : _bitset(bs), _mask(1 << bit) {
                assert(bit >= begin_bit && bit < end_bit);
            }
            bitset_c& _bitset;
            Int _mask;
        };
        
        class iterator_c {
            friend class bitset_c;
        public:
            constexpr iterator_c& operator++() {
                ++_bit;
                next_set_bit();
                return *this;
            }
            constexpr iterator_c operator++(int) {
                iterator_c tmp = *this;
                ++(*this);
                return tmp;
            }
            constexpr int operator*() const {
                return _bit;
            }
            constexpr bool operator==(const iterator_c& other) const {
                return _bit == other._bit && &_bitset == &other._bitset;
            }
            constexpr bool operator!=(const iterator_c& other) const {
                return !(*this == other);
            }
        private:
            constexpr iterator_c(const bitset_c& bs, int bit) : _bitset(bs), _bit(bit) {
                next_set_bit();
            }
            constexpr void next_set_bit() {
                while (_bit < end_bit && !_bitset[_bit]) {
                    ++_bit;
                }
            }
            const bitset_c& _bitset;
            int _bit;
        };
        
        constexpr bitset_c() = default;
        template<typename... Bits>
        requires(same_as<Bits, int> && ...)
        constexpr bitset_c(Bits... bits) : _raw(((Int(1) << bits) | ...)) {
#ifdef TOYBOX_HOST
            (check_bit(bits), ...);
#endif
        }
        constexpr bitset_c(const bitset_c& o) = default;
        constexpr bitset_c(bitset_c&& o) = default;
        constexpr bitset_c& operator=(const bitset_c& o) = default;
        constexpr bitset_c& operator=(bitset_c&& o) = default;
        
        constexpr reference_c operator[](int bit) { return reference_c(*this, bit); }
        constexpr const reference_c operator[](int bit) const { return reference_c(const_cast<bitset_c&>(*this), bit); }

        constexpr operator bool() const { return _raw != 0; }
        constexpr bool operator==(const bitset_c& o) const { return _raw == o._raw; }
        constexpr bool operator==(const int bit) const { return (*this)[bit]; }

        constexpr bitset_c operator+(const bitset_c& o) const { return bitset_c(_raw | o._raw, tag_s{}); }
        constexpr bitset_c& operator+=(const bitset_c& o) { _raw |= o._raw; return *this; }
        constexpr bitset_c operator-(const bitset_c& o) const { return bitset_c(_raw & ~o._raw, tag_s{}); }
        constexpr bitset_c& operator-=(const bitset_c& o) { _raw &= ~o._raw; return *this; }

        constexpr bitset_c operator&(const bitset_c& o) const { return bitset_c(_raw & o._raw, tag_s{}); }
        constexpr bitset_c& operator&=(const bitset_c& o) { _raw &= o._raw; return *this; }

        constexpr iterator_c begin() { return iterator_c(*this, begin_bit); }
        constexpr const iterator_c begin() const { return iterator_c(*this, begin_bit); }
        constexpr iterator_c end() { return iterator_c(*this, end_bit); }
        constexpr const iterator_c end() const { return iterator_c(*this, end_bit); }

    private:
        struct tag_s{};
        constexpr bitset_c(Int raw, tag_s tag) : _raw(raw) {}
#ifdef TOYBOX_HOST
        static constexpr void check_bit(int bit) {
            assert(bit >= begin_bit && bit < end_bit);
        }
#endif
        Int _raw = 0;
    };
    
}
