//
//  input.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-24.
//

#include "machine/input.hpp"
#include "machine/timer.hpp"

using namespace toybox;

static uint8_t g_prev_mouse_buttons;
uint8_t g_mouse_buttons;
static button_state_e g_mouse_button_states[2];
point_s g_mouse_position;

volatile uint8_t g_joysticks[2] = { 0 };
static button_state_e g_joystick_button_states[2] = { button_state_e::released };

extern "C" {
#ifndef __M68000__
    // Host must call when mouse state changes
    void g_update_mouse(point_s position, bool left, bool right) {
        g_mouse_position = position;
        g_mouse_buttons = (left ? 2 : 0) | (right ? 1 : 0);
    }
    
    void g_update_joystick(directions_e directions, bool fire) {
        g_joysticks[1] = ((uint8_t)directions | (fire ? (uint8_t)controller_c::fire : 0));
    }
#endif
}

mouse_c& mouse_c::shared() {
    static mouse_c s_shared;
    return s_shared;
}

const rect_s& mouse_c::limits() const {
    return _limits;
}

void mouse_c::set_limits(const rect_s& limits) {
    _limits = limits;
    g_mouse_position = point_s(
        limits.origin.x + _limits.size.width / 2,
        limits.origin.y + _limits.size.height / 2
    );
}

static void update_mouse_state() {
    for (int button = 0; button < 2; button++) {
        if (g_mouse_buttons & (1 << button)) {
            g_mouse_button_states[button] = button_state_e::pressed;
        } else if (g_prev_mouse_buttons & (1 << button)) {
            g_mouse_button_states[button] = button_state_e::clicked;
        } else {
            g_mouse_button_states[button] = button_state_e::released;
        }
    }
    g_prev_mouse_buttons = g_mouse_buttons;
}

bool mouse_c::is_pressed(button_e button) const {
    return (g_mouse_buttons & (1 << (int)button)) != 0;
}

button_state_e mouse_c::state(button_e button) const {
    auto tick = timer_c::shared(timer_c::timer_e::vbl).tick();
    if (tick > _update_tick) {
        update_mouse_state();
        _update_tick = tick;
    }
    return g_mouse_button_states[(int)button];
}

point_s mouse_c::position() {
    point_s clamped_point = point_s(
        MIN(_limits.max_x(), MAX(g_mouse_position.x, _limits.origin.x)),
        MIN(_limits.max_y(), MAX(g_mouse_position.y, _limits.origin.y))
    );
    g_mouse_position = clamped_point;
    return clamped_point;
}


controller_c& controller_c::shared(port_e port) {
    static controller_c s_controllers[2] = { controller_c(joy_0), controller_c(joy_1) };
    return s_controllers[(int)port];
}

directions_e controller_c::directions() const {
    //hard_crash();
    return (directions_e)(g_joysticks[(int)_port] & 0xf);
}

static void update_joystick_state() {
    for (int joystick = 0; joystick < 2; joystick++) {
        const auto& joy = controller_c::shared((controller_c::port_e)joystick);
        if (joy.is_pressed(controller_c::fire)) {
            g_joystick_button_states[joystick] = button_state_e::pressed;
        } else if (g_joystick_button_states[joystick] == button_state_e::pressed) {
            g_joystick_button_states[joystick] = button_state_e::clicked;
        } else {
            g_joystick_button_states[joystick] = button_state_e::released;
        }
    }
}

bool controller_c::is_pressed(button_e button) const {
    return (g_joysticks[(int)_port] & (uint8_t)button) != 0;
}
button_state_e controller_c::state(button_e button) const {
    auto tick = timer_c::shared(timer_c::timer_e::vbl).tick();
    if (tick > _update_tick) {
        update_joystick_state();
        _update_tick = tick;
    }
    return g_joystick_button_states[(int)_port];
}
