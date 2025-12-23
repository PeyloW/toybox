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
    set_visible_bounds(bounds);
}

tilemap_level_c::~tilemap_level_c() {
    // TODO: This will be legal eventually.
    assert(0 && "Why?");
}

void tilemap_level_c::update_entity_indexes(int from) {
    assert(_all_entities.size() <= 255 && "Too many entities");
    for (int i = from; i < _all_entities.size(); ++i) {
        _all_entities[i].index = (uint8_t)i;
    }
}

entity_s& tilemap_level_c::spawn_entity(uint8_t type, uint8_t group, frect_s position) {
    const int idx = _all_entities.size();
    auto& entity = _all_entities.emplace_back();
    entity.index = idx;
    entity.type = type;
    entity.group = group;
    entity.position = position;
    return entity;
};

void tilemap_level_c::destroy_entity(int index) {
    auto& entity = _all_entities[index];
    entity.action = 0;  // If destroyed by world, no need to also run actions.
    entity.group = 0;   // Move to no group to disable collision detection.
    _destroy_entities.push_back(index);
}

void tilemap_level_c::erase_destroyed_entities() {
    if (_destroy_entities.size() > 0) {
        sort(_destroy_entities.begin(), _destroy_entities.end());
        for (int i = _destroy_entities.size() - 1; i >= 0; --i) {
            const int j = _destroy_entities[i];
            _all_entities.erase(j);
        }
        update_entity_indexes(_destroy_entities[0]);
        _destroy_entities.clear();
    }
}

static bool verify_entity_indexes(const tilemap_level_c& level) {
    const auto& entities = level.all_entities();
    for (int i = 0; i < entities.size(); ++i) {
        if (entities[i].index != i) {
            return false;
        }
    }
    return true;
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
        assert(verify_entity_indexes(*this) && "Invalid entity index detected");
    }
    {
        // Update the AI for entities.
        // This may dirty the tiles dirty map, and both add and remove entities.
        debug_cpu_color(0x020);
        update_actions();
        erase_destroyed_entities();
        assert(verify_entity_indexes(*this) && "Invalid entity index detected");
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
        assert(verify_entity_indexes(*this) && "Invalid entity index detected");
    }
    {
        // And lastly draw all the sprites needed
        debug_cpu_color(0x221);
        draw_entities();
        assert(verify_entity_indexes(*this) && "Invalid entity index detected");
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

tile_s::type_e tilemap_level_c::collides_with_level(int index) const {
    assert(index >= 0 && index < _all_entities.size() && "Entity index out of bounds");
    const auto& entity = _all_entities[index];
    return collides_with_level(entity.position);
}

tile_s::type_e tilemap_level_c::collides_with_level(fpoint_s at) const {
    point_s iat(at);
    const auto& tile = (*this)[iat.x >> 4, iat.y >> 4];
    return tile.type;
}

tile_s::type_e tilemap_level_c::collides_with_level(const frect_s& rect) const {
    const auto pixel_rect = static_cast<rect_s>(rect);
    assert(pixel_rect.contained_by(_visible_bounds) && "Rect must be in visible bounds");
    // Tile coordinate bounds
    const auto tile_x_min = pixel_rect.origin.x >> 4;
    const auto tile_y_min = pixel_rect.origin.y >> 4;
    const auto tile_x_max = pixel_rect.max_x() >> 4;
    const auto tile_y_max = pixel_rect.max_y() >> 4;
    // Check each tile in the rect's coverage area
    tile_s::type_e max_type = tile_s::none;
    for (int16_t y = tile_y_min; y <= tile_y_max; ++y) {
        for (int16_t x = tile_x_min; x <= tile_x_max; ++x) {
            const auto& tile = (*this)[x, y];
            max_type = max(max_type, tile.type);
        }
    }
    return max_type;
}

bool tilemap_level_c::collides_with_entity(int index, uint8_t in_group, int* index_out) const {
    assert(index >= 0 && index < _all_entities.size() && "Entity index out of bounds");
    assert(index_out != nullptr && "index_out must not be null");
    const auto& source_position = _all_entities[index].position;
    // Iterate through all entities and check for collisions with matching group
    for (int idx = 0; idx < _all_entities.size(); ++idx) {
        if (idx == index) continue; // Skip self
        const auto& entity = _all_entities[idx];
        if (entity.group != in_group) continue;
        if (!entity.active) continue;
        if (source_position.intersects(entity.position)) {
            *index_out = idx;
            return true;
        }
    }
    return false;
}

bool tilemap_level_c::collides_with_entity(const frect_s& rect, uint8_t in_group, int* index_out) const {
    assert(index_out != nullptr && "index_out must not be null");
    // Iterate through all entities and check for collisions with matching group
    for (int idx = 0; idx < _all_entities.size(); ++idx) {
        const auto& entity = _all_entities[idx];
        if (entity.group != in_group) continue;
        if (!entity.active) continue;
        if (rect.intersects(entity.position)) {
            *index_out = idx;
            return true;
        }
    }
    return false;
}

void tilemap_level_c::set_visible_bounds(const rect_s& bounds) {
    // TODO: When changing bounds columns (and eventually rows) of tiles needs to be marked dirty.
    // NOTE: Outside of visible bounds should always be clean.
    _tiles_dirtymap->mark(bounds);
#if TOYBOX_DEBUG_DIRTYMAP
    _tiles_dirtymap->print_debug("tilemap_level_c::set_visible_bounds()");
#endif
    _visible_bounds = bounds;
}

void tilemap_level_c::splice_subtilemap(int index) {
    // TODO: Merge the subtilemap into self, and mark all changes tiles as dirty.
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

