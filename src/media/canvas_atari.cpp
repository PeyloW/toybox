//
//  graphics_draw_blitter.cpp
//  toybox
//
//  Created by Fredrik on 2024-02-24.
//

#include "media/canvas.hpp"
#include "machine/blitter_atari.hpp"
#include "core/algorithm.hpp"

#if TOYBOX_TARGET_ATARI

using namespace toybox;

static constexpr uint8_t pBlitter_skewflags[4] = {
    blitter_s::nfsr_bit,
    blitter_s::fxsr_bit,
    0,
    blitter_s::nfsr_bit|blitter_s::fxsr_bit,
};

static constexpr uint16_t pBlitter_mask[17] = {
    0xffff, 0x7fff, 0x3fff, 0x1fff,
    0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f,
    0x000f, 0x0007, 0x0003, 0x0001,
    0x0000
};

static const canvas_c::stencil_t* pActiveStencil = nullptr;

static __forceinline void __set_active_stencil(struct blitter_s* blitter, const canvas_c::stencil_t* const stencil) {
    if (pActiveStencil != stencil) {
        memcpy((void*)blitter->halftoneRAM, stencil, 32);
        pActiveStencil = stencil;
    }
}

void canvas_c::imp_fill(uint8_t color, const rect_s& rect) const {
    uint16_t dummy_src = 0;
    auto blitter = pBlitter;

    const int16_t dst_max_x = rect.max_x();
    const int16_t dst_words_dec_1  = ((dst_max_x / 16) - (rect.origin.x / 16));

    // Source
    blitter->srcIncX = 0;
    blitter->srcIncY = 0;
    blitter->pSrc   = &dummy_src;

    // Dest
    blitter->dstIncX  = 8;
    blitter->dstIncY = (_image._line_words * 8 - (dst_words_dec_1 * 8));
    const int16_t dst_word_offset = (rect.origin.y * _image._line_words) + (rect.origin.x / 16);
    uint16_t* dst_bitmap = _image._bitmap + dst_word_offset * 4l;

    // Mask
    uint16_t end_mask_0 = pBlitter_mask[rect.origin.x & 15];
    uint16_t end_mask_2 = ~pBlitter_mask[(dst_max_x & 15) + 1];
    if (dst_words_dec_1 == 0) {
        end_mask_0 &= end_mask_2;
        end_mask_2 = end_mask_0;
    }
    blitter->endMask[0] = end_mask_0;
    blitter->endMask[1] = 0xFFFF;
    blitter->endMask[2] = end_mask_2;

    // Counts
    blitter->countX  = (dst_words_dec_1 + 1);
    
    // Operation flags
    if (_stencil) {
        __set_active_stencil(blitter, _stencil);
        blitter->HOP = blitter_s::hop_e::halftone;
    } else {
        blitter->HOP = blitter_s::hop_e::one;
    }
    blitter->skew = 0;
    

    // Color 4 planes
    for (int i = 0; i < 4; i++) {
        if ((color & 1) == 0) {
            blitter->LOP = blitter_s::lop_e::notsrc_and_dst;
        } else {
            blitter->LOP = blitter_s::lop_e::src_or_dst;
        }
        blitter->pDst   = dst_bitmap;
        blitter->countY = rect.size.height;

        blitter->start();

        color >>= 1;
        dst_bitmap++;
    }

}

void canvas_c::imp_draw_aligned(const image_c& srcImage, const rect_s& rect, point_s at) const {
    assert((rect.origin.x & 0xf) == 0 && "Rect origin X must be 16-byte aligned");
    assert((rect.size.width & 0xf) == 0 && "Rect width must be 16-byte aligned");
    assert((at.x & 0xf) == 0 && "Destination X must be 16-byte aligned");
    assert((srcImage.size().width & 0xf) == 0 && "Source image width must be 16-byte aligned");
    assert(!rect.size.is_empty() && "Rect size must not be empty");
    assert(rect_s(at, rect.size).contained_by(clip_rect()) && "Destination rect must be within canvas bounds");
    assert(rect.contained_by(srcImage.size()) && "Source rect must be within source image bounds");
        
    auto blitter = pBlitter;
    const int16_t copy_words = (rect.size.width / 16);

    // Source
    blitter->srcIncX  = 2;
    blitter->srcIncY = (srcImage._line_words - copy_words) * 8 + 2;
    const int16_t src_word_offset = (rect.origin.y * srcImage._line_words) + (rect.origin.x / 16);
    blitter->pSrc = srcImage._bitmap + src_word_offset * 4l;

    // Dest
    blitter->dstIncX  = 2;
    blitter->dstIncY = (_image._line_words - copy_words) * 8 + 2;
    const int16_t dst_word_offset = (at.y * _image._line_words) + (at.x / 16);
    blitter->pDst = _image._bitmap + dst_word_offset * 4l;

    // Mask
    blitter->endMask[0] =  0xFFFF;
    blitter->endMask[1] =  0xFFFF;
    blitter->endMask[2] =  0xFFFF;

    // Counts
    const auto countX = (copy_words) * 4;
    blitter->countX  = countX;
    const auto countY = rect.size.height;
    blitter->skew = 0;
    
    // Operation flags
    if (_stencil) {
        // TODO: This should be using the stencil mask, but that is buggy on target.
        const bool hog = countX <= 16;
        blitter->HOP = blitter_s::hop_e::src;
        blitter->LOP = blitter_s::lop_e::src;
        for (int y = 0; y < countY; y++) {
            blitter->countY = 1;
            const auto m = ((uint16_t*)_stencil)[y & 0xf];
            blitter->endMask[0] = m;
            blitter->endMask[1] = m;
            blitter->endMask[2] = m;
            blitter->start(hog);
        }
    } else {
        blitter->countY = countY;
        blitter->HOP = blitter_s::hop_e::src;
        blitter->LOP = blitter_s::lop_e::src;
        blitter->start();
    }
}

void canvas_c::imp_draw(const image_c& srcImage, const rect_s& rect, point_s at) const {
    assert(!rect.size.is_empty() && "Rect size must not be empty");
    assert(rect_s(at, rect.size).contained_by(clip_rect()) && "Destination rect must be within canvas bounds");
    assert(rect.contained_by(srcImage.size()) && "Source rect must be within source image bounds");
    auto blitter = pBlitter;

    const int16_t src_max_x       = rect.max_x();
    const int16_t dst_max_x       = (at.x + rect.size.width - 1);
    const int16_t src_words_dec_1 = ((src_max_x / 16) - (rect.origin.x / 16));
    const int16_t dst_words_dec_1 = ((dst_max_x / 16) - (at.x / 16));

    // Source
    blitter->srcIncX = 8;
    blitter->srcIncY = ((srcImage._line_words - src_words_dec_1) * 8);
    const int16_t src_word_offset = (rect.origin.y * srcImage._line_words) + (rect.origin.x / 16);
    uint16_t* src_bitmap  = srcImage._bitmap + src_word_offset * 4l;
    
    // Dest
    blitter->dstIncX  = 8;
    blitter->dstIncY = ((_image._line_words - dst_words_dec_1) * 8);
    const uint16_t dst_word_offset = (at.y * _image._line_words) + (at.x / 16);
    uint16_t* dst_bitmap  = _image._bitmap + dst_word_offset * 4l;

    // Mask
    uint16_t end_mask_0 = pBlitter_mask[at.x & 15];
    uint16_t end_mask_2 = ~pBlitter_mask[(dst_max_x & 15) + 1];
    uint8_t skew = (uint8_t)(((at.x & 15) - (rect.origin.x & 15)) & 15);
    if (dst_words_dec_1 == 0) {
        end_mask_0 &= end_mask_2;
        end_mask_2 = end_mask_0;
        if (src_words_dec_1 != 0) {
            skew |= blitter_s::fxsr_bit;
        } else if ((rect.origin.x & 15) > (at.x & 15)) {
            skew |= blitter_s::fxsr_bit;
            blitter->srcIncY -= 8;
        }
    } else {
        int idx = 0;
        if ((rect.origin.x & 15) > (at.x & 15)) {
            idx |= 1;
        }
        if (src_words_dec_1 == dst_words_dec_1) {
            idx |= 2;
        }
        skew |= pBlitter_skewflags[idx];
    }
    blitter->endMask[0] = end_mask_0;
    blitter->endMask[1] = 0xFFFF;
    blitter->endMask[2] = end_mask_2;

    // Counts
    blitter->countX  = (dst_words_dec_1 + 1);
    
    // Operation flags
    blitter->HOP = blitter_s::hop_e::src;
    blitter->LOP = blitter_s::lop_e::src;
    blitter->skew = skew;

    // Move 4 planes
    for (int i = 0; i < 4; i++) {
        blitter->pDst   = dst_bitmap;
        blitter->pSrc   = src_bitmap;
        blitter->countY = rect.size.height;

        blitter->start();

        src_bitmap++;
        dst_bitmap++;
    }
}

void canvas_c::imp_draw_masked(const image_c& srcImage, const rect_s& rect, point_s at) const {
    assert(!rect.size.is_empty() && "Rect size must not be empty");
    assert(rect_s(at, rect.size).contained_by(clip_rect()) && "Destination rect must be within canvas bounds");
    assert(rect.contained_by(srcImage.size()) && "Source rect must be within source image bounds");
    auto blitter = pBlitter;

    const int16_t src_max_x       = rect.max_x();
    const int16_t dst_max_x       = (at.x + rect.size.width - 1);
    const int16_t src_words_dec_1 = ((src_max_x / 16) - (rect.origin.x / 16));
    const int16_t dst_words_dec_1 = ((dst_max_x / 16) - (at.x / 16));
    
    // Source
    blitter->srcIncX = 2;
    blitter->srcIncY = ((srcImage._line_words - src_words_dec_1) * 2);
    const int16_t src_word_offset = (rect.origin.y * srcImage._line_words) + (rect.origin.x / 16);
    uint16_t* src_maskmap  = srcImage._maskmap + src_word_offset;

    // Dest
    blitter->dstIncX  = 8;
    blitter->dstIncY = (_image._line_words * 8 - (dst_words_dec_1 * 8));
    const int16_t dst_word_offset = (at.y * _image._line_words) + (at.x / 16);
    uint16_t* dst_bitmap  = _image._bitmap + dst_word_offset * 4l;

    // Mask
    uint16_t end_mask_0 = pBlitter_mask[at.x & 15];
    uint16_t end_mask_2 = ~pBlitter_mask[(dst_max_x & 15) + 1];
    uint8_t skew = (uint8_t)(((at.x & 15) - (rect.origin.x & 15)) & 15);
    if (dst_words_dec_1 == 0) {
        end_mask_0 &= end_mask_2;
        end_mask_2 = end_mask_0;
        if (src_words_dec_1 != 0) {
            skew |= blitter_s::fxsr_bit;
        } else if ((rect.origin.x & 15) > (at.x & 15)) {
            skew |= blitter_s::fxsr_bit;
            blitter->srcIncY -= 2;
        }
    } else {
        int idx = 0;
        if ((rect.origin.x & 15) > (at.x & 15)) {
            idx |= 1;
        }
        if (src_words_dec_1 == dst_words_dec_1) {
            idx |= 2;
        }
        skew |= pBlitter_skewflags[idx];
    }
    blitter->endMask[0] = end_mask_0;
    blitter->endMask[1] = 0xFFFF;
    blitter->endMask[2] = end_mask_2;

    // Counts
    blitter->countX  = (dst_words_dec_1 + 1);
    
    // Operation flags
    blitter->HOP = blitter_s::hop_e::src;
    blitter->LOP = blitter_s::lop_e::notsrc_and_dst;
    blitter->skew = skew;

    // Mask 4 planes
    for (int i = 0; i < 4; i++) {
        blitter->pDst   = dst_bitmap;
        blitter->pSrc   = src_maskmap;
        blitter->countY = rect.size.height;

        blitter->start();

        dst_bitmap++;
    }

    // Update source
    blitter->srcIncX *= 4;
    blitter->srcIncY *= 4;
    uint16_t* src_bitmap = srcImage._bitmap + src_word_offset * 4;
    
    // Update dest
    dst_bitmap -= 4;
    
    // Update operation flags
    blitter->LOP = blitter_s::lop_e::src_or_dst;

    // Draw 4 planes
    for (int i = 0; i < 4; i++) {
        blitter->pDst   = dst_bitmap;
        blitter->pSrc   = src_bitmap;
        blitter->countY = rect.size.height;

        blitter->start();

        src_bitmap++;
        dst_bitmap++;
    }
}

void canvas_c::imp_draw_color(const image_c& srcImage, const rect_s& rect, point_s at, uint16_t color) const {
    assert(!rect.size.is_empty() && "Rect size must not be empty");
    assert(rect_s(at, rect.size).contained_by(clip_rect()) && "Destination rect must be within canvas bounds");
    assert(rect.contained_by(srcImage.size()) && "Source rect must be within source image bounds");
    auto blitter = pBlitter;

    const int16_t src_max_x       = rect.max_x();
    const int16_t dst_max_x       = (at.x + rect.size.width - 1);
    const int16_t src_words_dec_1 = ((src_max_x / 16) - (rect.origin.x / 16));
    const int16_t dst_words_dec_1 = ((dst_max_x / 16) - (at.x / 16));

    // Source
    blitter->srcIncX = 2;
    blitter->srcIncY = ((srcImage._line_words - src_words_dec_1) * 2);
    const int16_t src_word_offset = (rect.origin.y * srcImage._line_words) + (rect.origin.x / 16);
    uint16_t* src_maskmap  = srcImage._maskmap + src_word_offset;
    
    // Dest
    blitter->dstIncX  = 8;
    blitter->dstIncY = (_image._line_words * 8 - (dst_words_dec_1 * 8));
    const int16_t dst_word_offset = (at.y * _image._line_words) + (at.x / 16);
    uint16_t* dst_bitmap  = _image._bitmap + dst_word_offset * 4l;

    // Mask
    uint16_t end_mask_0 = pBlitter_mask[at.x & 15];
    uint16_t end_mask_2 = ~pBlitter_mask[(dst_max_x & 15) + 1];
    uint8_t skew = (uint8_t)(((at.x & 15) - (rect.origin.x & 15)) & 15);
    if (dst_words_dec_1 == 0) {
        end_mask_0 &= end_mask_2;
        end_mask_2 = end_mask_0;
        if (src_words_dec_1 != 0) {
            skew |= blitter_s::fxsr_bit;
        } else if ((rect.origin.x & 15) > (at.x & 15)) {
            skew |= blitter_s::fxsr_bit;
            blitter->srcIncY -= 2;
        }
    } else {
        int idx = 0;
        if ((rect.origin.x & 15) > (at.x & 15)) {
            idx |= 1;
        }
        if (src_words_dec_1 == dst_words_dec_1) {
            idx |= 2;
        }
        skew |= pBlitter_skewflags[idx];
    }
    blitter->endMask[0] = end_mask_0;
    blitter->endMask[1] = 0xFFFF;
    blitter->endMask[2] = end_mask_2;

    // Counts
    blitter->countX  = (dst_words_dec_1 + 1);
    
    // Operation flags
    blitter->HOP = blitter_s::hop_e::src;
    blitter->skew = skew;

    // Color 4 planes
    for (int i = 0; i < 4; i++) {
        if ((color & 1) == 0) {
            blitter->LOP = blitter_s::lop_e::notsrc_and_dst;
        } else {
            blitter->LOP = blitter_s::lop_e::src_or_dst;
        }
        blitter->pDst   = dst_bitmap;
        blitter->pSrc   = src_maskmap;
        blitter->countY = rect.size.height;

        blitter->start();

        color >>= 1;
        dst_bitmap++;
    }
}

void canvas_c::imp_init_draw_tile(const tileset_c& srcTileset) {
    assert(srcTileset.tile_size() == size_s(16,16) && "Only 16x16 tiles supported");
    _tileset_line_words = srcTileset.image()->_line_words;

    auto blitter = pBlitter;

    // Source configuration (constant for tileset)
    // We fake srcIncX for lines, and srcIncY to step back to next bitplane
    blitter->srcIncX = _tileset_line_words * 8;
    blitter->srcIncY = -(_tileset_line_words * 15 * 8) + 2;

    // Destination configuration (constant for canvas)
    blitter->dstIncX = _image._line_words * 8;
    blitter->dstIncY = -(_image._line_words * 15 * 8) + 2;

    // Masks (16-pixel aligned = no edge masking)
    blitter->endMask[0] = 0xFFFF;
    blitter->endMask[1] = 0xFFFF;
    blitter->endMask[2] = 0xFFFF;

    // Counts (16x16 tile = 1 word wide)
    // But we use X for lines, and Y to step back
    blitter->countX = 16;

    // Operation (HOP always src for both draw and fill)
    blitter->HOP = blitter_s::hop_e::src;
    blitter->skew = 0;
}

void canvas_c::imp_fill_tile(uint8_t ci, point_s at) const {
    assert(_tileset_line_words != 0 && "fill_tile called without correct with_tileset");
    assert((at.x & 0xf) == 0 && "Tile must be aligned to 16px boundary");

    auto blitter = pBlitter;

    // Destination address
    const int16_t dst_word_offset = (at.y * _image._line_words) + (at.x / 16);
    uint16_t* dst_bitmap = _image._bitmap + dst_word_offset * 4;
    blitter->pDst = dst_bitmap;

    // Fill each bitplane based on color bits
    for (int i = 0; i < 4; i++) {
        // LOP: zero clears bit, one sets bit
        if ((ci & 1) == 0) {
            blitter->LOP = blitter_s::lop_e::zero;
        } else {
            blitter->LOP = blitter_s::lop_e::one;
        }

        blitter->countY = 1;
        blitter->start(true);

        ci >>= 1;
    }
}

void canvas_c::imp_draw_tile(const image_c& srcImage, const rect_s& rect, point_s at) const {
    assert(srcImage._line_words == _tileset_line_words && "draw_tile called without correct with_tileset");
    assert(rect.size == size_s(16,16) && "Only 16x16 tiles supported");
    assert((rect.origin.x & 0xf) == 0 && "Tile must be aligned to 16px boundary");
    assert((at.x & 0xf) == 0 && "Destination must be aligned to 16px boundary");

    auto blitter = pBlitter;

    // Calculate source address (tile in tileset)
    const int16_t src_word_offset = (rect.origin.y * _tileset_line_words)
                                  + (rect.origin.x / 16);
    uint16_t* src_bitmap = srcImage._bitmap + src_word_offset * 4;
    blitter->pSrc = src_bitmap;

    // Calculate destination address (screen position)
    const int16_t dst_word_offset = (at.y * _image._line_words)
                                  + (at.x / 16);
    uint16_t* dst_bitmap = _image._bitmap + dst_word_offset * 4;
    blitter->pDst = dst_bitmap;

    // LOP for straight copy
    blitter->LOP = blitter_s::lop_e::src;

    // Draw all 4 bitplanes
    for (int i = 0; i < 4; i++) {
        blitter->countY = 1;
        blitter->start(true);
    }
}

void canvas_c::imp_draw_tile_masked(const image_c& srcImage, const rect_s& rect, point_s at) const {
    assert(srcImage._line_words == _tileset_line_words && "draw_tile called without correct with_tileset");
    assert(rect.size == size_s(16,16) && "Only 16x16 tiles supported");
    assert((rect.origin.x & 0xf) == 0 && "Tile must be aligned to 16px boundary");
    assert((at.x & 0xf) == 0 && "Destination must be aligned to 16px boundary");

    auto blitter = pBlitter;

    // Calculate source address (tile in tileset)
    const int16_t src_word_offset = (rect.origin.y * _tileset_line_words)
                                  + (rect.origin.x / 16);
    uint16_t* src_maskmap = srcImage._maskmap + src_word_offset;
    blitter->pSrc = src_maskmap;
    blitter->srcIncX = _tileset_line_words * 2;
    blitter->srcIncY = -(_tileset_line_words * 15 * 2);

    // Calculate destination address (screen position)
    const int16_t dst_word_offset = (at.y * _image._line_words)
                                  + (at.x / 16);
    uint16_t* dst_bitmap = _image._bitmap + dst_word_offset * 4;

    blitter->pDst = dst_bitmap;

    // LOP for straight copy
    blitter->LOP = blitter_s::lop_e::notsrc_and_dst;

    // Draw all 4 bitplanes
    for (int i = 0; i < 4; i++) {
        blitter->countY = 1;
        blitter->start(true);
    }
        
    uint16_t* src_bitmap = srcImage._bitmap + src_word_offset * 4;
    blitter->pSrc = src_bitmap;
    blitter->srcIncX = _tileset_line_words * 8;
    blitter->srcIncY = -(_tileset_line_words * 15 * 8) + 2;

    blitter->pDst = dst_bitmap;

    // LOP for straight copy
    blitter->LOP = blitter_s::lop_e::src_or_dst;

    // Draw all 4 bitplanes
    for (int i = 0; i < 4; i++) {
        blitter->countY = 1;
        blitter->start(true);
    }
}

void canvas_c::imp_draw_rect_SLOW(const image_c& srcImage, const rect_s& rect, point_s at) const {
    assert(!rect.size.is_empty() && "Rect size must not be empty");
    assert(rect_s(at, rect.size).contained_by(clip_rect()) && "Destination rect must be within canvas bounds");
    assert(rect.contained_by(srcImage.size()) && "Source rect must be within source image bounds");
    for (int y = 0; y < rect.size.height; y++) {
        for (int x = 0; x < rect.size.width; x++) {
            int color = srcImage.get_pixel(point_s{(int16_t)(rect.origin.x + x), (int16_t)(rect.origin.y + y)});
            if (!image_c::is_masked(color)) {
                _image.put_pixel(color, point_s{(int16_t)(at.x + x), (int16_t)(at.y + y)});
            }
        }
    }
}

#endif
