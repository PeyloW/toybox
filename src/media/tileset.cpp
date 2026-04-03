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
    _max_tile((uint16_t)image->size().width / tile_size.width, (uint16_t)image->size().height / tile_size.height),
    _max_index(_max_tile.x * _max_tile.y),
    _rects(), _data()
{
    assert(_max_tile.x > 0 && _max_tile.y > 0 && "Tileset must have at least one tile");
    _rects.reset((rect_s*)_malloc(sizeof(rect_s) * max_index()));
    int i = 0;
    for (int y = 0; y < _max_tile.y; y++) {
        for (int x = 0; x < _max_tile.x; x++) {
            _rects[i] = rect_s(
                x * tile_size.width, y * tile_size.height,
                tile_size.width, tile_size.height
            );
            i++;
        }
    }
}

tileset_c::tileset_c(const char* path, size_s tile_size)
{
    detail::tileset_header_s header = { .tile_size = tile_size, .reserved = {0} };
    auto chunk_handler = [&](iffstream_c& stream, iff_chunk_s& chunk) {
        if (chunk.id == cc4_t("TSHD")) {
            stream.read(&header);
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
