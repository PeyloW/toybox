//
//  tilemap.cpp
//  toybox
//
//  Created by Fredrik on 2025-11-09.
//

#include "runtime/tilemap.hpp"

using namespace toybox;

tilemap_c::tilemap_c(const rect_s& tilespace_bounds) :
    _tilespace_bounds(tilespace_bounds)
{
    _tiles.resize(_tilespace_bounds.size.width * _tilespace_bounds.size.height);
    if (!tilespace_bounds.size.is_empty()) {
        _mul_table.reset(create_multable(_tilespace_bounds.size.height, _tilespace_bounds.size.width));
    }
}
