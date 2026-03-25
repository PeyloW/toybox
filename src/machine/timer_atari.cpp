//
//  timer.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-24.
//

#include "machine/timer.hpp"
#include "machine/machine.hpp"

#if TOYBOX_TARGET_ATARI

using namespace toybox;

extern "C" {
#ifdef __M68000__
    extern timer_c::func_t g_system_vbl_interupt;
    extern void g_vbl_interupt();
    extern int16_t g_system_vbl_freq;
    extern timer_c::func_t g_system_clock_interupt;
    extern void g_clock_interupt();
#endif
}

timer_c::timer_c(timer_e timer) : _timer(timer) {
    assert((timer == timer_e::vbl || timer == timer_e::clock) && "Timer must be VBL or clock");
    machine_c::shared();
    with_paused_timers([timer] {
        switch (timer) {
            case timer_e::vbl:
#ifdef __M68000__
                g_system_vbl_interupt = *((func_t*)0x0070);
                *((func_t*)0x0070) = &g_vbl_interupt;
                g_system_vbl_freq = *(uint8_t*)0xffff820a == 0 ? 60 : 50;
#endif
                break;
            case timer_e::clock:
#ifdef __M68000__
                g_system_clock_interupt = *((func_t*)0x0114);
                *((func_t*)0x0114) = &g_clock_interupt;
#endif
                break;
        }
    });
}

timer_c::~timer_c() {
#ifndef TOYBOX_HOST
    with_paused_timers([this] {
#   if TOYBOX_TARGET_ATARI
        switch (_timer) {
            case timer_e::vbl:
#ifdef __M68000__
                *((func_t*)0x0070) = g_system_vbl_interupt;
#endif
                break;
            case timer_e::clock:
#ifdef __M68000__
                *((func_t*)0x0114) = g_system_clock_interupt;
#endif
                break;
        }
#else
#  error "Unsupported target"
#endif
    });
#endif
}

uint8_t timer_c::base_freq() const {
    switch (_timer) {
        case timer_e::vbl:
#ifdef __M68000__
            return g_system_vbl_freq;
#else
            return 50;
#endif
            //
        case timer_e::clock:
            return 200;
        default:
            assert(0 && "Unsupported timer type");
            return 0;
    }
}

#endif
