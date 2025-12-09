//
//  viewport.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-17.
//

#pragma once

#include "media/canvas.hpp"
#include "media/display_list.hpp"

namespace toybox {

    namespace detail {
        struct display_config_t {
            uint16_t* bitmap_start;
            uint8_t extra_words;
            uint8_t pixel_shift;
        };
        
        // C++ constructs superclass before members, we need the image to be constructed before the canvas.
        struct viewport_image_holder {
            viewport_image_holder(size_s viewport_size);
            size_s _viewport_size;
            image_c _backing_image;
        };
    }
     
    /*
     A `viewport_c` is an abstraction for displaying a viewport of content.
     Contains an `image_c` for the bitmap data, and a `dirtymap_c` to restore
     dirty areas.
     */
    static_assert(!is_polymorphic<canvas_c>::value);
    class viewport_c : public display_item_c, private detail::viewport_image_holder, public canvas_c {
        friend class machine_c;
    public:
        static constexpr size_s min_size = size_s(320, 208);
        static constexpr size_s max_size = size_s(2032, 208);

        static size_s backing_size(size_s viewport_size);
        
        __forceinline type_e display_type() const override { return viewport; }
        
        viewport_c(size_s viewport_size = min_size);
        ~viewport_c();

        point_s offset() const { return _offset; }
        void set_offset(point_s offset);

        __forceinline dirtymap_c* dirtymap() { return _dirtymap; }
        __forceinline const dirtymap_c* dirtymap() const { return _dirtymap; }

    private:
        const detail::display_config_t display_config() const;
        point_s _offset;
    };

}
