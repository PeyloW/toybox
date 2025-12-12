//
//  entity.hpp
//  toybox
//
//  Created by Fredrik on 2025-11-11.
//

#pragma once

#include "core/geometry.hpp"
#include "core/memory.hpp"
#include "media/tileset.hpp"
#include "machine/input.hpp"
#include "runtime/tilemap.hpp"

namespace toybox {

    struct entity_s {
        uint8_t index = 0;
        uint8_t active:1 = 1;  // Only active entities are drawn, run and actions.
        uint8_t event:1 = 0;    // If set action is not called per frame, only on target event trigger
        uint8_t flags:6 = 0;
        uint8_t type = 0;
        uint8_t group = 0;
        uint8_t action = 0;
        uint8_t frame_index = 0;
        frect_s position;
        uint16_t reserved_data[5];
        template<class T> requires (sizeof(T) <= 10)
        T& data_as() { return (T&)(reserved_data[0]); }
        template<class T> requires (sizeof(T) <= 10)
        const T& data_as() const { return (const T&)(reserved_data[0]); }
    };
    static_assert((offsetof(entity_s, reserved_data) & 1) == 0);
    static_assert(sizeof(entity_s) == 24);

    // struct_layout for byte-order swapping
    template<>
    struct struct_layout<entity_s> {
        static constexpr const char* value = "6b4w10b";  // index, type, group, action, frame_index, flags, position(4w), data[10]
    };

    struct entity_type_def_s {
        struct frame_def_s {
            int index;      // -1 do not draw
            rect_s rect;    // Rect of relative to graphics tile
        };
        tileset_c* tileset;
        vector_c<frame_def_s, 0> frame_defs; //
    };
    
}
