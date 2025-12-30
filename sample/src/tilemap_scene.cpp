//
//  tilemap_scene.cpp
//  toybox
//
//  Created by Fredrik on 2025-11-12.
//

#include "tilemap_scene.hpp"
#include "demo_assets.hpp"
#include "fullscreen_scene.hpp"

static constexpr uint16_t is_target = 1 << 0;

enum entity_type {
    PLAYER = 0,
    BOX = 1
};

enum tile_type {
    EMPTY = -1,   // Color  #0 - Black
    WALL = 1,    // Tile   #1 - Brick wall
    FLOOR = -10, // Color #10 - Light gray
    TARGET = -13 // Color #13 - Light blue
};

enum player_frame_index {
    UP, DOWN, LEFT, RIGHT
};

static bool move_entity_if_possible(tilemap_level_c& level, entity_s& entity, fpoint_s delta) {
    // Move entity, if not posissible we adjust back on exit
    entity.position.origin = entity.position.origin + delta;
    // Collision with level is always fail
    if (level.collides_with_level(entity.id) < tile_s::solid) {
        uint8_t box_id;
        if (!level.collides_with_entity(entity.id, BOX, &box_id)) {
            return true;
        } else if (entity.type == PLAYER) {
            // Colliding with a box is fine, if we are the player and box can be moved
            if (move_entity_if_possible(level, level.entity_at(box_id), delta)) {
                return true;
            }
        }
    }
    entity.position.origin = entity.position.origin - delta;
    return false;
}

static void player_control(tilemap_level_c& level, entity_s& entity, bool event) {
    auto dir = controller_c::shared().directions();
    fpoint_s delta(0,0);
    if ((dir & controller_c::up) == true) {
        delta.y -= 1;
        entity.frame_index = UP;
    } else if ((dir & controller_c::down) == true) {
        delta.y += 1;
        entity.frame_index = DOWN;
    }
    if ((dir & controller_c::left) == true) {
        delta.x -= 1;
        entity.frame_index = LEFT;
    } else if ((dir & controller_c::right) == true) {
        delta.x += 1;
        entity.frame_index = RIGHT;
    }
    if (delta != fpoint_s()) {
        move_entity_if_possible(level, entity, delta);
    }
    point_s offset((int16_t)entity.position.origin.x - 160 + 8, 0);
    level.active_viewport().set_offset(offset);
}

tilemap_level_c* create_tilemaplevel() {
    static constexpr const char* recipe[] = {
        "       #####              ",
        "       #---#              ",
        "       #$--#              ",
        "     ###--$##             ",
        "     #--$-$-#             ",
        "   ###-#-##-#   ######    ",
        "   #---#-##-#####--..#    ",
        "   #-$--$----------..#    ",
        "   #####-###-#@##--..#    ",
        "       #-----#########    ",
        "       #######            ",
    };
    const size_s size((int16_t)strlen(recipe[0]), 11);
    auto level_ptr = new tilemap_level_c(rect_s(point_s(), size_s(size.width,size.height)), &asset_manager_c::shared().tileset(ASSET_TILESET_WALL));
    auto& level = *level_ptr;
        
    // Setup available actions
    level.add_action(&actions::idle);
    level.add_action(&player_control);
    
    // Setup entity type defs:
    auto& player = level.add_entity_type_def().second;
    player.tileset = &asset_manager_c::shared().tileset(ASSET_TILESET_SPR);
    player.frame_defs.push_back({ 2, {{2,2},{12,12}} }); // Up
    player.frame_defs.push_back({ 1, {{2,2},{12,12}} }); // Down
    player.frame_defs.push_back({ 4, {{2,2},{12,12}} }); // Left
    player.frame_defs.push_back({ 3, {{2,2},{12,12}} }); // Right
    auto& box = level.add_entity_type_def().second;
    box.tileset = &asset_manager_c::shared().tileset(ASSET_TILESET_SPR);
    box.frame_defs.push_back({ 5, {{0,0},{16,16}} });

    for (int y = 0; y < size.height; ++y) {
        const char* line = recipe[y];
        for (int x = 0; x < size.width; x++) {
            auto& tile = level[x,y];
            auto origin = [&]() {
                return fpoint_s(x * 16, y * 16);
            };
            switch (line[x]) {
                case ' ':
                    tile.index = EMPTY;
                    break;
                case '#':
                    tile.index = WALL;
                    tile.type = tile_s::solid;
                    break;
                case '-':
                    tile.index = FLOOR;
                    break;
                case '.':
                    tile.index = TARGET;
                    tile.flags = is_target;
                    break;
                case '@': {
                    tile.index = FLOOR;
                    auto& player = level.spawn_entity(PLAYER, PLAYER, frect_s{ origin() + fpoint_s(2,2), {12,12} });
                    player.action = 1;
                    player.frame_index = DOWN;
                    break;
                }
                case '$': {
                    tile.index = FLOOR;
                    auto& box = level.spawn_entity(BOX, BOX, frect_s{ origin(), {16,16} });
                    break;
                }
                default:
                    break;
            }
        }
    }
    
    return level_ptr;
}


tilemap_scene_c::tilemap_scene_c() :
    _level(asset_manager_c::shared().tilemap_level(ASSET_LEVEL))
{
}

const scene_c::configuration_s& tilemap_scene_c::configuration() const {
    static scene_c::configuration_s config{_level.visible_bounds().size, asset_manager_c::shared().tileset(ASSET_TILESET_SPR).image()->palette(), 2, false};
    return config;
}

void tilemap_scene_c::will_appear(bool obscured) {
}

void tilemap_scene_c::update(display_list_c& display, int ticks) {
    auto& viewport = display.get(PRIMARY_VIEWPORT).viewport();
    _level.update(viewport, PRIMARY_VIEWPORT, ticks);
    if (controller_c::shared().state() == button_state_e::clicked) {
        auto next_scene = new fullscreen_scene_c();
        auto& pal = *configuration().palette;
        manager.replace(next_scene, transition_c::create(pal[1]));
    }
}
