//
//  input.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-24.
//

#include "machine/input.hpp"
#include "machine/machine.hpp"
#include "machine/timer.hpp"

#ifndef __M68000__
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
static struct termios g_orig_termios;
static bool g_termios_saved = false;

// Try to read one byte from stdin with a short timeout (for escape sequences)
static bool try_read_byte(char& c, int timeout_usec = 1000) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = { 0, timeout_usec };
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
        return read(STDIN_FILENO, &c, 1) == 1;
    }
    return false;
}
#endif

#if TOYBOX_TARGET_ATARI

using namespace toybox;

/*
  Mouse and joystick reading is mostly implemented in system_helpers_atari.S
  for maximumum speed during gameplay.
  For simplicity keyboard uses standard BIOS calls, cannot be used in-game,
  but works fine for highscore entry and such.
 */
extern "C" {
#ifdef __M68000__
    extern timer_c::func_a_t g_system_mouse_interupt;
    extern void g_mouse_interupt(void*);
    extern timer_c::func_a_t g_system_joystick_interupt;
    extern void g_joystick_interupt(void*);
    static _KBDVECS* g_keyboard_vectors = nullptr;

    static bool g_joystick_reporting = false;
    
    static void init_keyboard_vectors(void) {
        if (g_keyboard_vectors == nullptr) {
            g_keyboard_vectors = Kbdvbase();
        }
    }
#endif
}


mouse_c::mouse_c() : _update_tick(0) {
    set_limits(rect_s(point_s(), machine_c::shared().screen_size()));
#ifdef __M68000__
    if (g_system_mouse_interupt == nullptr) {
        init_keyboard_vectors();
        g_system_mouse_interupt = g_keyboard_vectors->mousevec;
        g_keyboard_vectors->mousevec = &g_mouse_interupt;
    }
    if (g_joystick_reporting) {
        static char s_packer[] = {0x1A, 0x08}; // Relative mouse reporting on
        Ikbdws(1, s_packer);
        g_joystick_reporting = false;
    }
#endif
}

mouse_c::~mouse_c() {
#ifdef __M68000__
    g_keyboard_vectors->mousevec = g_system_mouse_interupt;
#endif
}

controller_c::controller_c(controller_c::port_e port) : _port(port) {
#ifdef __M68000__
    if (g_system_joystick_interupt == nullptr) {
        init_keyboard_vectors();
        g_system_joystick_interupt = g_keyboard_vectors->joyvec;
        g_keyboard_vectors->joyvec = &g_joystick_interupt;
    }
    if (!g_joystick_reporting) {
        static char s_packer[] = {0x12, 0x14}; //Joustick reporting on, mouse off
        Ikbdws(1, s_packer);
        g_joystick_reporting = true;
    }
#endif
}

controller_c::~controller_c() {
#ifdef __M68000__
    if (g_system_joystick_interupt != nullptr) {
        g_keyboard_vectors->joyvec = g_system_joystick_interupt;
        g_system_joystick_interupt = nullptr;
    }
#endif
}


keyboard_c& keyboard_c::shared() {
    static keyboard_c s_shared;
    return s_shared;
}

keyboard_c::keyboard_c() {
#ifndef __M68000__
    // Set terminal to raw, non-blocking mode
    if (isatty(STDIN_FILENO)) {
        tcgetattr(STDIN_FILENO, &g_orig_termios);
        g_termios_saved = true;
        struct termios raw = g_orig_termios;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }
#endif
}

keyboard_c::~keyboard_c() {
#ifndef __M68000__
    // Restore original terminal settings
    if (g_termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
    }
#endif
}

bool keyboard_c::has_key() const {
#ifdef __M68000__
    return Bconstat(2) != 0;
#else
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = { 0, 0 };
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
#endif
}

keyboard_c::key_s keyboard_c::key() const {
#ifdef __M68000__
    uint32_t raw = Bconin(2);
    return key_s {
        .scancode = (scancode_e)(raw >> 16),
        .ascii = (char)(raw & 0xff)
    };
#else
    char c = 0;
    read(STDIN_FILENO, &c, 1);
    if (c != 0x1b) {
        // Regular key — return ascii directly, scancode 0
        return key_s { .scancode = (scancode_e)0, .ascii = c };
    }
    // Got ESC — check for escape sequence
    char seq;
    if (!try_read_byte(seq)) {
        // Bare ESC key
        return key_s { .scancode = (scancode_e)0, .ascii = 0x1b };
    }
    if (seq == '[') {
        // CSI sequence: ESC [ ...
        char code;
        if (!try_read_byte(code)) {
            return key_s { .scancode = (scancode_e)0, .ascii = 0x1b };
        }
        switch (code) {
            case 'A': return key_s { .scancode = scancode_e::up,       .ascii = 0 };
            case 'B': return key_s { .scancode = scancode_e::down,     .ascii = 0 };
            case 'C': return key_s { .scancode = scancode_e::right,    .ascii = 0 };
            case 'D': return key_s { .scancode = scancode_e::left,     .ascii = 0 };
            case 'H': return key_s { .scancode = scancode_e::clr_home, .ascii = 0 };
            case '2': case '3': case '1': case '5': case '7':
            case '8': case '9': {
                // Extended: ESC [ <digit> [<digit>] ~
                char next;
                if (!try_read_byte(next)) break;
                if (next == '~') {
                    switch (code) {
                        case '2': return key_s { .scancode = scancode_e::insert, .ascii = 0 };
                        case '3': return key_s { .scancode = scancode_e::del,    .ascii = 0 };
                    }
                } else if (next >= '0' && next <= '9') {
                    // Two-digit code: ESC [ <code> <next> ~
                    char tilde;
                    try_read_byte(tilde);
                    int num = (code - '0') * 10 + (next - '0');
                    switch (num) { // ANSI skips 16; F1-F4 use ESC O P/Q/R/S
                        case 15: return key_s { .scancode = scancode_e::f5,  .ascii = 0 };
                        case 17: return key_s { .scancode = scancode_e::f6,  .ascii = 0 };
                        case 18: return key_s { .scancode = scancode_e::f7,  .ascii = 0 };
                        case 19: return key_s { .scancode = scancode_e::f8,  .ascii = 0 };
                        case 20: return key_s { .scancode = scancode_e::f9,  .ascii = 0 };
                        case 21: return key_s { .scancode = scancode_e::f10, .ascii = 0 };
                    }
                }
                break;
            }
        }
    } else if (seq == 'O') {
        // SS3 sequence: ESC O P/Q/R/S → F1-F4
        char code;
        if (!try_read_byte(code)) {
            return key_s { .scancode = (scancode_e)0, .ascii = 0x1b };
        }
        switch (code) {
            case 'P': return key_s { .scancode = scancode_e::f1, .ascii = 0 };
            case 'Q': return key_s { .scancode = scancode_e::f2, .ascii = 0 };
            case 'R': return key_s { .scancode = scancode_e::f3, .ascii = 0 };
            case 'S': return key_s { .scancode = scancode_e::f4, .ascii = 0 };
        }
    }
    // Unrecognized sequence — return ESC
    return key_s { .scancode = (scancode_e)0, .ascii = 0x1b };
#endif
}

keyboard_c::statekey_e keyboard_c::statekeys() const {
#ifdef __M68000__
    // Kbshift(-1) returns current modifier key state
    // Bits: 0=Rshift, 1=Lshift, 2=Control, 3=Alt, 4=Capslock
    // This matches statekey_e bit layout directly
    return (statekey_e)(Kbshift(-1) & 0x1f);
#else
    return (statekey_e)0;
#endif
}

#endif
