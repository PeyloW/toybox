//
//  tileset.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-17.
//

#pragma once

#include "media/image.hpp"

namespace toybox {
    
    /**
     A `tileset_c` is a convenience wrapper on top of `image_c` for handling a
     a set of equally sized tiles.
     Tiles can be indexed as column/row using a `point_t`, or by a continuous
     index.
     */
    class tileset_c : public asset_c {
    public:
        tileset_c() = delete;
        tileset_c(const char* path);
        tileset_c(const char* path, size_s tile_size);
        tileset_c(const char* path, const vector_c<rect_s, 0>& rects);
        tileset_c(const shared_ptr_c<image_c>& image, size_s tile_size);
        tileset_c(const shared_ptr_c<image_c>& image, const vector_c<rect_s, 0>& rects);
        virtual ~tileset_c() {}

        __forceinline type_e asset_type() const override final __pure { return tileset; }

        __forceinline const shared_ptr_c<image_c>& image() const_pure {
            return _image;
        }

        int16_t max_index() const_pure { return _rects.size(); }

        __forceinline const rect_s& operator[](int i) const_pure {
            assert(i >= 0 && i < max_index() && "Tile index out of bounds");
            __assume_count(i, 0x8000 / sizeof(rect_s));
            return _rects[i];
        }

        bool is_uniform() const_pure { return _uniform; };
        __forceinline size_s tile_size() const_pure { return _rects[0].size; }
        
        __forceinline array_s<uint16_t, 6>& data() { return _data; }
        __forceinline const array_s<uint16_t, 6>& data() const_pure { return _data; }

        template<typename T> requires (sizeof(T) <= 12)
        T& data_as() { return (T&)(_data[0]); }
        template<typename T> requires (sizeof(T) <= 12)
        const T& data_as() const { return (const T&)(_data[0]); }
    private:
        const shared_ptr_c<image_c> _image;
        vector_c<rect_s, 0> _rects;
        array_s<uint16_t, 6> _data;
        bool _uniform;
    };

    namespace detail {
        // EA IFF 85 chunk IDs
        namespace cc4 {
            static constexpr toybox::cc4_t TSHD("TSHD");
        }
        // Tileset header for EA IFF 85 TSHD chunk (inside ILBM)
        // If tile_size is non-zero, tiles are uniform.
        // If tile_size is {0,0}, (chunk.size - 16) / 8 rect_s entries follow.
        struct tileset_header_s {
            size_s tile_size;           // Fixed size of tiles, or {0,0} for variable rects
            uint16_t reserved[6];       // Custom data
        };
        static_assert(sizeof(tileset_header_s) == 16);
    }
    template<>
    struct struct_layout<detail::tileset_header_s> {
        static constexpr const char* value = "8w";
    };
    
}
