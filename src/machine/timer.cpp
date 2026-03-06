//
//  timer.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-24.
//

#include "machine/timer.hpp"
#include "core/list.hpp"

using namespace toybox;


#ifndef __M68000__
#include <unistd.h>
#include "machine/host_bridge.hpp"
#endif

struct  timer_func_s {
uint8_t freq;
uint8_t cnt;
timer_c::func_a_t func;
void* context;
};

#define TIMER_FUNC_MAX_CNT 16
using timer_func_list_c = list_c<timer_func_s, TIMER_FUNC_MAX_CNT>;
#ifdef __M68000__
static_assert(sizeof(timer_func_list_c::detail::node_s) == 14, "timer_func_list_c::detail::node_s size mismatch");
#endif

timer_func_list_c g_vbl_functions;
volatile uint32_t g_vbl_tick = 0;
timer_func_list_c g_clock_functions;
volatile uint32_t g_clock_tick = 0;

extern "C" {
#ifndef __M68000__
    static void g_do_timer(timer_func_list_c& timer_funcs, int freq) {
        for (auto& timer_func : timer_funcs) {
            bool trigger = false;
            int cnt = (int)timer_func.cnt - timer_func.freq;
            if (cnt <= 0) {
                trigger = true;
                cnt += freq;
            }
            timer_func.cnt = (uint8_t)cnt;
            if (trigger) {
                timer_func.func(timer_func.context);
            }
        }
    }
    
    void g_vbl_interupt() {
        g_vbl_tick += 1;
        g_do_timer(g_vbl_functions, timer_c::shared(timer_c::timer_e::vbl).base_freq());
    }
    
    void g_clock_interupt() {
        g_clock_tick += 1;
        g_do_timer(g_clock_functions, 200);
    }
#endif
}

timer_c& timer_c::shared(timer_e timer) {
    switch (timer) {
        case timer_e::vbl: {
            static timer_c s_timer_vbl(timer_e::vbl);
            return s_timer_vbl;
        }
        case timer_e::clock: {
            static timer_c s_timer_clock(timer_e::clock);
            return s_timer_clock;
        }
        default:
            hard_assert(0);
            return *(timer_c*)0x0;
    }
}



void timer_c::add_func(const func_t func, uint8_t freq) {
    add_func((func_a_t)func, nullptr, freq);
}

void timer_c::remove_func(const func_t func) {
    remove_func((func_a_t)func, nullptr);
}

void timer_c::add_func(const func_a_t func, void* context, uint8_t freq) {
    if (freq == 0) {
        freq = base_freq();
    }
    with_paused_timers([this, func, context, freq] {
        auto&functions = _timer == timer_e::vbl ? g_vbl_functions : g_clock_functions;
        functions.push_front((timer_func_s){freq, base_freq(), func, context});
    });
}

void timer_c::remove_func(const func_a_t func, const void* context) {
    with_paused_timers([this, func, context] {
        auto&functions = _timer == timer_e::vbl ? g_vbl_functions : g_clock_functions;
        auto prev = functions.before_begin();
        auto curr = functions.begin();
        auto pred = [&func, &context](const timer_func_s& f) __forceinline_lambda {
            return f.func == func && f.context == context;
        };
        while (curr != functions.end()) {
            if (pred(*curr)) {
                functions.erase_after(prev);
                return;
            }
            prev = curr;
            ++curr;
        }
        assert(0 && "Timer function not found in list");
    });
}

uint32_t timer_c::tick() {
    return (_timer == timer_e::vbl) ? g_vbl_tick : g_clock_tick;
}

void timer_c::reset_tick() {
    if (_timer == timer_e::vbl) {
        g_vbl_tick = 0;
    } else {
        g_clock_tick = 0;
    }
}

void timer_c::wait(int ticks) {
    const auto wait_tick = max(tick() + 1, _previous_tick + ticks);
    while (wait_tick > (_previous_tick = tick())) {
#ifndef __M68000__
        host_bridge_c::shared().yield();
#else
        __asm__ volatile("stop #0x2300" : : : "cc");
#endif
    }
}
