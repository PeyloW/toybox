//
//  blitter_atari.hpp
//  toybox
//
//  Created by Fredrik on 2024-02-25.
//

#pragma once

#include "core/cincludes.hpp"
#include "core/geometry.hpp"

namespace toybox {
    
#if TOYBOX_TARGET_ATARI
    
    /**
     A `blitter_s` is an abstraction of the Atari blitter.
     Emulated on emulation host.
     */
    struct blitter_s {
        enum class hop_e : uint8_t {
            one = 0,
            halftone = 1,
            src = 2,
            src_and_halftone = 3
        };
        
        enum class lop_e : uint8_t {
            zero = 0,
            src = 3,
            notsrc_and_dst = 4,
            src_or_dst = 7,
            one = 15
        };

        static constexpr uint8_t skew_mask = 0x0f;
        static constexpr uint8_t nfsr_bit = (1<<6);
        static constexpr uint8_t fxsr_bit = (1<<7);
        
        static constexpr uint8_t hog_bit = (1<<6);
        static constexpr uint8_t busy_bit = (1<<7);

        __target_volatile uint16_t halftoneRAM[16];
        __target_volatile int16_t srcIncX;
        __target_volatile int16_t srcIncY;
        __target_volatile uint16_t* __target_volatile pSrc;
        __target_volatile uint16_t endMask[3];
        __target_volatile int16_t dstIncX;
        __target_volatile int16_t dstIncY;
        __target_volatile uint16_t* __target_volatile pDst;
        __target_volatile uint16_t countX;
        __target_volatile uint16_t countY;
        __target_volatile hop_e HOP;
        __target_volatile lop_e LOP;
        __target_volatile uint8_t mode;
        __target_volatile uint8_t skew;

        __forceinline uint8_t get_skew() const {
            return skew & skew_mask;
        }
        __forceinline bool is_nfsr() const {
            return (skew & nfsr_bit) != 0;
        }
        __forceinline bool is_fxsr() const {
            return (skew & fxsr_bit) != 0;
        }
        __forceinline uint16_t halftone() const {
            return halftoneRAM[mode & 0xf];
        }
#ifdef __M68000__
        __forceinline void start(bool hog = false) {
            if (hog) {
                __asm__ volatile ("move.b #0xc0,0xffff8A3C.w \n\t"  : : : );
            } else {
                __asm__ volatile (
                                  "move.b #0x80,0xffff8A3C.w \n\t"
                                  "nop \n"
                                  "1: bset.b #7,0xffff8A3C.w \n\t"
                                  "nop \n\t"
                                  "bne.s 1b \n\t" : : : );
            }
        }
#else
        bool debug;
        void start(bool hog = false);
#endif
    };
    
#ifdef __M68000__
    static struct blitter_s* pBlitter = (struct blitter_s*)0xffff8a00;
#else
    extern struct blitter_s* pBlitter;
#endif
    
#else
#   error "Unsupported target"
#endif
    
}
