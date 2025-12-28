//
//  tilemap_scene.hpp
//  toybox
//
//  Created by Fredrik on 2025-11-12.
//

#pragma once

#include "runtime/scene.hpp"
#include "runtime/tilemap_level.hpp"

using namespace toybox;

tilemap_level_c* create_tilemaplevel();

class tilemap_scene_c final : public scene_c {
public:
    tilemap_scene_c();

    virtual const scene_c::configuration_s &configuration() const override;
    virtual void will_appear(bool obscured) override;
    virtual void update(display_list_c& display, int ticks) override;

private:
    tilemap_level_c& _level;
};
