//
//  main.cpp
//  toybox - sample
//
//  Created by Fredrik on 2025-10-12.
//

#include "machine/machine.hpp"
#include "media/audio_mixer.hpp"
#include "runtime/assets.hpp"
#include "runtime/tilemap_level.hpp"
#include "demo_assets.hpp"
#include "fullscreen_scene.hpp"
#include "tilemap_scene.hpp"

static asset_manager_c& setup_assets() {
    constexpr pair_c<int,asset_manager_c::asset_def_s> asset_defs[] = {
        // Background is simply loaded from iff image file.
        { ASSET_BACKGROUND, asset_manager_c::asset_def_s(asset_c::image, 1, "backgrnd.iff") },

        // Sprites are loaded from image, but uses lambda to rempa colors
        { ASSET_SPRITES, asset_manager_c::asset_def_s(asset_c::tileset, 1, "sprites.iff", [](const asset_manager_c& manager, const char* path) -> asset_c* {
            auto image = new image_c(path, 0);
            constexpr auto table = canvas_c::remap_table_c({
                {1, 10}, {2, 11}, {3, 11}, {4, 12}, {5, 13}, {6, 14}
            });
            canvas_c canvas(*image);
            canvas.remap_colors(table, rect_s(0, 0, 128, 32));
            return new tileset_c(image, size_s(32, 32));
        })},
        
        // Music is just an SNDH file
        { ASSET_MUSIC, asset_manager_c::asset_def_s(asset_c::music, 1, "music.snd") },
        
        { ASSET_TILESET_WALL, asset_manager_c::asset_def_s(asset_c::tileset, 2, "wall.iff", [](const asset_manager_c& manager, const char* path) -> asset_c* {
            auto image = new image_c(path);
            return new tileset_c(image, size_s(16, 16));
        })},
        { ASSET_TILESET_SPR, asset_manager_c::asset_def_s(asset_c::tileset, 2, "player.iff", [](const asset_manager_c& manager, const char* path) -> asset_c* {
            auto image = new image_c(path,0); // Color #0 is transparent
            return new tileset_c(image, size_s(16, 16));
        })},

        // Level is not loaded at all, dynamically created on demand
        { ASSET_LEVEL, asset_manager_c::asset_def_s(asset_c::tilemap_level, 2, nullptr, [](const asset_manager_c& manager, const char* path) -> asset_c* {
            return create_tilemaplevel();
        })},
    };

    auto &assets = asset_manager_c::shared();
    for (const auto &asset_def : asset_defs) {
        assets.add_asset_def(asset_def.first, asset_def.second);
    }

    assets.preload(1);

    return assets;
}

int main(int argc, const char* argv[]) {
    return  machine_c::with_machine(argc, argv, [] (machine_c& m) {
        // Setup and pre-load all assets
        auto &assets = setup_assets();
        
        
        audio_mixer_c::shared().play(assets.music(ASSET_MUSIC));
        
        // Setup and start the scene
#if 1
        auto main_scene = new fullscreen_scene_c();
#else
        auto main_scene = new tilemap_scene();
#endif
        scene_manager_c::shared().run(main_scene);

        return 0;
    });
}
