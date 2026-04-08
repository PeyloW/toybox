//
//  tileset.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-17.
//

#include "media/tileset.hpp"
#include "core/expected.hpp"

using namespace toybox;


tileset_c::tileset_c(const shared_ptr_c<image_c> &image, size_s tile_size) :
    _image(image),
    _rects(), _data()
{
    point_s max_tile((uint16_t)_image->size().width / tile_size.width, (uint16_t)_image->size().height / tile_size.height);
    assert(max_tile.x > 0 && max_tile.y > 0 && "Tileset must have at least one tile");
    _rects.reserve(max_tile.x * max_tile.y);
    for (int y = 0; y < max_tile.y; y++) {
        for (int x = 0; x < max_tile.x; x++) {
            _rects.emplace_back(x * tile_size.width, y * tile_size.height,
                                tile_size.width, tile_size.height);
        }
    }
    _uniform = true;
}

tileset_c::tileset_c(const shared_ptr_c<image_c> &image, const vector_c<rect_s, 0>& rects) :
    _image(image),
    _rects(rects), _data()
{
    assert(_rects.size() > 0 && "Tileset must have at least one tile rect");
    _uniform = true;
    for (int i = 1; i < _rects.size(); i++) {
        if (_rects[i].size != _rects[0].size) {
            _uniform = false;
            break;
        }
    }
}

// Helper: read TSHD chunk, including optional trailing rect_s entries
static bool read_tshd(iffstream_c& stream, iff_chunk_s& chunk,
                       detail::tileset_header_s& header,
                       vector_c<rect_s, 0>& rects) {
    stream.read(&header);
    if (header.tile_size.is_empty()) {
        // Variable rects follow the header in the chunk
        int rect_count = (chunk.size - sizeof(detail::tileset_header_s)) / sizeof(rect_s);
        assert(rect_count > 0 && "TSHD with zero tile_size must contain rect entries");
        rects.reserve(rect_count);
        for (int i = 0; i < rect_count; i++) {
            rect_s r;
            stream.read((uint16_t*)&r, sizeof(rect_s) / sizeof(uint16_t));
            rects.emplace_back(r);
        }
    }
    return true;
}

tileset_c::tileset_c(const char* path)
{
    detail::tileset_header_s header = { .tile_size = {0, 0}, .reserved = {0} };
    vector_c<rect_s, 0> tshd_rects;
    bool found_tshd = false;
    auto chunk_handler = [&](iffstream_c& stream, iff_chunk_s& chunk) {
        if (chunk.id == cc4_t("TSHD")) {
            found_tshd = read_tshd(stream, chunk, header, tshd_rects);
            return true;
        }
        return false;
    };
    auto image = new expected_c<image_c>(failable, path, image_c::MASKED_CIDX, chunk_handler);
    hard_assert(found_tshd && "Tileset file must contain a TSHD chunk");
    if (*image) {
        if (tshd_rects.size() > 0) {
            construct_at(this, expected_cast(image), tshd_rects);
        } else {
            construct_at(this, expected_cast(image), header.tile_size);
        }
        copyn(&header.reserved[0], 6, _data.begin());
    } else {
        errno = image->error();
    }
}

tileset_c::tileset_c(const char* path, size_s tile_size)
{
    detail::tileset_header_s header = { .tile_size = tile_size, .reserved = {0} };
    vector_c<rect_s, 0> tshd_rects;
    auto chunk_handler = [&](iffstream_c& stream, iff_chunk_s& chunk) {
        if (chunk.id == cc4_t("TSHD")) {
            read_tshd(stream, chunk, header, tshd_rects);
            hard_assert(header.tile_size == tile_size && "TSHD tile_size does not match");
            return true;
        }
        return false;
    };
    auto image = new expected_c<image_c>(failable, path, image_c::MASKED_CIDX, chunk_handler);
    if (*image) {
        construct_at(this, expected_cast(image), header.tile_size);
        copyn(&header.reserved[0], 6, _data.begin());
    } else {
        errno = image->error();
    }
}
tileset_c::tileset_c(const char* path, const vector_c<rect_s, 0>& rects)
{
    detail::tileset_header_s header = { .tile_size = {0, 0}, .reserved = {0} };
    vector_c<rect_s, 0> tshd_rects;
    auto chunk_handler = [&](iffstream_c& stream, iff_chunk_s& chunk) {
        if (chunk.id == cc4_t("TSHD")) {
            read_tshd(stream, chunk, header, tshd_rects);
            // If TSHD has rects, they must match the provided rects
            if (tshd_rects.size() > 0) {
                hard_assert(rects.size() == tshd_rects.size() && "TSHD rect count does not match");
                for (int i = 0; i < tshd_rects.size(); i++) {
                    hard_assert(tshd_rects[i].origin == rects[i].origin && "TSHD rect origin does not match");
                    hard_assert(tshd_rects[i].size == rects[i].size && "TSHD rect size does not match");
                }
            } else {
                // Uniform TSHD — tile_size must be consistent with all rects
                hard_assert(header.tile_size.width > 0 && header.tile_size.height > 0 && "TSHD has no tile_size and no rects");
                for (int i = 0; i < rects.size(); i++) {
                    hard_assert(rects[i].size == header.tile_size && "TSHD tile_size does not match rect size");
                }
            }
            return true;
        }
        return false;
    };
    auto image = new expected_c<image_c>(failable, path, image_c::MASKED_CIDX, chunk_handler);
    if (*image) {
        construct_at(this, expected_cast(image), rects);
        copyn(&header.reserved[0], 6, _data.begin());
    } else {
        errno = image->error();
    }
}

