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
     TODO: Implement this
     */
    class keyboard_c : public nocopy_c {
    };

}
