//
//  graphics.hpp
//  toybox
//
//  Created by Fredrik on 2024-02-04.
//

#pragma once

#include "core/array.hpp"
#include "core/geometry.hpp"
#include "core/algorithm.hpp"
#include "media/display_list.hpp"

namespace toybox {
    
    using namespace toybox;

    /**
     A 12 bit color in a 16 bit word.
     MOTE: Make STe hacks for Atari target only, once Amiga is to be supported.
     */
    class color_c {
    public:
        uint16_t color;
        constexpr color_c() = default;
        constexpr color_c(uint16_t c) : color(c) {}
        constexpr color_c(uint8_t r, uint8_t g, uint8_t b) : color(to_ste(r, 8) | to_ste(g, 4) | to_ste(b, 0)) {}
        void set_at(int i) const {
#ifdef __M68000__
#   if TOYBOX_TARGET_ATARI
            reinterpret_cast<uint16_t*>(0xffff8240)[i] = color;
#   else
#       error "Unsupported target"
#   endif
#endif
        }
        constexpr void get(uint8_t* r_out, uint8_t* g_out, uint8_t* b_out) const {
            *r_out = from_ste(color, 8);
            *g_out = from_ste(color, 4);
            *b_out = from_ste(color, 0);
        }
        color_c mix(color_c other, int shade) const;
        static constexpr int MIX_FULLY_THIS = 0;
        static constexpr int MIX_FULLY_OTHER = 64;
    private:
        static __forceinline constexpr uint16_t to_ste(uint8_t c, uint8_t shift) {
            constexpr uint8_t STE_TO_SEQ[16] = { 0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15 };
            const int i = c >> 4;
            __assume_count(i, 16);
            return STE_TO_SEQ[i] << shift;
        }
        static __forceinline constexpr uint8_t from_ste(uint16_t c, uint8_t shift) {
            constexpr uint8_t STE_FROM_SEQ[16] = { 0x00, 0x22, 0x44, 0x66, 0x88, 0xaa, 0xcc, 0xee,
                0x11, 0x33, 0x55, 0x77, 0x99, 0xbb, 0xdd, 0xff};
            const int i = (c >> shift) & 0x0f;
            __assume_count(i, 16);
            return STE_FROM_SEQ[i];
        }
    };
        
    /**
     A `basic_palette_c` is an arbitrary list of colors.
     */
    static_assert(!is_polymorphic<nocopy_c>::value);
    template<int Count>
    class basic_palette_c : public array_s<color_c, Count> {
    public:
        constexpr basic_palette_c() = default;
        constexpr basic_palette_c(uint16_t* cs) { copyn(cs, Count, this->begin()); }
        constexpr basic_palette_c(uint8_t* c) {
            for (int i = 0; i < Count; i++) {
                this->_data[i] = color_c(c[0], c[1], c[2]);
                c += 3;
            }
        }
    };
        
    /**
     A `palette_c` is a specialized list of colors with exactly 16 colors.
     NOTE: Should Amiga target allow for 32?
     */
    static_assert(!is_polymorphic<basic_palette_c<16>>::value);
    class palette_c : public display_item_c, public basic_palette_c<16>, public nocopy_c {
    public:
        type_e display_type() const override { return palette; }
        constexpr palette_c() : basic_palette_c<16>() {}
        constexpr palette_c(uint16_t* cs) : basic_palette_c<16>(cs) {}
        constexpr palette_c(uint8_t* c) : basic_palette_c<16>(c) {}
    };
    
}
