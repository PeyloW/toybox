//
//  entity_helpers.cpp
//  toybox
//
//  Created by Fredrik on 2025-12-01.
//

#include "runtime/entity_helpers.hpp"

namespace toybox {

    void set_frame_index(const tilemap_level_c& level, const entity_s& entity, uint8_t index, resize_origin_e origin = resize_origin_e::none);
    
    frect_s entity_position_with_frame_index(const tilemap_level_c& level, const entity_s& entity, uint8_t index, resize_origin_e origin) {
        const auto& ent_def = level.entity_type_defs()[entity.type].frame_defs[index];
        // TODO: Calculate actual new position rect here
        return entity.position;
    }

    void set_frame_index(const tilemap_level_c& level, entity_s& entity, uint8_t index, resize_origin_e origin) {
        const auto& ent_def = level.entity_type_defs()[entity.type].frame_defs[index];
        entity.frame_index = index;
        entity.position = entity_position_with_frame_index(level, entity, index, origin);
    }
    
}
