//
//  level.hpp
//  toybox
//
//  Created by Fredrik on 2025-11-12.
//

#pragma once

#include "runtime/actions.hpp"
#include "runtime/tilemap.hpp"

namespace toybox {

    class dirtymap_c;
    
    static_assert(!is_polymorphic<tilemap_c>::value);
    class tilemap_level_c : public asset_c, public tilemap_c {
    public:
        /// Creates a tilemap level of given bounds.
        /// Tileset is optional (can be null), non-owning, client manages lifetime.
        tilemap_level_c(rect_s tilespace_bounds, tileset_c* tileset);
        
        // Creates a tilemap by loading the level file, and splicing subtilemap 0 into it.
        tilemap_level_c(const char* path);

        ~tilemap_level_c();
        
        __forceinline type_e asset_type() const override final { return tilemap_level; }

        const char* name() const { return _name.get(); }
        
        virtual void update(viewport_c& viewport, int display_id, int ticks);
        bool is_initialized() const { return _is_initialized; }
        virtual void init();
        virtual void reset();
        
        dirtymap_c& tiles_dirtymap() { return *_tiles_dirtymap; }
        void mark_tiles_dirtymap(point_s point);
        void mark_tiles_dirtymap(rect_s rect);

        viewport_c& active_viewport() {
            assert(_viewport && "No active viewport");
            return *_viewport;
        };

        tile_s::type_e collides_with_level(int index) const;
        tile_s::type_e collides_with_level(fpoint_s at) const;
        tile_s::type_e collides_with_level(const frect_s& rect) const;
        bool collides_with_entity(int index, uint8_t in_group, int* index_out) const;
        bool collides_with_entity(const frect_s& rect, uint8_t in_group, int* index_out) const;

        vector_c<action_f, 0>& actions() { return _actions; };
        const vector_c<action_f, 0>& actions() const { return _actions; };
        
        vector_c<entity_type_def_s, 0>& entity_type_defs() { return _entity_type_defs; };
        const vector_c<entity_type_def_s, 0>& entity_type_defs() const { return _entity_type_defs; };

        void update_entity_indexes(int from = 0);
        vector_c<entity_s, 0>& all_entities() { return _all_entities; }
        const vector_c<entity_s, 0>& all_entities() const { return _all_entities; }
        entity_s& spawn_entity(uint8_t type, uint8_t group, frect_s position);
        void destroy_entity(int index);
        void erase_destroyed_entities();

        const rect_s&visible_bounds() const { return _visible_bounds; };
        void set_visible_bounds(const rect_s& bounds);
        const tileset_c& tileset() const { return *_tileset; }
        
        void splice_subtilemap(int index);
    
    protected:
        virtual void update_level();
        virtual void update_actions();
        virtual void draw_tiles();
        virtual void draw_tile(const tile_s& tile, point_s at);
        virtual void draw_entities();
        
        virtual void setup_actions();
        virtual void setup_entity_defs();
        /// Returns tileset for index, or null if not available. Non-owning.
        virtual tileset_c* init_tileset(int index);
        virtual void init(entity_s& entity);
        virtual void init(tile_s& tile, int subtilemap_index);
        virtual void reset(entity_s& entity);
        virtual void reset(tile_s& tile);

        virtual void splice_tile(tile_s& tile, point_s tilespace_at);
        virtual void splice_entity(entity_s& entity);
        
    private:
        viewport_c* _viewport;  // Non-owning, valid only during update() call
        unique_ptr_c<dirtymap_c> _tiles_dirtymap;
        rect_s _visible_bounds;
        tileset_c* _tileset;  // Non-owning, optional (can be null)
        unique_ptr_c<const char> _name;
        vector_c<entity_s, 0> _all_entities;
        vector_c<tilemap_c, 32> _subtilemaps;
        vector_c<action_f, 0> _actions;
        vector_c<entity_type_def_s, 0> _entity_type_defs;
        vector_c<int, 16> _destroy_entities;
        uint8_t _tileset_index;
        bool _is_initialized;
    };
    
    
    // Shared file format structures for level editor and game runtime
    namespace detail {
        // EA IFF 85 chunk IDs
        namespace cc4 {
            static constexpr toybox::cc4_t LEVL("LEVL");
            static constexpr toybox::cc4_t LVHD("LVHD");
            static constexpr toybox::cc4_t ENTS("ENTS");
        }
        // Level header for EA IFF 85 LVHD chunk
        struct level_header_s {
            size_s size;                // Size of level in tiles
            uint8_t tileset_index;      // Index of tileset to use
            uint8_t entity_count;       // Number of entities in level
            int8_t reserved_data[10];
            template<class T> requires (sizeof(T) <= 10)
            T& data_as() { return (T&)(reserved_data[0]); }
            template<class T> requires (sizeof(T) <= 10)
            const T& data_as() const { return (const T&)(reserved_data[0]); }
        };
        static_assert((offsetof(level_header_s, reserved_data) & 1) == 0);
        static_assert(sizeof(level_header_s) == 16);
    }
    // struct_layout for byte-order swapping
    template<>
    struct struct_layout<detail::level_header_s> {
        static constexpr const char* value = "2w2b";  // size.width, size.height, tileset_index, entity_count
    };

}
