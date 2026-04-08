//
//  input.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-24.
//

#pragma once

#include "core/cincludes.hpp"
#include "core/geometry.hpp"
#include "core/optionset.hpp"

namespace toybox {
    
    
    enum class button_state_e : uint8_t {
        released,
        pressed,
        clicked
    };

    /**
     A `mouse_c` is an abstraction for mouse input.
     The mouse is a lazy initialized singleton.
     Aquiring a mouse disables joystick controller inputs
     */
    class mouse_c : public nocopy_c {
    public:
        enum class button_e : uint8_t {
            right, left
        };
        using enum button_e;
        
        static mouse_c& shared();

        const rect_s& limits() const_pure;
        void set_limits(const rect_s& limits);
        
        bool is_pressed(button_e button) const;
        button_state_e state(button_e button) const;
        
        point_s position();
        
    private:
        mouse_c();
        ~mouse_c();
        rect_s _limits;
        mutable uint32_t _update_tick;
    };
    
    /**
     A `controller_c` is an abstraction for joystick/joypad input.
     The joysticks are lazy initialized singleton.
     Aquiring a joystick controller disables mouse inputs
     */
    class controller_c : public nocopy_c {
    public:
        enum class port_e : uint8_t {
            joy_0, joy_1 /* , jaypad_a, jaypad_b */
        };
        using enum port_e;
        
        static controller_c& shared(port_e port = joy_1);
        
        using enum directions_e;
        enum class button_e : uint8_t {
            fire = 1 << 7
        };
        using enum button_e;
        
        directions_e directions() const;
        
        bool is_pressed(button_e button) const;
        button_state_e state(button_e button = fire) const;
    private:
        controller_c(port_e port);
        ~controller_c();
        port_e _port;
        mutable uint32_t _update_tick;
    };
    template<>
    class is_optionset<controller_c::button_e> : public true_type {};

    /**
     A `keyboard_c` is an abstraction for keyboard input.
     The keyboard is a lazy initialized singleton.
     */
    class keyboard_c : public nocopy_c {
    public:
        static keyboard_c& shared();
        
        enum class scancode_e : uint8_t {
            // Function keys
            f1          = 0x3b,
            f2          = 0x3c,
            f3          = 0x3d,
            f4          = 0x3e,
            f5          = 0x3f,
            f6          = 0x40,
            f7          = 0x41,
            f8          = 0x42,
            f9          = 0x43,
            f10         = 0x44,
            // Arrow keys
            up          = 0x48,
            down        = 0x50,
            left        = 0x4b,
            right       = 0x4d,
            // Misc keys
            help        = 0x62,
            undo        = 0x61,
            insert      = 0x52,
            del         = 0x53,
            clr_home    = 0x47,
        };
        struct key_s {
            scancode_e scancode;
            char ascii;
        };
        static_assert(sizeof(key_s) == 2);
        enum class statekey_e : uint8_t {
            rshift   = 1 << 0,
            lshift   = 1 << 1,
            control  = 1 << 2,
            alt      = 1 << 3,
            capslock = 1 << 4,
        };
        
        bool has_key() const;
        key_s key() const;
        
        statekey_e statekeys() const;
    private:
        keyboard_c();
        ~keyboard_c();
    };

    template<>
    class is_optionset<keyboard_c::statekey_e> : public true_type {};
    
}
