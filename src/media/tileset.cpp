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
    _max_tile(image->size().width / tile_size.width, image->size().height / tile_size.height),
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

detail::tileset_header_s s_header;
static expected_c<image_c*> load_image(const char* path, size_s tile_size) {
    s_header = { .tile_size = tile_size, .reserved = {0} };
    auto chunk_handler = [&](iffstream_c& stream, iff_chunk_s& chunk) {
        if (chunk.id == cc4_t("TSHD")) {
            stream.read(&s_header);
            return true;
        }
        return false;
    };
    return new image_c(path, image_c::MASKED_CIDX, chunk_handler);
}

tileset_c::tileset_c(const char* path, size_s tile_size)
{
    auto image = load_image(path, tile_size);
    if (image) {
        construct_at(this, *image, s_header.tile_size);
        copy(&s_header.reserved[0], &s_header.reserved[6], _data.begin());
    } else {
        errno = image.error();
    }
}
