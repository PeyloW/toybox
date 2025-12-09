//
//  viewport.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-17.
//

#include "media/viewport.hpp"
#include "machine/machine.hpp"

using namespace toybox;

static int16_t multof16(int16_t v) {
    return (v + 15) & ~0xf;
}

static size_s fixed_viewport_size(size_s viewport_size) {
    return size_s(
        // Width is min 320, and max 336 which is enough for HW scroll
        min(viewport_c::max_size.width, max(viewport_c::min_size.width, multof16(viewport_size.width))),
        // Height is nearest multiple of 16, + 8 for HW scroll
        min(viewport_c::max_size.height, max(viewport_c::min_size.height, multof16(viewport_size.height)))
    );
}

size_s viewport_c::backing_size(size_s viewport_size) {
    viewport_size = fixed_viewport_size(viewport_size);
    return size_s(
        // Width is min 320, and max 336 which is enough for HW scroll
        min(viewport_size.width, (int16_t)336),
        // Height is nearest multiple of 16, + 8 for HW scroll
        viewport_size.height + 6 // ~(2032-320)/320 extra lines
    );
}

detail::viewport_image_holder::viewport_image_holder(size_s viewport_size) :
    _viewport_size(fixed_viewport_size(viewport_size)),
    _backing_image(viewport_c::backing_size(viewport_size), false, nullptr)
{}

// View port size is the total potential area, the image only what is needed to support hardware scrolling that area.
viewport_c::viewport_c(size_s viewport_size) :
    detail::viewport_image_holder(viewport_size),
    canvas_c(_backing_image)
{
    assert(image().size().width >= 320 && image().size().width <= 336);
    assert(image().size().height >= _viewport_size.height && "Image height must fit viewport");
    assert(_viewport_size.contained_by(max_size));
    assert(min_size.contained_by(_viewport_size));
    _clip_rect = rect_s(_offset, size_s(image().size().width, _viewport_size.height));
    _dirtymap = dirtymap_c::create(_viewport_size);
    _dirtymap->clear();
    _dirtymap->mark(_clip_rect);
#if TOYBOX_DEBUG_DIRTYMAP
        _dirtymap->print_debug("viewport_c::viewport_c()");
#endif
    assert(_dirtymap->dirty_bounds().contained_by(_clip_rect) && "Dirty bounds must fit clip rect");
}

viewport_c::~viewport_c() {
    assert(_dirtymap && "Dirtymap must not be null");
    _free(_dirtymap);
    _dirtymap = nullptr;
}

void viewport_c::set_offset(point_s offset) {
    assert(offset.y == 0 && "Vertical offset must be 0");
    const auto screen_width = image().size().width;
    offset.x = max((int16_t)0, min((int16_t)(_viewport_size.width - screen_width), offset.x));
    const int16_t old_left_tile = _offset.x >> 4;
    const int16_t new_left_tile = offset.x >> 4;
    
    if (new_left_tile != old_left_tile) {
        const int16_t tile_delta = new_left_tile - old_left_tile;
        rect_s mark_rect(
            min(old_left_tile, new_left_tile) << 4, 0,
            abs(tile_delta) << 4, _viewport_size.height
        );
        if (tile_delta > 0) {
            // Scrolling right: mark right
            //_dirtymap->mark<dirtymap_c::mark_type_e::clean>(mark_rect);
            mark_rect.origin.x += screen_width;
            _dirtymap->mark(mark_rect);
        } else {
            // Scrolling left: mark left
            const int16_t left_tiles_gained = -tile_delta;
            _dirtymap->mark(mark_rect);
            //mark_rect.origin.x += screen_width;
            //_dirtymap->mark<dirtymap_c::mark_type_e::clean>(mark_rect);
        }
        _clip_rect = rect_s(
            offset.x & ~0xf, 0,
            screen_width,_viewport_size.height
        );
#if TOYBOX_DEBUG_DIRTYMAP
        _dirtymap->print_debug("viewport::set_offset()");
#endif
        //assert(_dirtymap->dirty_bounds().contained_by(_clip_rect));
    }
    _offset = offset;
}

const detail::display_config_t viewport_c::display_config() const {
    uint8_t extra = _image.size().width > 320 ? 4 : 0;
    detail::display_config_t config{
        _image._bitmap + ((_offset.x >> 4) << 2),// _bitmap_start
        (uint8_t)((_offset.x & 0xf) ? 0 : extra),// Add 4 extra words per line if shift is 0, if needed
        (uint8_t)(_offset.x & 0xf)               // Pixel shift
    };
    return config;
}
