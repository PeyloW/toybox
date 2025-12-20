//
//  font.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-11.
//

#include "media/font.hpp"
#include "core/expected.hpp"

using namespace toybox;

font_c::font_c(const shared_ptr_c<image_c> &image, size_s character_size) : _image(image) {
    const int cols = image->size().width / character_size.width;
    int i;
    do_dbra(i, 95) {
        const int col = i % cols;
        const int row = i / cols;
        _rects[i] = rect_s(
            point_s(col * character_size.width, row * character_size.height),
            character_size
        );
    } while_dbra(i);
}

font_c::font_c(const shared_ptr_c<image_c> &image, size_s max_size, uint8_t space_width, uint8_t lead_req_space, uint8_t trail_req_space) : _image(image)  {
    const int cols = image->size().width / max_size.width;
    int i;
    do_dbra(i, 95) {
        const int col = i % cols;
        const int row = i / cols;
        rect_s rect(point_s(col * max_size.width, row * max_size.height), max_size);
        if (i == 0) {
            rect.size.width = space_width;
        } else {
            // Find first non-empty column, count spaces from top
            {
                for (int fc = 0; fc < rect.size.width; fc++) {
                    for (int fcc = 0; fcc < rect.size.height; fcc++) {
                        const point_s at = point_s(rect.origin.x + fc, rect.origin.y + fcc);
                        if (!image_c::is_masked(image->get_pixel(at))) {
                            fc = MAX(0, fcc >= lead_req_space ? fc : fc - 1);
                            rect.origin.x += fc;
                            rect.size.width -= fc;
                            goto leading_done;
                        }
                    }
                }
                rect.size.width = space_width;
                goto trailing_done;
            }
        leading_done:
            // Find last non-empty column, count spaces from bottom
            {
                const point_s max_at = point_s(rect.max_x(), rect.max_y());
                for (int fc = 0; fc < rect.size.width; fc++) {
                    for (int fcc = 0; fcc < rect.size.height; fcc++) {
                        const point_s at = point_s(max_at.x - fc, max_at.y - fcc);
                        if (!image_c::is_masked(image->get_pixel(at))) {
                            fc = MAX(0, fcc >= trail_req_space ? fc : fc - 1);
                            rect.size.width -= fc;
                            goto trailing_done;
                        }
                    }
                }
            }
        trailing_done: ;
        }
        _rects[i] = rect;
    } while_dbra(i);
}

font_c::font_c(const char* path, size_s character_size) {
    auto image = new expected_c<image_c>(failable, path);
    if (*image) {
        construct_at(this, expected_cast(image), character_size);
    } else {
        errno = image->error();
    }
}
font_c::font_c(const char* path, size_s max_size, uint8_t space_width, uint8_t lead_req_space, uint8_t trail_req_space) {
    auto image = new expected_c<image_c>(failable, path);
    if (*image) {
        construct_at(this, expected_cast(image), max_size, space_width, lead_req_space, trail_req_space);
    } else {
        errno = image->error();
    }
}
