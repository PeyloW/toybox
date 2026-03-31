//
//  graphics.cpp
//  toybox
//
//  Created by Fredrik on 2024-02-04.
//

#include "media/palette.hpp"
#include "media/audio.hpp"
#include "core/math.hpp"

using namespace toybox;

#ifndef __M68000__
extern "C" {
    const sound_c* g_active_sound = nullptr;
}
#endif

color_c::color_c(const uint8_t c[3], colorspace_e space) : color_c(c[0], c[1], c[2]) {
    switch (space) {
        case colorspace_e::rgb:
            break;
        case colorspace_e::ycrcb: {
            // YCrCb to RGB conversion (ITU-R BT.601)
            int16_t y  = c[0];
            int16_t cr = c[1] - 128;
            int16_t cb = c[2] - 128;
            auto clamp = [](int32_t v) -> uint8_t { return v < 0 ? 0 : (v > 255 ? 255 : v); };
            *this = color_c(clamp(y + ((mul_fast(cr, (int16_t)359) >> 8))),
                            clamp(y - ((mul_fast(cb, (int16_t)88) + mul_fast(cr, (int16_t)183)) >> 8)),
                            clamp(y + ((mul_fast(cb, (int16_t)454) >> 8))));
            break;
        }
        case colorspace_e::hsb: {
            // HSB to RGB conversion
            // H: 0-255 (hue), S: 0-255 (saturation), B: 0-255 (brightness)
            int16_t h = c[0];
            int16_t s = c[1];
            int16_t b = c[2];
            if (s == 0) {
                *this = color_c((uint8_t)b, (uint8_t)b, (uint8_t)b);
            } else {
                // sector 0-5, using 6 * 256 = 1536 steps for full hue range
                int16_t sector = (h * 6) >> 8;       // 0-5
                int16_t frac = (h * 6) - (sector << 8); // fractional part 0-255
                int16_t p = (int16_t)(mul_fast(b, (int16_t)(255 - s)) >> 8);
                int16_t q = (int16_t)(mul_fast(b, (int16_t)(255 - (int16_t)(mul_fast(s, frac) >> 8))) >> 8);
                int16_t t = (int16_t)(mul_fast(b, (int16_t)(255 - (int16_t)(mul_fast(s, (int16_t)(255 - frac)) >> 8))) >> 8);
                switch (sector) {
                    case 0: *this = color_c((uint8_t)b, (uint8_t)t, (uint8_t)p); break;
                    case 1: *this = color_c((uint8_t)q, (uint8_t)b, (uint8_t)p); break;
                    case 2: *this = color_c((uint8_t)p, (uint8_t)b, (uint8_t)t); break;
                    case 3: *this = color_c((uint8_t)p, (uint8_t)q, (uint8_t)b); break;
                    case 4: *this = color_c((uint8_t)t, (uint8_t)p, (uint8_t)b); break;
                    default: *this = color_c((uint8_t)b, (uint8_t)p, (uint8_t)q); break;
                }
            }
            break;
        }
    }
}

void color_c::get(uint8_t c[3], colorspace_e space) const {
    uint8_t r = from_ste(color, 8);
    uint8_t g = from_ste(color, 4);
    uint8_t b = from_ste(color, 0);
    switch (space) {
        case colorspace_e::rgb:
            c[0] = r;
            c[1] = g;
            c[2] = b;
            break;
        case colorspace_e::ycrcb: {
            // RGB to YCrCb conversion (ITU-R BT.601)
            int16_t ri = r, gi = g, bi = b;
            c[0] = ((mul_fast(ri, (int16_t)66) + mul_fast(gi, (int16_t)129) + mul_fast(bi, (int16_t)25) + 128) >> 8) + 16;
            c[1] = ((mul_fast(ri, (int16_t)112) - mul_fast(gi, (int16_t)94) - mul_fast(bi, (int16_t)18) + 128) >> 8) + 128;
            c[2] = ((-mul_fast(ri, (int16_t)38) - mul_fast(gi, (int16_t)74) + mul_fast(bi, (int16_t)112) + 128) >> 8) + 128;
            break;
        }
        case colorspace_e::hsb: {
            // RGB to HSB conversion
            int16_t max = r > g ? (r > b ? r : b) : (g > b ? g : b);
            int16_t min = r < g ? (r < b ? r : b) : (g < b ? g : b);
            int16_t delta = max - min;
            // Brightness
            c[2] = (uint8_t)max;
            // Saturation
            c[1] = max == 0 ? 0 : (uint8_t)(mul_fast(delta, (int16_t)255) / max);
            // Hue
            if (delta == 0) {
                c[0] = 0;
            } else {
                int16_t delta6 = delta * 6;
                int32_t hue;
                if (max == r) {
                    hue = mul_fast((int16_t)(g - b), (int16_t)256) / delta6;
                    if (hue < 0) hue += 256;
                } else if (max == g) {
                    hue = (mul_fast((int16_t)(b - r), (int16_t)256) / delta6) + 256 / 3;
                } else {
                    hue = (mul_fast((int16_t)(r - g), (int16_t)256) / delta6) + (256 * 2) / 3;
                }
                c[0] = (uint8_t)(hue & 0xff);
            }
            break;
        }
    }
}

color_c color_c::mix(color_c other, int shade) const {
    assert(shade >= MIX_FULLY_THIS && shade <= MIX_FULLY_OTHER && "Shade must be between MIX_FULLY_THIS and MIX_FULLY_OTHER");
    int r = from_ste(color, 8) * (MIX_FULLY_OTHER - shade) + from_ste(other.color, 8) * shade;
    int g = from_ste(color, 4) * (MIX_FULLY_OTHER - shade) + from_ste(other.color, 4) * shade;
    int b = from_ste(color, 0) * (MIX_FULLY_OTHER - shade) + from_ste(other.color, 0) * shade;
    color_c mixed(r / MIX_FULLY_OTHER, g / MIX_FULLY_OTHER, b / MIX_FULLY_OTHER);
    return mixed;
}
