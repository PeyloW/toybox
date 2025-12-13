//
//  tileset.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-17.
//

#pragma once

#include "media/image.hpp"

namespace toybox {
    
    using namespace toybox;
 
    /**
     A `tileset_c` is a convenience wrapper on top of `image_c` for handling a
     a set of equally sized tiles.
     Tiles can be indexed as column/row using a `point_t`, or by a continuous
     index.
     */
    class tileset_c : public asset_c {
    public:
        tileset_c() = delete;
        tileset_c(const char* path, size_s tile_size);
        tileset_c(const shared_ptr_c<image_c>& image, size_s tile_size);
        virtual ~tileset_c() {};

        __forceinline type_e asset_type() const override final { return tileset; }
        
        __forceinline const shared_ptr_c<image_c>& image() const __pure {
            return _image;
        }

        __forceinline size_s tile_size() const __pure { return _rects[0].size; }
        
        int16_t max_index() const __pure { return _max_tile.x * _max_tile.y; };
        point_s max_tile() const __pure { return _max_tile; };

        __forceinline const rect_s& operator[](int i) const __pure {
            assert(i >= 0 && i < max_index() && "Tile index out of bounds");
            return _rects[i];
        }
        __forceinline const rect_s& operator[](point_s p) const __pure {
            return (*this)[p.x, p.y];
        }
        __forceinline const rect_s& operator[](int x, int y) const __pure {
            assert(x >= 0 && x < _max_tile.x && y >= 0 && y < _max_tile.y && "Tile coordinates out of bounds");
            return _rects[x + _max_tile.x * y];
        }
        
        __forceinline array_s<uint16_t, 6>& data() { return _data; }
        __forceinline const array_s<uint16_t, 6>& data() const { return _data; }

        template<typename T> requires (sizeof(T) <= 12)
        T& data_as() { return (T&)(_data[0]); }
        template<typename T> requires (sizeof(T) <= 12)
        const T& data_as() const { return (const T&)(_data[0]); }
    private:
        const shared_ptr_c<image_c> _image;
        const point_s _max_tile;
        unique_ptr_c<rect_s> _rects;
        array_s<uint16_t, 6> _data;
    };

    namespace detail {
        // EA IFF 85 chunk IDs
        namespace cc4 {
            static constexpr toybox::cc4_t TSHD("TSHD");
        }
        // Tileset header for EA IFF 85 TSHD chunk (inside ILBM)
        struct tileset_header_s {
            size_s tile_size;           // Size of tiles.
            uint16_t reserved[6];            // Custom data
        };
        static_assert(sizeof(tileset_header_s) == 16);
    }
    template<>
    struct struct_layout<detail::tileset_header_s> {
        static constexpr const char* value = "8w";  // tilecount_static, tilecount_animated
    };
    
}
