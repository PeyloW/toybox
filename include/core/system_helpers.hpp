//
//  system_helpers.hpp
//  toybox
//
//  Created by Fredrik on 2024-02-11.
//

#pragma once

#include "core/cincludes.hpp"

namespace toybox {
       
#define DEBUG_CPU_CLOCK_INTERUPT 0x011
#define DEBUG_CPU_MOUSE_INTERUPT 0x101
#if TOYBOX_DEBUG_CPU
    static __forceinline void debug_cpu_color(uint16_t c) {
        __asm__ volatile ("move.w %[d],0xffff8240.w" :  : [d] "g" (c) : );
    }
#else
    static void debug_cpu_color(uint16_t) { }
#endif

    static __forceinline void hard_crash() {
#ifdef __M68000__
        __asm__ volatile ("move.w #1,0xffff8241.w" :  :  : );
#else
        hard_assert(false);
#endif
    }

    struct codegen_s {
        // Buffer must be 16 bytes
        static void make_trampoline(uint16_t* buffer, void* func, bool all_regs) {
#ifdef __M68000__
            //movem.l d3-d7/a2-a6,-(sp)
            //jsr     [func].l
            //movem.l (sp)+,d3-d7/a2-a6
            //rts
            if (all_regs) {
                buffer[0] = 0x48e7;
                buffer[1] = 0xfffe;
            } else {
                buffer[0] = 0x48e7;
                buffer[1] = 0x1f3e;
            }
            buffer[2] = 0x4eb9;
            buffer[3] = (int32_t)func >> 16;
            buffer[4] = (int32_t)func & 0xffff;
            if (all_regs) {
                buffer[5] = 0x4cdf;
                buffer[6] = 0x7fff;
            } else {
                buffer[5] = 0x4cdf;
                buffer[6] = 0x7cf8;
            }
            buffer[7] = 0x4e75;
#endif
        }
    };

}
