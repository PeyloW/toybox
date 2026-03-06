//
//  timer.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-24.
//

#pragma once

#include "core/cincludes.hpp"
#include "core/geometry.hpp"
#include "core/concepts.hpp"
#ifndef __M68000__
#include "machine/host_bridge.hpp"
#endif

namespace toybox {
    
    
    /**
     A `timer_c` abstracts a system timer.
     Timer are lazy initialized singletons.
     VBL (50/60Hz) and system timer at 200Hz are required.
     */
    class timer_c : public nocopy_c {
    public:
        enum class timer_e : uint8_t {
            vbl, clock
        };
        using enum timer_e;
        using func_t = void(*)(void);
        using func_a_t = void(*)(void*);
        using func_i_t = void(*)(int);
        
        static timer_c& shared(timer_e timer);

        template<invocable<> Commands>
        static inline void with_paused_timers(Commands commands) {
#ifdef __M68000__
            uint16_t saved_sr;
            __asm__ volatile ("move.w %%sr,%0" : "=d"(saved_sr));
            __asm__ volatile ("move.w #0x2700,%%sr" : : : "cc");
            commands();
            __asm__ volatile ("move.w %0,%%sr" : : "d"(saved_sr) : "cc");
#else
            host_bridge_c::shared().pause_timers();
            commands();
            host_bridge_c::shared().resume_timers();
#endif
        }
        
        uint8_t base_freq() const;
        
        void add_func(const func_t func, uint8_t freq = 0);
        void remove_func(const func_t func);
        void add_func(const func_a_t func, void* context = nullptr, uint8_t freq = 0);
        void remove_func(const func_a_t func, const void* context = nullptr);
        
        uint32_t tick();
        void reset_tick();
        void wait(int ticks = 1);
        
    private:
        timer_c(timer_e timer);
        ~timer_c();
        timer_e _timer;
        uint32_t _previous_tick = ~0;
    };
    
}
