//
//  host_bridge.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-25.
//

#ifdef TOYBOX_HOST

#include "machine/host_bridge.hpp"
#include "media/image.hpp"
#include <errno.h>

using namespace toybox;

extern "C" {
    extern void g_vbl_interupt();
    extern void g_clock_interupt();
    extern void g_update_mouse(point_s position, bool left, bool right);
    extern void g_update_joystick(directions_e directions, bool fire);
    
#ifdef TOYBOX_HOST
    static const char* s_added_searchpath = nullptr;
    void _add_searchpath(const char* path) {
        s_added_searchpath = path;
    }
    
    FILE* _fopen(const char* path, const char* mode) {
        FILE* file = nullptr;
        if (s_added_searchpath) {
            char buffer[1024] = {0};
            strcpy(buffer, s_added_searchpath);
            strcat(buffer, "/");
            strcat(buffer, path + 4);
            file = fopen(buffer, mode);
        }
        if (file == nullptr) {
            errno = 0;
            file = fopen(path, mode);
        }
        return file;
    }
#endif
}

static host_bridge_c* s_bridge = nullptr;

host_bridge_c& host_bridge_c::shared() {
    assert(s_bridge && "Host bridge not initialized");
    return *s_bridge;
}
void host_bridge_c::set_shared(host_bridge_c* bridge) {
    s_bridge = bridge;
}

// Host must call on a 50/60hz interval
void host_bridge_c::vbl_interupt() {
    g_vbl_interupt();
}

// Host must call on a 200hz interval
void host_bridge_c::clock_interupt() {
    g_clock_interupt();
}
        
// Host must call when mouse state changes
void host_bridge_c::update_mouse(point_s position, bool left, bool right) {
    g_update_mouse(position, left, right);
}

void host_bridge_c::update_joystick(directions_e directions, bool fire) {
    g_update_joystick(directions, fire);
}

int host_bridge_c::get_pixel(const image_c& image, point_s at, bool clipping) const {
    if (clipping) {
        return image.get_pixel(at);
    } else {
        return image.imp_get_pixel(at);
    }
}

#endif
