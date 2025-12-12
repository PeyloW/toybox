//
//  tilemap.hpp
//  toybox
//
//  Created by Fredrik on 2025-11-09.
//

#pragma once

#include "media/tileset.hpp"
#include "runtime/entity.hpp"
#include "core/system_helpers.hpp"

namespace toybox {

    struct tile_s  {
        tile_s() {}
        tile_s(const tile_s& o) {
            *this = o;
        }
        tile_s(tile_s&& o) {
            *this = o;
        }
        tile_s& operator=(const tile_s& o) {
            memcpy(this, &o, sizeof(tile_s)); return *this;
        };
        tile_s& operator=(tile_s&& o) {
            memcpy(this, &o, sizeof(tile_s)); return *this;
        };
        enum class type_e : uint8_t {
            none      = 0,
            water     = 1,
            climbable = 2,
            platform  = 3,
            solid     = 4,
            invalid   = 255     // Invalid type, tile is not copied when splicing
        };
        using enum type_e;

        int16_t index = 0;  // tileset index to draw with
        type_e type = none;    // Tile type
        uint8_t flags = 0;
        uint16_t reserved_data[2];
        template<class T> requires (sizeof(T) <= 4)
        T& data_as() { return (T&)(reserved_data[0]); }
        template<class T> requires (sizeof(T) <= 4)
        const T& data_as() const { return (const T&)(reserved_data[0]); }
    };
    static_assert((offsetof(tile_s, reserved_data) & 1) == 0);
    static_assert(sizeof(tile_s) == 8);
    
    
    class tilemap_c : nocopy_c {
    public:
        tilemap_c(const rect_s& tilespace_bounds);
        ~tilemap_c() = default;
        
        __forceinline tile_s& operator[](point_s p) __pure { return (*this)[p.x, p.y]; }
        __forceinline const tile_s& operator[](point_s p) const __pure { return (*this)[p.x, p.y]; }
        __forceinline tile_s& operator[](int x, int y) __pure { return _tiles[x + y * _tilespace_bounds.size.width]; }
        __forceinline const tile_s& operator[](int x, int y) const __pure { return _tiles[x + y * _tilespace_bounds.size.width]; }

        __forceinline rect_s tilespace_bounds() const { return _tilespace_bounds; }
        
        __forceinline vector_c<int8_t,0>& activate_entity_idxs() { return _activate_entity_idxs; };
        __forceinline vector_c<tile_s, 0>& tiles() { return _tiles; };
        
    protected:
        rect_s _tilespace_bounds;
        vector_c<tile_s, 0> _tiles;
        vector_c<int8_t,0> _activate_entity_idxs;
    };
    
    // Shared file format structures for level editor and game runtime
    namespace detail {
        // EA IFF 85 chunk IDs
        namespace cc4 {
            static constexpr toybox::cc4_t TMAP("TMAP");
            static constexpr toybox::cc4_t TMHD("TMHD");
            static constexpr toybox::cc4_t ENTA("ENTA");
            static constexpr toybox::cc4_t BODY("BODY");
        }
        
        // Tilemap header for EA IFF 85 TMAP chunk
        struct tilemap_header_s {
            rect_s bounds;          // Bounds of tilemap in tiles
        };
        static_assert(sizeof(tilemap_header_s) == 8);
    }

    // struct_layout for byte-order swapping
    template<>
    struct struct_layout<tile_s> {
        static constexpr const char* value = "1w6b";  // index(w), type(b), flags(b), data[4](4b)
    };

    template<>
    struct struct_layout<detail::tilemap_header_s> {
        static constexpr const char* value = "4w";  // bounds: origin.x, origin.y, size.width, size.height
    };

}
