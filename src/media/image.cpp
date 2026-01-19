//
//  image.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-11.
//

#include "media/image.hpp"
#include "media/canvas.hpp"
#include "core/iffstream.hpp"
#include <errno.h>

using namespace toybox;

image_c::image_c(const size_s size, bool masked, shared_ptr_c<palette_c> palette) {
    _palette = palette;
    _size = size;
    _line_words = ((size.width + 15) / 16);
    uint16_t bitmap_words = (_line_words * size.height) << 2;
    uint16_t mask_bytes = masked ? (_line_words * size.height) : 0;
    _bitmap.reset(reinterpret_cast<uint16_t*>(_calloc(bitmap_words + mask_bytes, 2)));
    if (masked) {
        _maskmap = _bitmap + bitmap_words;
    } else {
        _maskmap = nullptr;
    }
}

int image_c::get_pixel(point_s at) const {
    if (_size.contains(at)) {
        return imp_get_pixel(at);
    }
    return _maskmap != nullptr ? MASKED_CIDX : 0;
}

int image_c::imp_get_pixel(point_s at) const {
    int word_offset = (at.x / 16) + at.y * _line_words;
    const uint16_t bit = 1 << (15 - at.x & 15);
    if (_maskmap != nullptr) {
        uint16_t* maskmap = _maskmap + word_offset;
        if (!(*maskmap & bit)) {
            return MASKED_CIDX;
        }
    }
    uint8_t ci = 0;
    uint8_t cb = 1;
    uint16_t* bitmap = _bitmap + (word_offset << 2);
    for (int bp = 0; bp < 4; bp++) {
        if (*bitmap++ & bit) {
            ci |= cb;
        }
        cb <<= 1;
    }
    return ci;
}

void image_c::put_pixel(int ci, point_s at) const {
    if (_size.contains(at)) {
        int word_offset = (at.x / 16) + at.y * _line_words;
        const uint16_t bit = 1 << (15 - at.x & 15);
        const uint16_t mask = ~bit;
        if (_maskmap != nullptr) {
            uint16_t* maskmap = _maskmap + word_offset;
            if (image_c::is_masked(ci)) {
                *maskmap &= mask;
                ci = 0;
            } else {
                *maskmap |= bit;
            }
        } else if (image_c::is_masked(ci)) {
            return;
        }
        uint16_t* bitmap = _bitmap + (word_offset << 2);
        uint8_t cb = 1;
        for (int bp = 0; bp < 4; bp++) {
            if (ci & cb) {
                *bitmap++ |= bit;
            } else {
                *bitmap++ &= mask;
            }
            cb <<= 1;
        }
    }
}

namespace cc4 {
    static constexpr cc4_t ILBM("ILBM");
    static constexpr cc4_t BMHD("BMHD");
    static constexpr cc4_t CMAP("CMAP");
    static constexpr cc4_t GRAB("GRAB");
    static constexpr cc4_t BODY("BODY");
}

enum class mask_type_e : uint8_t {
    none,
    plane,
    color,
    lasso,
};

struct ilbm_header_s {
    size_s size;
    point_s offset;
    uint8_t plane_count;
    mask_type_e mask_type;
    image_c::compression_type_e compression_type;
    uint8_t _pad;
    uint16_t mask_color;
    uint8_t aspect[2];
    size_s page_size;
};
static_assert(sizeof(ilbm_header_s) == 20, "Header size mismatch");


namespace toybox {
    template<>
    struct struct_layout<ilbm_header_s> {
        static constexpr const char* value = "4w4b1w2b2w";
    };
}

static void image_read(iffstream_c& file, uint16_t line_words, unsigned height, uint16_t* bitmap, uint16_t* maskmap) {
    uint16_t word_buffer[line_words];
    const int bp_count = (maskmap ? 5 : 4);
    while (height--) {
        for (int bp = 0; bp < bp_count; bp++) {
            uint16_t* buffer;
            if (bp < 4) {
                buffer = (uint16_t*)word_buffer;
            } else {
                buffer = (uint16_t*)maskmap;
            }
            if (!file.read(buffer, line_words)) {
                return; // Failed to read line
            }
            if (bp < 4) {
                for (int i = 0; i < line_words; i++) {
                    bitmap[bp + i * 4] = buffer[i];
                }
            }
        }
        bitmap += line_words * 4;
        if (maskmap) {
            maskmap += line_words;
        }
    }
}

static void image_read_packbits(iffstream_c& file, uint16_t line_words, unsigned height, uint16_t* bitmap, uint16_t* maskmap) {
    const int bp_count = (maskmap ? 5 : 4);
    uint16_t word_buffer[line_words * bp_count];
    while (height--) {
        uint8_t* buffer = (uint8_t*)word_buffer;
        uint8_t* bufferEnd = buffer + (line_words * bp_count * 2);
        while (buffer < bufferEnd) {
            int8_t cmd;
            if (!file.read((uint8_t*)&cmd, 1)) {
                return; // Failed read
            }
            if (cmd >= 0) {
                const int to_read = cmd + 1;
                if (!file.read(buffer, to_read)) {
                    return; // Failed read
                }
                buffer += to_read;
            } else if (cmd != -128) {
                uint8_t data;
                if (!file.read(&data, 1)) {
                    return; // Failed read
                }
                while (cmd++ <= 0) {
                    *buffer++ = data;
                }
            }
        }
        for (int bp = 0; bp < bp_count; bp++) {
            if (bp < 4) {
                for (int i = 0; i < line_words; i++) {
                    bitmap[bp + i * 4] = word_buffer[bp * line_words + i];
                    hton(bitmap[bp + i * 4]);
                }
            } else {
                for (int i = 0; i < line_words; i++) {
                    maskmap[i] = word_buffer[bp * line_words + i];
                    hton(maskmap[i]);
                }
            }
        }
        bitmap += line_words * 4;
        if (maskmap) {
            maskmap += line_words;
        }
    }
}

image_c::image_c(const char* path, int masked_cidx, const iffstream_c::unknown_reader& unknown_reader) :
    _palette(nullptr), _bitmap(nullptr), _maskmap(nullptr), _size(), _line_words(0)
{
    bool masked = false;
    iffstream_c file(path);
    iff_group_s form;
    if (!file.good() || !file.first(cc4::FORM, ::cc4::ILBM, form)) {
        if (errno == 0) {
            errno = EINVAL;
        }
        return; // Not a ILBM
    }
    iff_chunk_s chunk;
    ilbm_header_s bmhd;
    while (file.next(form, cc4::ANY, chunk)) {
        if (chunk.id == ::cc4::BMHD) {
            if (!file.read(&bmhd)) {
                errno = EINVAL;
                return;
            }
            _size = bmhd.size;
            assert(bmhd.plane_count == 4 && "Only 4-plane images are supported");
            if (masked_cidx != MASKED_CIDX) {
                assert(bmhd.mask_type != mask_type_e::plane && "Plane mask type conflicts with custom mask color");
                bmhd.mask_color = masked_cidx;
                masked = true;
            } else if (bmhd.mask_type == mask_type_e::color) {
                masked_cidx = bmhd.mask_color;
                masked = true;
            } else if (bmhd.mask_type == mask_type_e::plane) {
                masked = true;
            } else {
                assert(bmhd.mask_type == mask_type_e::none && "Mask type must be none when not using color or plane masks");
            }
            // DeluxePain ST format and custom deflate not supported
            assert(bmhd.compression_type < compression_type_e::vertical && "DeluxePaint ST vertical compression not supported");
        } else if (chunk.id == ::cc4::CMAP) {
            uint8_t cmpa[48];
            if (file.read(cmpa, 48) != 48) {
                errno = EINVAL;
                return; // Could not read palette
            }
            _palette.reset(new palette_c(&cmpa[0]));
        } else if (chunk.id == ::cc4::BODY) {
            _line_words = ((_size.width + 15) / 16);
            const uint16_t bitmap_words = (_line_words * _size.height) << 2;
            const bool needs_mask_words = masked || (bmhd.mask_type == mask_type_e::plane);
            const uint16_t mask_words = needs_mask_words ? (bitmap_words >> 2) : 0;
            _bitmap.reset((uint16_t*)(_malloc((bitmap_words + mask_words) << 1)));
            assert(_bitmap && "Failed to allocate bitmap memory");
            if (needs_mask_words) {
                _maskmap = _bitmap + bitmap_words;
            } else {
                _maskmap = nullptr;
            }
            switch (bmhd.compression_type) {
                case compression_type_e::none:
                    image_read(file, _line_words, _size.height, _bitmap.get(), bmhd.mask_type == mask_type_e::plane ? _maskmap : nullptr);
                    break;
                case compression_type_e::packbits:
                    image_read_packbits(file, _line_words, _size.height, _bitmap.get(), bmhd.mask_type == mask_type_e::plane ? _maskmap : nullptr);
                    break;
                default:
                    break;
            }
            if (needs_mask_words) {
                if (!masked) {
                    _maskmap = nullptr;
                } else if (bmhd.mask_type != mask_type_e::plane) {
                    memset(_maskmap, -1, mask_words << 1);
                    canvas_c::remap_table_c table;
                    table[bmhd.mask_color] = MASKED_CIDX;
                    canvas_c canvas(*this);
                    canvas.remap_colors(table, rect_s(point_s(), _size));
                }
            }
        } else {
            bool skip = true;
            if (unknown_reader) {
               skip = !unknown_reader(file, chunk);
            }
            if (skip) {
#ifndef __M68000__
                printf("Skipping '%s'\n", chunk.id.cstring());
#endif
                file.skip(chunk);
            }
        }
    }
}


static void image_write(iffstream_c& file, uint16_t line_words, uint16_t next_line_words, unsigned height, uint16_t* bitmap, uint16_t* maskmap) {
    const int bp_count = (maskmap ? 5 : 4);
    uint16_t word_buffer[line_words * bp_count];
    while (height--) {
        for (int bp = 0; bp < bp_count; bp++) {
            if (bp < 4) {
                for (int i = 0; i < line_words; i++) {
                    word_buffer[bp * line_words + i] = bitmap[bp + i * 4];
                }
            } else {
                for (int i = 0; i < line_words; i++) {
                    word_buffer[bp * line_words + i] = maskmap[i];
                }
            }
        }
        file.write(word_buffer);
        
        bitmap += next_line_words * 4;
        if (maskmap) {
            maskmap += next_line_words;
        }
    }
}

static int image_packbits_into_body(uint8_t* body, const uint8_t* row_buffer, int row_byte_count) {
    assert(row_byte_count >= 2 && "Row byte count must be at least 2");
#define PACKBITS_MIN_RUN 3
#define PACKBITS_MAX_BYTES 128
    enum class mode_e : uint8_t {
        dump,
        run
    };
    
    const auto packRunIntoBody = [&body] (uint8_t byte, int count) {
        *body++ = -(count - 1);
        *body++ = byte;
    };
    const auto packDumpIntoBody = [&body] (uint8_t* buf, int count) {
        *body++ = count - 1;
        memcpy(body, buf, count);
        body += count;
    };
    
    uint8_t* const body_begin = body;
    uint8_t current_byte, previous_byte = '\0';
    static uint8_t buffer[256];
    short buffer_byte_count;
    short run_start_buffer_index = 0;
    mode_e mode = mode_e::dump;
    
    buffer[0] = previous_byte = current_byte = *row_buffer++;
    buffer_byte_count = 1;
    row_byte_count--;
    
    for (; row_byte_count > 0;  --row_byte_count) {
        buffer[buffer_byte_count++] = current_byte = *row_buffer++;
        switch (mode) {
            case mode_e::dump:
                if (buffer_byte_count > PACKBITS_MAX_BYTES) {
                    packDumpIntoBody(buffer, buffer_byte_count - 1);
                    buffer[0] = current_byte;
                    buffer_byte_count = 1;
                    run_start_buffer_index = 0;
                    break;
                }
                if (current_byte == previous_byte) {
                    if (buffer_byte_count - run_start_buffer_index >= PACKBITS_MIN_RUN) {
                        if (run_start_buffer_index > 0) {
                            packDumpIntoBody(buffer, run_start_buffer_index);
                        }
                        mode = mode_e::run;
                    }  else if (run_start_buffer_index == 0) {
                        mode = mode_e::run;
                    }
                } else {
                    run_start_buffer_index = buffer_byte_count - 1;
                }
                break;
                
            case mode_e::run:
                if ((current_byte != previous_byte) || (buffer_byte_count - run_start_buffer_index > PACKBITS_MAX_BYTES)) {
                    packRunIntoBody(previous_byte, buffer_byte_count - 1 - run_start_buffer_index);
                    buffer[0] = current_byte;
                    buffer_byte_count = 1;
                    run_start_buffer_index = 0;
                    mode = mode_e::dump;
                }
                break;
        }
        previous_byte = current_byte;
    }
    
    switch (mode) {
        case mode_e::dump:
            packDumpIntoBody(buffer, buffer_byte_count);
            break;
        case mode_e::run:
            packRunIntoBody(previous_byte, buffer_byte_count-run_start_buffer_index);
            break;
    }
    return (int)(body - body_begin);
}

static void image_write_packbits(iffstream_c& file, uint16_t line_words, uint16_t next_line_words, unsigned height, uint16_t* bitmap, uint16_t* maskmap) {
    const int bp_count = (maskmap ? 5 : 4);
    uint16_t word_buffer[line_words * bp_count];
    while (height--) {
        for (int bp = 0; bp < bp_count; bp++) {
            if (bp < 4) {
                for (int i = 0; i < line_words; i++) {
                    word_buffer[bp * line_words + i] = bitmap[bp + i * 4];
                    hton(word_buffer[bp * line_words + i]);
                }
            } else {
                for (int i = 0; i < line_words; i++) {
                    word_buffer[bp * line_words + i] = maskmap[i];
                    hton(word_buffer[bp * line_words + i]);
                }
            }
        }
        uint8_t body[line_words * bp_count * 2 + 32];
        int bytes = image_packbits_into_body(body, (const uint8_t*)word_buffer, line_words * bp_count * 2);
        file.write(body, bytes);
        
        bitmap += next_line_words * 4;
        if (maskmap) {
            maskmap += next_line_words;
        }
    }
}


bool image_c::save(const char* path, image_c::compression_type_e compression, bool masked, int masked_cidx, const iffstream_c::unknown_writer& unknown_writer) {
    // DeluxePain ST format and custom deflate not supported
    assert(compression < compression_type_e::vertical && "DeluxePaint ST vertical compression not supported");

    iffstream_c ilbm(path, fstream_c::openmode_e::input | fstream_c::openmode_e::output);
    if (ilbm.tell() >= 0) {
        iff_group_s form;
        iff_chunk_s chunk;
        ilbm_header_s header;
        ilbm.begin(cc4::FORM, form);
        ilbm.write(&::cc4::ILBM);
        {
            memset(&header, 0, sizeof(ilbm_header_s));
            header.size = _size;
            header.plane_count = 4;
            header.mask_type = masked_cidx != MASKED_CIDX ? mask_type_e::color : (masked && _maskmap) ? mask_type_e::plane : mask_type_e::none;
            header.compression_type = compression;
            if (header.mask_type == mask_type_e::color) {
                header.mask_color = masked_cidx;
            }
            header.aspect[0] = 10;
            header.aspect[1] = 11;
            header.page_size = {320, 200};
            ilbm.begin(::cc4::BMHD, chunk);
            ilbm.write(&header);
            ilbm.end(chunk);
        }
        /* if (_offset.x != 0 || _offset.y != 0) {
         ilbm.begin(::cc4::GRAB, chunk);
         ilbm.write(_offset);
         ilbm.end(chunk);
         } */
        if (_palette) {
            uint8_t cmap[48];
            for (int i = 0; i < 16; i++) {
                (*_palette)[i].get(&cmap[i * 3 + 0], &cmap[i * 3 + 1], &cmap[i * 3 + 2]);
            }
            ilbm.begin(::cc4::CMAP, chunk);
            ilbm.write(cmap, 48);
            ilbm.end(chunk);
        }
        if (unknown_writer) {
            if (!unknown_writer(ilbm)) {
                return false;
            }
        }
        {
            ilbm.begin(::cc4::BODY, chunk);
            switch (compression) {
                case compression_type_e::none:
                    image_write(ilbm, (_size.width + 15) / 16, _line_words, _size.height, _bitmap.get(),  header.mask_type == mask_type_e::plane ? _maskmap : nullptr);
                    break;
                case compression_type_e::packbits:
                    image_write_packbits(ilbm, (_size.width + 15) / 16, _line_words, _size.height, _bitmap.get(), header.mask_type == mask_type_e::plane ? _maskmap : nullptr);
                    break;
#if TOYBOX_ILBM_SUPPORTS_DEFLATE
                case compression_type_e::deflate:
                    image_write_deflate(ilbm, (_size.width + 15) / 16, _line_words, _size.height, _bitmap.get(), header.mask_type == mask_type_plane ? _maskmap : nullptr);
                    break;
#endif
                default:
                    assert(0 && "Unsupported compression type");
                    break;
            }
            ilbm.end(chunk);
        }
        ilbm.end(form);
        return true;
    }
    return false;
}
