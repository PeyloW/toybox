//
//  graphics_dirtymap.cpp
//  toybox
//
//  Created by Fredrik on 2024-03-18.
//

#include "media/dirtymap.hpp"
#include "media/canvas.hpp"

using namespace toybox;

static constexpr int16_t LOOKUP_SIZE = 256;
struct  bitrun_list_s {
    int16_t num_runs;
    struct bitrun_s {
        int16_t start;
        int16_t length;
    } bit_runs[4];
};
static_assert(sizeof(bitrun_list_s) == 18, "bitrun_list_s size mismatch");
static bitrun_list_s* lookup_table[LOOKUP_SIZE];
static void init_lookup_table_if_needed() {
    static bitrun_list_s _lookup_buffer[LOOKUP_SIZE];
    static bool is_initialized = false;
    if (is_initialized) {
        return;
    }
    is_initialized = true;
    for (int16_t input = 0; input < LOOKUP_SIZE; input++) {
        bitrun_list_s* result = &_lookup_buffer[input];
        lookup_table[input] = result;
        result->num_runs = 0;
        // Iterate through each bit in the input
        for (int16_t i = 0; i < 8; ) {
            // If the current bit is 1, start a new run
            if ((input >> i) & 1) {
                int16_t start = i * dirtymap_c::tile_size.width;
                int16_t length = dirtymap_c::tile_size.width;
                result->bit_runs[result->num_runs].start = start;
                while ((input >> (i + 1)) & 1) {
                    length += dirtymap_c::tile_size.width;
                    i++;
                }
                result->bit_runs[result->num_runs].length = length;
                result->num_runs++;
            }
            i++;
        }
    }
}

static __forceinline uint8_t __line_bytes(size_s tilespace_size) {
    return (tilespace_size.width + 8) / 8;
}

static __forceinline size_s __tilespace_size(size_s size) {
    return size_s(
        (size.width + dirtymap_c::tile_size.width - 1) / dirtymap_c::tile_size.width,
        (size.height + dirtymap_c::tile_size.height - 1) / dirtymap_c::tile_size.height
    );
}

__forceinline static int __instance_size(size_s size) {
    init_lookup_table_if_needed();
    const auto tilespace_size = __tilespace_size(size);
    const auto line_bytes = __line_bytes(size);
    const int data_size = line_bytes * (size.height + 1) + 3;
    return sizeof(dirtymap_c) + data_size;
}

dirtymap_c* dirtymap_c::create(size_s size) {
    assert(size.width > 0 && size.height > 0);
    assert(offsetof(dirtymap_c, _data) % 1 == 0);
    int bytes = __instance_size(size);
    return new (_calloc(1, bytes)) dirtymap_c(size);
}

dirtymap_c::dirtymap_c(const size_s size) :
    _tilespace_size(__tilespace_size(size)), _line_bytes(__line_bytes(_tilespace_size)), _is_dirty(false)
{
#if TOYBOX_DEBUG_DIRTYMAP
    this->print_debug("dirtymap_c::dirtymap_c()");
#endif
}

template<dirtymap_c::mark_type_e mark_type>
void dirtymap_c::mark(const rect_s& rect) {
    assert(rect.origin.x >= 0 && rect.origin.y >= 0 && "Must mark in dirtymap bounds");
    assert(__tilespace_size(size_s(rect.max_x(), rect.max_y())).width <= _tilespace_size.width);
    assert(__tilespace_size(size_s(rect.max_x(), rect.max_y())).height <= _tilespace_size.height);
    if constexpr (mark_type == mark_type_e::mask) {
        assert((rect.origin.x & 0xf) == 0 && (rect.origin.y & 0xf) == 0);
        assert((rect.size.width & 0xf) == 0 && (rect.size.width & 0xf) == 0);
        const size_s size = this->size();
        // TODO: Clear bytes directly for top and bottom
        if (rect.origin.y > 0) {
            rect_s r(0,0,size.width, rect.origin.y);
            mark<mark_type_e::clean>(r);
        }
        if (rect.max_y() + 1 < size.height) {
            rect_s r(0,rect.max_y() + 1,size.width, size.height - (rect.max_y() + 1));
            mark<mark_type_e::clean>(r);
        }
        if (rect.origin.x > 0) {
            rect_s r(0,rect.origin.y,rect.origin.x, rect.size.height);
            mark<mark_type_e::clean>(r);
        }
        if (rect.max_x() + 1 < size.width) {
            rect_s r(rect.max_x() + 1,rect.origin.y,size.width - (rect.max_x() + 1), rect.size.height);
            mark<mark_type_e::clean>(r);
        }
        return;
    }
    const int16_t x1 = rect.origin.x / tile_size.width;
    const int16_t x2 = (rect.max_x()) / tile_size.width;
    const int16_t y1 = rect.origin.y / tile_size.height;
    assert(y1 < _tilespace_size.height && "Y coordinate must be within dirtymap height");
    static constexpr uint8_t s_first_byte_masks[8] = {
        0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80
    };
    static constexpr uint8_t s_last_byte_masks[8] = {
        0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF
    };

    const int16_t extra_rows = ((rect.origin.y + rect.size.height - 1) / tile_size.height - y1);
    assert(y1 + extra_rows < _tilespace_size.height && "Y extent must be within dirtymap height");
    const int16_t start_byte = x1 / 8;
    assert(start_byte < _line_bytes && "Start byte must be within dirtymap width");
    const int16_t end_byte = x2 / 8;
    assert(end_byte < _line_bytes && "End byte must be within dirtymap width");
    const int16_t start_bit = x1 % 8;
    assert(start_bit < 8 && "Start bit must be less than 8");
    const int16_t end_bit = x2 % 8;
    assert(end_bit < 8 && "End bit must be less than 8");
    
    _is_dirty = true;
    uint8_t* data = _data + (start_byte + _line_bytes * y1);
    uint8_t first_byte_mask = s_first_byte_masks[start_bit];
    uint8_t last_byte_mask = s_last_byte_masks[end_bit];
    if constexpr (mark_type == mark_type_e::clean) {
        first_byte_mask = ~first_byte_mask;
        last_byte_mask = ~last_byte_mask;
    }
    
    if (extra_rows == 0) {
        if (start_byte == end_byte) {
            if constexpr (mark_type == mark_type_e::dirty) {
                *data |= (first_byte_mask & last_byte_mask);
            } else {
                *data &= (first_byte_mask | last_byte_mask);
            }
        } else {
            const int mid_bytes = end_byte - start_byte - 1;
            if constexpr (mark_type == mark_type_e::dirty) {
                *data++ |= first_byte_mask;
                for (int j = 0; j < mid_bytes; j++) {
                    *data++ = 0xff;
                }
                *data |= last_byte_mask;
            } else {
                *data++ &= first_byte_mask;
                for (int j = 0; j < mid_bytes; j++) {
                    *data++ = 0x00;
                }
                *data &= last_byte_mask;
            }
        }
    } else {
        if (start_byte == end_byte) {
            for (int16_t y = 0; y <= extra_rows; y++) {
                if constexpr (mark_type == mark_type_e::dirty) {
                    *data |= (first_byte_mask & last_byte_mask);
                } else {
                    *data &= (first_byte_mask | last_byte_mask);
                }
                data += _line_bytes;
            }
        } else {
            const int mid_bytes = end_byte - start_byte - 1;
            for (int16_t y = 0; y <= extra_rows; y++) {
                auto line_data = data;
                if constexpr (mark_type == mark_type_e::dirty) {
                    *line_data++ |= first_byte_mask;
                    for (int j = 0; j < mid_bytes; j++) {
                        *line_data++ = 0xff;
                    }
                    *line_data |= last_byte_mask;
                } else {
                    *line_data++ &= first_byte_mask;
                    for (int j = 0; j < mid_bytes; j++) {
                        *line_data++ = 0x00;
                    }
                    *line_data &= last_byte_mask;
                }
                data += _line_bytes;
            }
        }
    }
}
template void dirtymap_c::mark<dirtymap_c::mark_type_e::dirty>(const rect_s& rect);
template void dirtymap_c::mark<dirtymap_c::mark_type_e::clean>(const rect_s& rect);
template void dirtymap_c::mark<dirtymap_c::mark_type_e::mask>(const rect_s& rect);

void dirtymap_c::merge(const dirtymap_c& __restrict dirtymap) {
    assert(this != &dirtymap && "Merging dirtymap with self");
    assert(_tilespace_size.width == dirtymap._tilespace_size.width);   // Widths must match
    assert(_tilespace_size.height >= dirtymap._tilespace_size.height); // Other height may be smaller
    if (!dirtymap.is_dirty()) return;
    _is_dirty = true;
    uint32_t* l_dest = (uint32_t*)_data;
    const uint32_t* l_source = (uint32_t*)dirtymap._data;
    const int16_t long_count = (dirtymap._line_bytes * dirtymap._tilespace_size.height + 3) / 4;
    for (int i = 0; i < long_count; i++) {
        const uint32_t v = *l_source++;
        if (v) {
            *l_dest++ |= v;
        } else {
            l_dest++;
        }
    };
}

void dirtymap_c::restore(canvas_c& canvas, const image_c& clean_image) {
    // No check for dirty here, rely on restore(func) to handle this.
    auto& image = canvas.image();
    assert(image.size() == clean_image.size() && "Canvas and clean image sizes must match");
    assert(size().width == clean_image.size().width && "Dirtymap size must match image size");
    assert(size().height <= clean_image.size().height && "Dirtymap size must match image size");
    assert((size().width % tile_size.width) == 0 && "Image width must be a multiple of tile width");
    assert((size().height % tile_size.height) == 0 && "Image height must be a multiple of tile height");
    const_cast<canvas_c&>(canvas).with_clipping(false, [&] {
        canvas.with_dirtymap(nullptr, [&] {
            auto draw = [&](const rect_s& rect) {
                canvas.draw_aligned(clean_image, rect, rect.origin);
            };
            restore_f func(draw);
            restore(func);
        });
    });
}

void dirtymap_c::restore(function_c<void(const rect_s&)>& func) {
    if (!_is_dirty) return;
    _is_dirty = false;
#if TOYBOX_DEBUG_DIRTYMAP
    static uint32_t s_restore_generation = 0;
    s_restore_generation++;
#endif
    auto data = _data;
    point_s at = {0, 0};
    for (int row = 0; row < _tilespace_size.height; row++) {
        for (int col = 0; col < _line_bytes; col++) {
            const uint8_t byte = *data;
            if (byte) {
                const int16_t height = [&] {
                    int16_t height = 0;
                    do {
                        data[height * _line_bytes] = 0;
                        height++;
                    } while (data[height * _line_bytes] == byte);
                    return height * tile_size.height;
                }();
                auto bitrunlist = lookup_table[(int16_t)byte];
                int16_t* bitrun = (int16_t*)bitrunlist->bit_runs;
                for (int r = 0; r < bitrunlist->num_runs; r++) {
                    rect_s rect;
                    rect.origin = point_s(at.x + *bitrun++, at.y);
                    rect.size = size_s(*bitrun++, height);
#if TOYBOX_DEBUG_DIRTYMAP
                    printf("Restore [%u] {{%d, %d}, {%d, %d}}\n", s_restore_generation, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
#endif
                    func(rect);
                }
            }
            data++;
            at.x += tile_size.width * 8;
        }
        at.x = 0;
        at.y += tile_size.height;
    }
}

void dirtymap_c::clear() {
    _is_dirty = false;
    memset(_data, 0, _line_bytes * (_tilespace_size.height + 1));
}

rect_s dirtymap_c::dirty_bounds() const {
    int16_t minX = _tilespace_size.width;
    int16_t minY = _tilespace_size.height;
    int16_t maxX = 0;
    int16_t maxY = 0;
    for (int16_t y = 0; y < _tilespace_size.height; ++y) {
        for (int16_t x = 0; x < _tilespace_size.width; ++x) {
            int byte = (x / 8) + y * _line_bytes;
            uint8_t bit = 1 << (x & 0x7);
            if ((_data[byte] & bit) != 0) {
                minX = min(minX, x);
                maxX = max(maxX, x);
                minY = min(minY, y);
                maxY = max(maxY, y);
            }
        }
    }
    if (minX >= maxX) {
        return rect_s();
    } else {
        return rect_s(minX * 16, minY * 16, (maxX - minX + 1) * 16, (maxY - minY + 1) * 16);
    }
}

void dirtymap_c::print_debug(const char* name) const {
    printf("Dirtymap %d columns is %s [%s]\n", _tilespace_size.width, _is_dirty ? "dirty" : "clean", name);
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
    ((byte) & 0x01 ? '1' : '0'), \
    ((byte) & 0x02 ? '1' : '0'), \
    ((byte) & 0x04 ? '1' : '0'), \
    ((byte) & 0x08 ? '1' : '0'), \
    ((byte) & 0x10 ? '1' : '0'), \
    ((byte) & 0x20 ? '1' : '0'), \
    ((byte) & 0x40 ? '1' : '0'), \
    ((byte) & 0x80 ? '1' : '0')
    auto data = _data;
    for (int row = 0; row < _tilespace_size.height; row++) {
        for (int col = 0; col < _line_bytes; col++) {
            const auto byte = *data++;
            printf(BYTE_TO_BINARY_PATTERN,  BYTE_TO_BINARY(byte));
        }
        printf("\n");
    }
}
