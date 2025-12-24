//
//  canvas.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-10.
//

#include "media/canvas.hpp"
#include "core/vector.hpp"

using namespace toybox;

canvas_c::canvas_c(image_c& image) :
    _image(image), _clip_rect(point_s(), image.size())
{}

void canvas_c::remap_colors(const remap_table_c& table, const rect_s& rect) const {
    assert(rect.contained_by(_image.size()) && "Rect must be contained within image bounds");
    for (int16_t y = rect.origin.y; y < rect.origin.y + rect.size.height; y++) {
        for (int16_t x = rect.origin.x; x < rect.origin.x + rect.size.width; x++) {
            const point_s at(x, y);
            const int c = _image.get_pixel(at);
            const int rc = table[c];
            if (c != rc) {
                _image.put_pixel(rc, at);
            }
        }
    }
}

using with_clipped_rect_f = void(*)(const rect_s& rect, point_s at);
template<typename WithClipped>
static __forceinline bool __with_clipped_rect(canvas_c* canvas, const rect_s& rect, point_s at, WithClipped func) {
    //assert(canvas._clipping);
    rect_s r = rect;
    if (r.clip_to(canvas->clip_rect(), at)) {
        if (!r.size.is_empty()) {
            canvas->with_clipping(false, [&] { func(r, at); });
        }
        return true;
    }
    return false;
}

void canvas_c::fill(uint8_t ci, const rect_s& rect) {
    assert(_image._maskmap == nullptr && "Image must not have a maskmap");
    assert(rect.contained_by(_clip_rect) && "Rect must be contained within canvas bounds");
    if (_clipping) {
        if (__with_clipped_rect(this, rect, rect.origin, [&] (const rect_s& rect, point_s at) {
            fill(ci, rect);
        })) {
            return;
        }
    }
    if (_dirtymap) {
        _dirtymap->mark(rect);
    }
    imp_fill(ci, rect);
}

void canvas_c::draw_aligned(const image_c& src, point_s at) {
    assert((at.x & 0xf) == 0 && "Destination X must be 16-byte aligned");
    assert((src._size.width & 0xf) == 0 && "Source width must be 16-byte aligned");
    assert(_image._maskmap == nullptr && "Canvas image must not have a maskmap");
    assert(src._maskmap == nullptr && "Source image must not have a maskmap");
    rect_s rect(point_s(), src.size());
    draw_aligned(src, rect, at);
}

void canvas_c::draw_aligned(const image_c& src, const rect_s& rect, point_s at) {
    assert((at.x & 0xf) == 0 && "Destination X must be 16-byte aligned");
    assert((rect.origin.x &0xf) == 0 && "Rect origin X must be 16-byte aligned");
    assert((rect.size.width & 0xf) == 0 && "Rect width must be 16-byte aligned");
    assert(_image._maskmap == nullptr && "Canvas image must not have a maskmap");
    assert(src._maskmap == nullptr && "Source image must not have a maskmap");
    if (_clipping) {
        if (__with_clipped_rect(this, rect, at, [&] (const rect_s& rect, point_s at) {
            draw_aligned(src, rect, at);
        })) {
            return;
        }
    }
    if (_dirtymap) {
        const rect_s dirty_rect(at, rect.size);
        _dirtymap->mark(dirty_rect);
    }
    imp_draw_aligned(src, rect, at);
}

void canvas_c::draw_aligned(const tileset_c& src, int idx, point_s at) {
    draw_aligned(*src.image(), src[idx], at);
}

void canvas_c::draw_aligned(const tileset_c& src, point_s tile, point_s at) {
    draw_aligned(*src.image(), src[tile], at);
}

void canvas_c::draw(const image_c& src, point_s at, const int color) {
    assert(_image._maskmap == nullptr && "Canvas must not have maskmap");
    rect_s rect(point_s(), src.size());
    draw(src, rect, at, color);
}

void canvas_c::draw(const image_c& src, const rect_s& rect, point_s at, const int color) {
    assert(_image._maskmap == nullptr && "Canvas image must not have a maskmap");
    assert(rect.contained_by(src.size()) && "Rect must be contained within canvas bounds");
    if (_clipping) {
        if (__with_clipped_rect(this, rect, at, [&] (const rect_s& rect, point_s at) {
            draw(src, rect, at, color);
        })) {
            return;
        }
    }
    if (rect.size.is_empty()) {
        return;
    }
    if (_dirtymap) {
        const rect_s dirty_rect(at, rect.size);
        _dirtymap->mark(dirty_rect);
    }
    if (src._maskmap) {
        if (image_c::is_masked(color)) {
            imp_draw_masked(src, rect, at);
        } else {
            imp_draw_color(src, rect, at, color);
        }
    } else {
        assert(image_c::is_masked(color) && "Color must be masked when source has no maskmap");
        imp_draw(src, rect, at);
    }
}

void canvas_c::draw(const tileset_c& src, int idx, point_s at, const int color) {
    draw(*src.image(), src[idx], at, color);
}

void canvas_c::draw(const tileset_c& src, point_s tile, point_s at, int color) {
    draw(*src.image(), src[tile], at, color);
}

void canvas_c::draw_3_patch(const image_c& src, int16_t cap, const rect_s& in) {
    rect_s rect(point_s(), src.size());
    draw_3_patch(src, rect, cap, in);
}

void canvas_c::draw_3_patch(const image_c& src, const rect_s& rect, int16_t cap, const rect_s& in) {
    assert(in.size.width >= cap * 2 && "Input rect width must be at least twice the cap size");
    assert(rect.size.width > cap * 2 && "Source rect width must be greater than twice the cap size");
    assert(rect.size.height == in.size.height && "Source and input rect heights must match");
    if (_dirtymap) {
        _dirtymap->mark(in);
    }
    const_cast<canvas_c*>(this)->with_dirtymap(nullptr, [&] {
        const rect_s left_rect(rect.origin, size_s(cap, rect.size.height));
        draw(src, left_rect, in.origin);
        const rect_s right_rect(
            rect.origin.x + rect.size.width - cap, rect.origin.y,
            cap, rect.size.height
        );
        const point_s right_at(in.origin.x + in.size.width - cap, in.origin.y);
        draw(src, right_rect, right_at);
        rect_s middle_rect(
            rect.origin.x + cap, rect.origin.y,
            rect.size.width - cap * 2, rect.size.height
        );
        point_s at(in.origin.x + cap, in.origin.y);
        int16_t to_draw = in.size.width - cap * 2;
        while (to_draw > 0) {
            const int16_t width = MIN(to_draw, middle_rect.size.width);
            middle_rect.size.width = width;
            draw(src, middle_rect, at);
            to_draw -= width;
            at.x += width;
        }
    });
}

size_s canvas_c::draw(const font_c& font, const char* text, point_s at, alignment_e alignment, int color) {
    int len = (int)strlen(text);
    size_s size = font.char_rect(' ').size;
    size.width = 0;
    if (len == 0) return size;
    int i;
    do_dbra(i, len - 1) {
        size.width += font.char_rect(text[i]).size.width;
    } while_dbra(i);
    switch (alignment) {
        case alignment_e::left:
            at.x += size.width;
            break;
        case alignment_e::center:
            at.x += size.width / 2;
            break;
        default:
            break;
    }
    if (_dirtymap) {
        rect_s dirty_rect(point_s(at.x - size.width, at.y), size);
        _dirtymap->mark(dirty_rect);
    }
    const_cast<canvas_c*>(this)->with_dirtymap(nullptr, [&] {
        do_dbra(i, len - 1) {
            const rect_s& rect = font.char_rect(text[i]);
            at.x -= rect.size.width;
            draw(*font.image(), rect, at, color);
        } while_dbra(i);
    });
    return size;
}

#define MAX_LINES 8
static char draw_text_buffer[80 * MAX_LINES];

size_s canvas_c::draw(const font_c& font, const char* text, const rect_s& in, uint16_t line_spacing, alignment_e alignment, int color) {
    strcpy(draw_text_buffer, text);
    vector_c<const char*, 12> lines;

    uint16_t line_width = 0;
    int start = 0;
    int last_good_pos = 0;
    bool done = false;
    for (int i = 0; !done; i++) {
        bool emit = false;
        const char c = text[i];
        if (c == 0) {
            last_good_pos = i;
            emit = true;
            done = true;
        } else if (c == ' ') {
            last_good_pos = i;
        } else if (c == '\n') {
            last_good_pos = i;
            emit = true;
        }
        if (!emit) {
            line_width += font.char_rect(text[i]).size.width;
            if (line_width  > in.size.width) {
                emit = true;
            }
        }
        
        if (emit) {
            draw_text_buffer[last_good_pos] = 0;
            lines.push_back(draw_text_buffer + start);
            line_width = 0;
            start = last_good_pos + 1;
            i = start;
        }
    }
    point_s at;
    switch (alignment) {
        case alignment_e::left: at = in.origin; break;
        case alignment_e::center: at = point_s( in.origin.x + in.size.width / 2, in.origin.y); break;
        case alignment_e::right: at = point_s( in.origin.x + in.size.width / 2, in.origin.y); break;
    }
    size_s max_size = {0,0};
    bool first = true;
    for (auto&line : lines) {
        const auto size = draw(font, line, at, alignment, color);
        at.y += size.height + line_spacing;
        max_size.width = MAX(max_size.width, size.width);
        max_size.height += size.height + (!first ? line_spacing : 0);
        first = false;
    }
    return  max_size;
}

void canvas_c::fill_tile(uint8_t ci, point_s at) {
    assert(_tileset_line_words != 0 && "fill_tile must be called within with_tileset()");
    assert((at.x & 0xf) == 0 && "Tile must be aligned to 16px boundary");
    assert(rect_s(at, size_s(16, 16)).contained_by(_clip_rect) && "Tile must be within canvas bounds");
    imp_fill_tile(ci, at);
}

void canvas_c::draw_tile(const tileset_c& src, int idx, point_s at) {
    assert(_tileset_line_words != 0 && "draw_tile must be called within with_tileset()");
    assert(src.image()->_line_words == _tileset_line_words && "Tileset must match with_tileset() tileset");
    assert((at.x & 0xf) == 0 && "Tile must be aligned to 16px boundary");
    assert(rect_s(at, size_s(16, 16)).contained_by(_clip_rect) && "Tile must be within canvas bounds");
    imp_draw_tile(*src.image(), src[idx], at);
}

void canvas_c::draw_tile(const tileset_c& src, point_s tile, point_s at) {
    assert(_tileset_line_words != 0 && "draw_tile must be called within with_tileset()");
    assert(src.image()->_line_words == _tileset_line_words && "Tileset must match with_tileset() tileset");
    assert((at.x & 0xf) == 0 && "Tile must be aligned to 16px boundary");
    assert(rect_s(at, size_s(16, 16)).contained_by(_clip_rect) && "Tile must be within canvas bounds");
    imp_draw_tile(*src.image(), src[tile], at);
}
