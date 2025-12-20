//
//  tilemap_level_load.cpp
//  FireFlight
//
//  Created by Fredrik on 2025-12-03.
//

#include "runtime/tilemap_level.hpp"
#include "core/expected.hpp"
#include "media/dirtymap.hpp"

using namespace toybox;

tilemap_level_c::tilemap_level_c(const char* path) :
tilemap_c(rect_s()), _is_initialized(false)
{
    iffstream_c file(path);
    iff_group_s form;
    iff_chunk_s chunk;
    if (!file.good() || !file.first(cc4::FORM, detail::cc4::LEVL, form)) {
        return; // Not a ILBM
    }
    detail::level_header_s header;
    while (file.next(form, cc4::ANY, chunk)) {
        if (chunk.id == detail::cc4::LVHD) {
            if (!file.read(&header)) {
                return;
            }
            assert(header.size.width >= 20);
            assert(header.size.height >= 12);
            _tilespace_bounds = {{0,0}, header.size};
            rect_s bounds(0,0, header.size.width << 4, header.size.height << 4);
            _tiles_dirtymap = unique_ptr_c<dirtymap_c>(dirtymap_c::create(bounds.size));
            set_visible_bounds(bounds);
            _tileset_index = header.tileset_index;
            _tiles.resize(header.size.width * header.size.height);
            for (auto& tile : _tiles) {
                tile.type = tile_s::type_e::invalid;
            }
        } else if (chunk.id == cc4::NAME) {
            char* name = (char*)_malloc(chunk.size);
            file.read((uint8_t*)name, chunk.size);
            _name.reset(name);
        } else if (chunk.id == detail::cc4::ENTS) {
            assert(header.entity_count * sizeof(entity_s) == chunk.size);
            _all_entities.reserve(header.entity_count + 16);
            for (int i = 0; i < header.entity_count; ++i) {
                auto& entity = _all_entities.emplace_back();
                file.read(&entity);
            }
        } else if (chunk.id == cc4::LIST) {
            iff_group_s list;
            if (!file.expand(chunk, list) || list.subtype != detail::cc4::TMAP) {
                return;
            }
            while (file.next(list, cc4::FORM, chunk)) {
                iff_group_s form;
                if (!file.expand(chunk, form) || form.subtype != detail::cc4::TMAP) {
                    return;
                }
                detail::tilemap_header_s header;
                while (file.next(form, cc4::ANY, chunk)) {
                    if (chunk.id == detail::cc4::TMHD) {
                        if (!file.read(&header)) {
                            errno = EINVAL;
                            return;
                        }
                        _subtilemaps.emplace_back(header.bounds);
                        assert(_subtilemaps.back().tilespace_bounds().contained_by(_tilespace_bounds) && "Tilemap must fit in world bound");
                    } else if (chunk.id == detail::cc4::ENTA) {
                        auto& tilemap = _subtilemaps.back();
                        tilemap.activate_entity_idxs().resize(chunk.size);
                        file.read(tilemap.activate_entity_idxs().data(), chunk.size);
                    } else if (chunk.id == detail::cc4::BODY) {
                        auto& tilemap = _subtilemaps.back();
                        int tile_count = header.bounds.size.width * header.bounds.size.height;
                        assert(tile_count * sizeof(tile_s) == chunk.size);
                        file.read(tilemap.tiles().data(), tile_count);
                    } else {
                        assert(false && "Unkown chunk");
                        errno = EINVAL;
                        return;
                    }
                }
            }
        } else {
            assert(false && "Unkown chunk");
            errno = EINVAL;
            return;
        }
    }
    assert(_subtilemaps.size() > 0 && "Must have at least one tilemap.");
}

void tilemap_level_c::init() {
    fast_rand_seed = _name[0] + ((uint16_t)_name[1] << 8);
    setup_actions();
    setup_entity_defs();
    _tileset = init_tileset(_tileset_index);
    for (auto& entity : _all_entities) {
        init(entity);
    }
    int idx = 0;
    for (auto& tilemap : _subtilemaps) {
        for (auto& tile : tilemap.tiles()) {
            init(tile, idx);
        }
        ++idx;
    }
    splice_subtilemap(0);
    _is_initialized = true;
}

void tilemap_level_c::setup_actions() {
    _actions.emplace_back(actions::idle);
}

void tilemap_level_c::setup_entity_defs() {
}

tileset_c* tilemap_level_c::init_tileset(int index) {
    return &asset_manager_c::shared().tileset(index);
}

void tilemap_level_c::reset() {
    for (auto& tile : _tiles) {
        reset(tile);
    }
    // TODO: Should figure out how to destroy all temp entities here.
    for (auto& entity : _all_entities) {
        reset(entity);
    }
}

void tilemap_level_c::init(entity_s& entity) {}
void tilemap_level_c::init(tile_s& tile, int subtilemap_index) {}
void tilemap_level_c::reset(entity_s& entity) {}
void tilemap_level_c::reset(tile_s& tile) {}
