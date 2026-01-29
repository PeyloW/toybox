//
//  font.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-11.
//

#pragma once

#include "media/image.hpp"
#include "core/memory.hpp"

namespace toybox {
    
    using namespace toybox;
    
    /**
     A `font_c` is a convenience wrapper of an `image_c` for managing a bitmap
     font.
     A font can be monospaced or have variable width characters.
     */
    class font_c final : public asset_c {
    public:
        font_c(const shared_ptr_c<image_c>& image, size_s character_size);
        font_c(const shared_ptr_c<image_c>& image, size_s max_size, uint8_t space_width, uint8_t lead_req_space, uint8_t trail_req_space);
        font_c(const char* path, size_s character_size);
        font_c(const char* path, size_s max_size, uint8_t space_width, uint8_t lead_req_space, uint8_t trail_req_space);
        virtual ~font_c() {};

        __forceinline type_e asset_type() const override { return font; }
        
        __forceinline const shared_ptr_c<image_c>& image() const {
            return _image;
        }
        const rect_s& char_rect(char c) const {
            if (c < 32 || c > 127) {
                return _rects[0];
            } else {
                short i = c - 32;
                __assume_count(i, 96);
                return _rects[i];
            }
        }
        
    private:
        const shared_ptr_c<image_c> _image;
        rect_s _rects[96];
    };
       
}
