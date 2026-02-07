//
//  tilemap_level.cpp
//  toybox
//
//  Created by Fredrik on 2025-11-12.
//

#include "core/iffstream.hpp"
#include "media/viewport.hpp"
#include "media/dirtymap.hpp"
#include "runtime/scene.hpp"
#include "runtime/tilemap_level.hpp"

using namespace toybox;


tilemap_level_c::tilemap_level_c(rect_s tilespace_bounds, tileset_c* tileset) : tilemap_c(tilespace_bounds), _tileset(tileset), _is_initialized(true) {
    assert(tilespace_bounds.origin == point_s() && "Bounds origin must be {0,0}.");
    // And we should probably only dirty the visible region is the level is larger than the display size.
    // Size here is depending on the size of the viewport to draw in later. Is max screen size good enough?
    rect_s bounds = rect_s(
        0,
        0,
        tilespace_bounds.size.width * 16,
        tilespace_bounds.size.height * 16
    );
    _tiles_dirtymap = unique_ptr_c<dirtymap_c>(dirtymap_c::create(bounds.size));
    set_total_bounds(bounds);
}

tilemap_level_c::~tilemap_level_c() {
    // TODO: This will be legal eventually.
    assert(0 && "Why?");
}

pair_c<int, action_f> tilemap_level_c::add_action(action_f action) {
    int i = _actions.size();
    _actions.push_back(action);
    return {i, action};
}

pair_c<int, entity_type_def_s&> tilemap_level_c::add_entity_type_def(tileset_c* tileset) {
    int i = _entity_type_defs.size();
    return {i, _entity_type_defs.emplace_back(tileset)};
}

entity_s& tilemap_level_c::spawn_entity(uint8_t type, uint8_t group, frect_s position) {
    auto& entity = _all_entities.emplace_back();
    entity.type = type;
    entity.group = group;
    entity.position = position;
    return entity;
};

void tilemap_level_c::destroy_entity(uint8_t id) {
    all_entities()[id].active = false;
    _destroy_entities.push_back(id);
}

void tilemap_level_c::erase_destroyed_entities() {
    if (_destroy_entities.size() > 0) {
        auto& entities = all_entities();
        for (const uint8_t id : _destroy_entities) {
            entities.erase(id);
        }
        _destroy_entities.clear();
    }
}

void tilemap_level_c::update(viewport_c& viewport, int display_id, int ticks) {
    hard_assert(_is_initialized && "Must call init() on load construction.");
    _viewport = &viewport;
    {
        // Update the AI for the level world.
        // This may dirty the tiles dirty map, and both add and remove entities.
        debug_cpu_color(0x010);
        update_level();
        erase_destroyed_entities();
    }
    {
        // Update the AI for entities.
        // This may dirty the tiles dirty map, and both add and remove entities.
        debug_cpu_color(0x020);
        update_actions();
        erase_destroyed_entities();
    }
    {
        // AI may update tiles, so we need to dirty viewports to redraw them
        debug_cpu_color(0x120);
#if TOYBOX_DEBUG_DIRTYMAP
        _tiles_dirtymap->print_debug("tilemap_level_c::update() _tiles_dirtymap");
#endif
        if (_tiles_dirtymap->is_dirty()) {
            auto& manager = scene_manager_c::shared();
            for (int idx = 0; idx < manager.display_list_count(); ++idx) {
                auto& viewport = manager.display_list((scene_manager_c::display_list_e)idx).get(display_id).viewport();
                viewport.dirtymap()->merge(*_tiles_dirtymap);
            }
        }
        _tiles_dirtymap->clear();
    }
    {
        // Draw all the tiles, both updates, and previously dirtied by drawing sprites
        debug_cpu_color(0x122);
        draw_tiles();
    }
    {
        // And lastly draw all the sprites needed
        debug_cpu_color(0x221);
        draw_entities();
    }
    _viewport = nullptr;
}

void tilemap_level_c::update_level() {
    // We do nothing in base class, subclasses may use this to update tiles.
    // For animations, or changing state completely for timed platforms, etc.
}

void tilemap_level_c::update_actions() {
    // NOTE: Will need some optimisation to not run them all eventually.
    for (auto& entity : _all_entities) {
        if (entity.action != 0 && entity.active && !entity.event) {
            _actions[entity.action](*this, entity, false);
        }
    }
}

void tilemap_level_c::draw_tiles() {
    auto& viewport = active_viewport();
    viewport.with_tileset(*_tileset, [&](){
        // Need to capture the dirty map here, so we have one.
        // And then do the restore without dirtymap so we do not dirty it when restoring.
        auto dirtymap = viewport.dirtymap();
        const auto tilemap_height = _tilespace_bounds.size.height;
        assert(dirtymap != nullptr && "Viewport must have dirtymap");
#if TOYBOX_DEBUG_DIRTYMAP
        dirtymap->print_debug("tilemap_level_c::draw_tiles()");
#endif
        dirtymap->mark<dirtymap_c::mark_type_e::mask>(viewport.clip_rect());
#if TOYBOX_DEBUG_DIRTYMAP
        dirtymap->print_debug("tilemap_level_c::draw_tiles() masked");
#endif
        assert((dirtymap->dirty_bounds().size == size_s()) || dirtymap->dirty_bounds().contained_by(viewport.clip_rect()));
        viewport.with_dirtymap(nullptr, [&]() {
            auto restore = [&](const rect_s& rect) {
                assert(rect.contained_by(viewport.clip_rect()) && "Viewport must not be dirty outside clip rect");
                const rect_s tile_rect = rect_s(
                    rect.origin.x >> 4, rect.origin.y >> 4,
                    rect.size.width >> 4, rect.size.height >> 4
                );
                point_s at = rect.origin;
                for (int y = tile_rect.origin.y; y <= tile_rect.max_y(); ++y) {
                    at.x = rect.origin.x;
                    if (y >= tilemap_height) {
                        // TODO: Should the tilemap_level_c be forced to have a viewport size as min?
                    } else {
                        for (int x = tile_rect.origin.x; x <= tile_rect.max_x(); ++x) {
                            const auto& tile = (*this)[x, y];
                            debug_cpu_color(0x223);
                            draw_tile(tile, at);
                            at.x += 16;
                        }
                    }
                    at.y += 16;
                }
            };
            dirtymap_c::restore_f func(restore);
            dirtymap->restore(func);
        });
    });
}

void tilemap_level_c::draw_tile(const tile_s& tile, point_s at) {
    if (tile.index <= 0) {
        active_viewport().fill_tile(-tile.index, at);
    } else {
        active_viewport().draw_tile(*_tileset, tile.index, at);
    }
}

void tilemap_level_c::draw_entities() {
    auto& viewport = active_viewport();
    // NOTE: This will need to be a list of visible entities eventually
    for (auto& entity : _all_entities) {
        // Draw entity if not explicitly hidden, and have frame definitions.
        if (entity.active) {
            const auto& ent_def = _entity_type_defs[entity.type];
            if (ent_def.frame_defs.size() > 0) {
                const auto& frame_def = ent_def.frame_defs[entity.frame_index];
                if (frame_def.index >= 0) {
                    const point_s origin = static_cast<point_s>(entity.position.origin);
                    const point_s at = origin - frame_def.rect.origin;
                    debug_cpu_color(0x322);
                    viewport.draw(*ent_def.tileset, frame_def.index, at);
                }
            }
        }
    }
}


void tilemap_level_c::mark_tiles_dirtymap(point_s point) {
    mark_tiles_dirtymap(rect_s(point, size_s(1,1)));
}
void tilemap_level_c::mark_tiles_dirtymap(rect_s rect) {
    _tiles_dirtymap->mark(rect);
}

template<typename Level, invocable<tile_s&> Func>
    requires same_as<typename remove_cvref<Level>::type, tilemap_level_c>
__forceinline static void enmerate_level_tiles(Level& level, const frect_s& _rect, Func func) {
    const auto rect = static_cast<rect_s>(_rect);
    assert(rect.contained_by(level.total_bounds()) && "Rect must be in bounds");
    const auto tile_x_min = rect.origin.x >> 4;
    const auto tile_y_min = rect.origin.y >> 4;
    const auto tile_x_max = rect.max_x() >> 4;
    const auto tile_y_max = rect.max_y() >> 4;
    for (int16_t y = tile_y_min; y <= tile_y_max; ++y) {
        for (int16_t x = tile_x_min; x <= tile_x_max; ++x) {
            auto& tile = level[x, y]; // TODO: Investigate muls here.
            func(tile);
        }
    }
}

void tilemap_level_c::enumerate_tiles(const frect_s& rect, function_c<void(tile_s&)> func) {
    enmerate_level_tiles(*this, rect, func);
}

tile_s::type_e tilemap_level_c::collides_with_level(uint8_t id) const {
    assert(id < _all_entities.size() && "Entity id out of bounds");
    const auto& entity = get_entity(id);
    return collides_with_level(entity.position);
}

tile_s::type_e tilemap_level_c::collides_with_level(fpoint_s at) const {
    point_s iat(at);
    const auto& tile = (*this)[iat.x >> 4, iat.y >> 4];
    return tile.type;
}

tile_s::type_e tilemap_level_c::collides_with_level(const frect_s& rect) const {
    // Check each tile in the rect's coverage area
    tile_s::type_e max_type = tile_s::none;
    enmerate_level_tiles(*this, rect, [&](const tile_s& tile) {
        max_type = max(max_type, tile.type);
    });
    return max_type;
}

bool tilemap_level_c::collides_with_entity(uint8_t id, uint8_t in_group, uint8_t* id_out) const {
    assert(id < _all_entities.size() && "Entity id out of bounds");
    assert(id_out != nullptr && "id_out must not be null");
    const auto& source_position = get_entity(id).position;
    // Iterate through all entities and check for collisions with matching group
    for (int idx = 0; idx < _all_entities.size(); ++idx) {
        if (idx == id) continue; // Skip self
        const auto& entity = _all_entities[idx];
        if (entity.group != in_group) continue;
        if (!entity.active) continue;
        if (source_position.intersects(entity.position)) {
            *id_out = (uint8_t)idx;
            return true;
        }
    }
    return false;
}

bool tilemap_level_c::collides_with_entity(const frect_s& rect, uint8_t in_group, uint8_t* id_out) const {
    assert(id_out != nullptr && "id_out must not be null");
    // Iterate through all entities and check for collisions with matching group
    for (int idx = 0; idx < _all_entities.size(); ++idx) {
        const auto& entity = _all_entities[idx];
        if (entity.group != in_group) continue;
        if (!entity.active) continue;
        if (rect.intersects(entity.position)) {
            *id_out = (uint8_t)idx;
            return true;
        }
    }
    return false;
}

void tilemap_level_c::set_total_bounds(const rect_s& bounds) {
    _tiles_dirtymap->mark(bounds);
#if TOYBOX_DEBUG_DIRTYMAP
    _tiles_dirtymap->print_debug("tilemap_level_c::set_total_bounds()");
#endif
    _total_bounds = bounds;
}

void tilemap_level_c::set_visible_bounds(const rect_s& bounds) {
    assert(bounds.contained_by(_total_bounds) && "Visible bounds must be in total bounds");
    _visible_bounds = bounds;
}

void tilemap_level_c::add_visible_bounds(const rect_s& bounds) {
    assert(bounds.contained_by(_total_bounds) && "Visible bounds must be in total bounds");
    _visible_bounds = _visible_bounds.unification(bounds);
}

void tilemap_level_c::splice_subtilemap(int index) {
    // NOTE: Stretch goal would be to animate these, but probably not worth the effort.
    auto& tilemap = _subtilemaps[index];
    const auto& bounds = tilemap.tilespace_bounds();
    assert(bounds.contained_by(tilespace_bounds()));
    point_s at = bounds.origin;
    for (int y = 0; y < bounds.size.height; ++y) {
        at.x = bounds.origin.x;
        for (int x = 0; x < bounds.size.width; ++x) {
            auto& tile = tilemap[x,y];
            splice_tile(tile, at);
            ++at.x;
        }
        ++at.y;
    }
    rect_s rect(bounds.origin.x << 4, bounds.origin.y << 4, bounds.size.width << 4, bounds.size.height << 4);
    add_visible_bounds(rect);
    _tiles_dirtymap->mark(rect);
    for (const auto idx : tilemap.activate_entity_idxs()) {
        splice_entity(_all_entities[idx]);
    }
}

void tilemap_level_c::splice_tile(tile_s& tile, point_s tilespace_at) {
    if (tile.type != tile_s::invalid) {
        (*this)[tilespace_at] = tile;
    }
}

void tilemap_level_c::splice_entity(entity_s& entity) {
    entity.active = 1;
}

