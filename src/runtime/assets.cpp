//
//  asset.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-26.
//

#include "runtime/assets.hpp"
#include "media/image.hpp"
#include "media/tileset.hpp"
#include "media/font.hpp"
#include "media/audio.hpp"
#include "core/expected.hpp"

using namespace toybox;

static unique_ptr_c<asset_manager_c> s_shared;

asset_manager_c& asset_manager_c::shared() {
    static asset_manager_c s_shared;
    return s_shared;
}

asset_manager_c::asset_manager_c() {}

void asset_manager_c::preload(asset_set_t sets, progress_f progress) {
    int ids[_asset_defs.size()];
    int count = 0;
    int id = 0;
    for (auto& def : _asset_defs) {
        if ((def.sets & sets) && (_assets[id].get() == nullptr)) {
            ids[count++] = id;
        }
        id++;
    }
    for (int i = 0; i < count; i++) {
        asset(ids[i]);
        if (progress) {
            progress(i + 1, count);
        }
    }
}

void asset_manager_c::unload(asset_set_t sets) {
    int id = 0;
    for (auto& def : _asset_defs) {
        if ((def.sets & sets)) {
            _assets[id].reset();
        }
        id++;
    }
}

void asset_manager_c::unload(int id) {
    _assets[id].reset();
}

asset_c& asset_manager_c::asset(int id) const {
    auto& asset = _assets[id];
    if (asset.get() == nullptr) {
        asset.reset(create_asset(id, _asset_defs[id]));
    }
    return *asset;
}

void asset_manager_c::add_asset_def(int id, const asset_def_s& def) {
    while (_asset_defs.size() <= id) {
        _asset_defs.emplace_back(asset_c::custom, 0);
    }
    _asset_defs[id] = def;
    while (_assets.size() < _asset_defs.size()) {
        _assets.emplace_back();
    }
}

int asset_manager_c::add_asset_def(const asset_def_s& def) {
    int id = _asset_defs.size();
    add_asset_def(id, def);
    return id;
}

unique_ptr_c<char> asset_manager_c::data_path(const char* file) const {
    unique_ptr_c<char> path((char*)_malloc(128));
    strstream_c str(path.get(), 128);
#ifdef TOYBOX_HOST
    str << "data/";
#else
    str << "data\\";
#endif
    str << file << ends;
    return path;
}

unique_ptr_c<char> asset_manager_c::user_path(const char* file) const {
    unique_ptr_c<char> path((char*)_malloc(128));
    strstream_c str(path.get(), 128);
#ifndef __M68000__
    str << "/tmp/";
#endif
    str << file << ends;
    return path;
}

asset_c* asset_manager_c::create_asset(int id, const asset_def_s& def) const {
    auto path = def.file ? data_path(def.file) : nullptr;
    if (def.create) {
        return def.create(*this, path.get());
    } else {
        switch (def.type) {
            case asset_c::image:
                return expected_cast(new expected_c<image_c>(failable, path.get()));
            case asset_c::tileset:
                return expected_cast(new expected_c<tileset_c>(failable, path.get(), size_s(16, 16)));
            case asset_c::font:
                return expected_cast(new expected_c<font_c>(failable, path.get(), size_s(8, 8)));
            case asset_c::sound:
                return expected_cast(new expected_c<sound_c>(failable, path.get()));
            case asset_c::music:
                return expected_cast(new expected_c<music_c>(failable, path.get()));
            case asset_c::tilemap_level:
                // TODO: Implement file format and loading.
                return nullptr;
            default:
                hard_assert(0 && "Custom asset must have create function.");
                break;
        }
    }
    return nullptr;
}
