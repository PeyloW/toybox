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
    if (level.collides_with_level(entity.index) < tile_s::solid) {
        int box_index;
        if (!level.collides_with_entity(entity.index, BOX, &box_index)) {
            return true;
        } else if (entity.type == PLAYER) {
            // Colliding with a box is fine, if we are the player and box can be moved
            if (move_entity_if_possible(level, level.all_entities()[box_index], delta)) {
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

tilemap_level_c* make_tilemaplevel() {
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
    auto level_ptr = new tilemap_level_c(rect_s(point_s(), size_s(size.width,size.height)), &asset_manager_c::shared().tileset(TILESET_WALL));
    auto& level = *level_ptr;
        
    // Setup available actions
    level.actions().emplace_back(&actions::idle);
    level.actions().emplace_back(&player_control);
    
    // Setup entity type defs:
    auto& player = level.entity_type_defs().emplace_back();
    player.tileset = &asset_manager_c::shared().tileset(TILESET_SPR);
    player.frame_defs.push_back({ 2, {{2,2},{12,12}} }); // Up
    player.frame_defs.push_back({ 1, {{2,2},{12,12}} }); // Down
    player.frame_defs.push_back({ 4, {{2,2},{12,12}} }); // Left
    player.frame_defs.push_back({ 3, {{2,2},{12,12}} }); // Right
    auto& box = level.entity_type_defs().emplace_back();
    box.tileset = &asset_manager_c::shared().tileset(TILESET_SPR);
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
                case '@':
                    tile.index = FLOOR;
                    level.all_entities().emplace(level.all_entities().begin(), (entity_s){
                        .type=PLAYER, .group=PLAYER,
                        .action = 1,
                        .frame_index = DOWN,
                        .position=frect_s{ origin() + fpoint_s(2,2), {12,12} }
                    });
                    break;
                case '$':
                    tile.index = FLOOR;
                    level.all_entities().emplace_back((entity_s){
                        .type=BOX, .group=BOX,
                        .position=frect_s{ origin(), {16,16} }
                    });
                    break;
                default:
                    break;
            }
        }
    }
    level.update_entity_indexes();
    
    return level_ptr;
}


tilemap_scene_c::tilemap_scene_c() :
    _level(asset_manager_c::shared().tilemap_level(LEVEL))
{
}

scene_c::configuration_s& tilemap_scene_c::configuration() const {
    static scene_c::configuration_s config{_level.visible_bounds().size, asset_manager_c::shared().tileset(TILESET_SPR).image()->palette(), 2, false};
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
