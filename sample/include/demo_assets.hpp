//
//  demo_assets.hpp
//  toybox - sample
//
//  Created by Fredrik on 2025-10-14.
//

#pragma once

#include "runtime/assets.hpp"

using namespace toybox;

enum demo_assets_e {
// Shared
    
// Used by fullscreen_scene
    ASSET_BACKGROUND, // image
    ASSET_SPRITES,    // tileset
    ASSET_MUSIC,      // music
// Used by tilemap_scene
    ASSET_TILESET_WALL,
    ASSET_TILESET_SPR,// Tileset for sprites
    ASSET_LEVEL       // tilemap level
};
