//
//  machine.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-24.
//

#pragma once

#include "core/cincludes.hpp"
#include "core/geometry.hpp"
#include "core/memory.hpp"

namespace toybox {
    
    
    class palette_c;
    class viewport_c;
    class display_list_c;
    
    /**
     A `machine_c` is an abstraction for the target machine and OS.
     An emulation host such as macOS is **not** a machine of its own.
     */
    class machine_c : public nocopy_c {
    public:
#ifdef TOYBOX_HOST
        constexpr static bool host = true;
#else
        constexpr static bool host = false;
#endif
#if TOYBOX_TARGET_ATARI
        constexpr static bool atari = true;
        constexpr static bool amiga = true;
#elif TOYBOY_TARGET_AMIGA
        constexpr static bool atari = false;
        constexpr static bool amiga = true;
#else
#   error "Unsupported target"
#endif

        enum class type_e : uint8_t {
            unknown,
#if TOYBOX_TARGET_ATARI
            st, ste, falcon
#else
            osc, aga
#endif
        };
        using enum type_e;
        
        static machine_c& shared();

        using machine_f = int (*)(machine_c& machine);
        static int with_machine(int argc, const char* argv[], machine_f f);
        
        type_e type() const_pure;
        size_s screen_size() const_pure; // NOTE: static constexpr?
        size_t max_memory() const_pure;
        size_t user_memory() const_pure;
        void free_system_memory();

        uint32_t get_cookie(uint32_t cookie, uint32_t def_value = 0) const_pure;

        const shared_ptr_c<display_list_c>& active_display_list() const_pure;
        void set_active_display_list(const shared_ptr_c<display_list_c>& display_list);

    private:
        static machine_c* _shared_machine;
        shared_ptr_c<display_list_c> _active_display_list;
        machine_c();
        ~machine_c();
        void set_active_viewport(const shared_ptr_c<const viewport_c>& viewport);
        void set_active_palette(const shared_ptr_c<const palette_c>& palette);
#if TOYBOX_TARGET_ATARI
        uint32_t _old_super;
        uint16_t _old_modes[3];
#endif
    };
    
}
