//
//  actions.hpp
//  toybox
//
//  Created by Fredrik on 2025-11-13.
//


#pragma once

#include "runtime/entity.hpp"

namespace toybox {
    
    class tilemap_level_c;
    using action_f = void(*)(tilemap_level_c& level, entity_s& entity, bool event);
    
    namespace actions {
        static void idle(tilemap_level_c& level, entity_s& entity, bool event) {};
    }
    
}

