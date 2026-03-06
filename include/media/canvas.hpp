//
//  canvas.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-10.
//

#pragma once

#include "media/image.hpp"
#include "media/tileset.hpp"
#include "media/font.hpp"
#include "media/dirtymap.hpp"
#include "core/concepts.hpp"

namespace toybox {
    
    class canvas_c : public nocopy_c {
        friend class viewport_c;
    public:
        class remap_table_c : nocopy_c {
        public:
            constexpr remap_table_c() { for (int i = -1; i < 16; i++) _table[i + 1] = i; }
            template<int Count>
            constexpr remap_table_c(const pair_c<int, int> (&maps)[Count]) : remap_table_c() {
                for (const auto& map : maps) {
                    assert(map.first >= -1 && map.first < 16);
                    assert(map.second >= -1 && map.second < 16);
                    _table[map.first + 1] = map.second;
                }
            }
            int& operator[](int i) {
                assert(i >= -1 && i < 16);
                __assume_range(i, -1, 16);
                return _table[i + 1];
            }
            const int& operator[](int i) const {
                assert(i >= -1 && i < 16);
                __assume_range(i, -1, 16);
                return _table[i + 1];
            }
        private:
            int _table[17];
        };
        
        static constexpr int STENCIL_FULLY_TRANSPARENT = 0;
        static constexpr int STENCIL_FULLY_OPAQUE = 64;
        using stencil_t = uint16_t[16];
        enum class stencil_e : uint8_t {
            none,
            orderred,
            noise,
            diagonal,
            circle,
            random
        };
        static stencil_e effective_type(stencil_e type);
        
        enum class alignment_e : uint8_t {
            left,
            center,
            right
        };
        
        canvas_c(image_c& image);
        ~canvas_c() = default;

        __forceinline image_c& image() const { return _image; }
        __forceinline size_s size() const { return _image.size(); }
        
        static const stencil_t* const stencil(stencil_e type, int shade);
        
        void remap_colors(const remap_table_c& table,  const rect_s& rect) const;
        
        static void make_stencil(stencil_t stencil, stencil_e type, int shade);
        
        template<invocable<> Commands>
        void with_clipping(bool clip, Commands commands) {
            const bool old_clip = _clipping;
            _clipping = clip;
            commands();
            _clipping = old_clip;
        }
        const rect_s& clip_rect() const { return _clip_rect; }
        void set_clip_rect(const rect_s& rect) { _clip_rect = rect; }

        template<invocable<> Commands>
        void with_stencil(const stencil_t* const stencil, Commands commands) {
            const auto old_stencil = _stencil;
            _stencil = stencil;
            commands();
            _stencil = old_stencil;
        }

        template<invocable<> Commands>
        void with_dirtymap(dirtymap_c* dirtymap, Commands commands) {
            dirtymap_c* old_dirtymap = _dirtymap;
            _dirtymap = dirtymap;
            commands();
            _dirtymap = old_dirtymap;
        }
        
        void fill(uint8_t ci, const rect_s& rect);

        void draw_aligned(const image_c& src, point_s at);
        void draw_aligned(const image_c& src, const rect_s& rect, point_s at);
        void draw_aligned(const tileset_c& src, int idx, point_s at);
        void draw_aligned(const tileset_c& src, point_s tile, point_s at);
        void draw(const image_c& src, point_s at, int color = image_c::MASKED_CIDX);
        void draw(const image_c& src, const rect_s& rect, point_s at, int color = image_c::MASKED_CIDX);
        void draw(const tileset_c& src, int idx, point_s at, int color = image_c::MASKED_CIDX);
        void draw(const tileset_c& src, point_s tile, point_s at, int color = image_c::MASKED_CIDX);

        void draw_3_patch(const image_c& src, int16_t cap, const rect_s& in);
        void draw_3_patch(const image_c& src, const rect_s& rect, int16_t cap, const rect_s& in);

        size_s draw(const font_c& font, const char* text, point_s at, alignment_e alignment = alignment_e::center, int color = image_c::MASKED_CIDX);
        size_s draw(const font_c& font, const char* text, const rect_s& in, uint16_t line_spacing = 0, alignment_e alignment = alignment_e::center, int color = image_c::MASKED_CIDX);

        template<invocable<> Commands>
        void with_tileset(const tileset_c& tileset, Commands commands) {
            assert(_tileset_line_words == 0 && "Cannot nest with_tileset()");
            imp_init_draw_tile(tileset);
            commands();
            _tileset_line_words = 0;
        }
        void fill_tile(uint8_t ci, point_s at);
        void draw_tile(const tileset_c& src, int idx, point_s at);
        void draw_tile(const tileset_c& src, point_s tile, point_s at);

    protected:
        image_c& _image;
        dirtymap_c* _dirtymap = nullptr;
        const stencil_t* _stencil = nullptr;
        rect_s _clip_rect;
        uint16_t _tileset_line_words;
        bool _clipping = true;
        
        __neverinline void imp_fill(uint8_t ci, const rect_s& rect) const;
        __neverinline void imp_draw_aligned(const image_c& srcImage, const rect_s& rect, point_s point) const;
        __neverinline void imp_draw(const image_c& srcImage, const rect_s& rect, point_s point) const;
        __neverinline void imp_draw_masked(const image_c& srcImage, const rect_s& rect, point_s point) const;
        __neverinline void imp_draw_color(const image_c& srcImage, const rect_s& rect, point_s point, uint16_t color) const;

        __neverinline void imp_init_draw_tile(const tileset_c& srcTileset);
        __neverinline void imp_fill_tile(uint8_t ci, point_s point) const;
        __neverinline void imp_draw_tile(const image_c& srcImage, const rect_s& rect, point_s point) const;
        __neverinline void imp_draw_tile_masked(const image_c& srcImage, const rect_s& rect, point_s point) const;

        __neverinline void imp_draw_rect_SLOW(const image_c& srcImage, const rect_s& rect, point_s point) const;
        
        
    };
    
}
