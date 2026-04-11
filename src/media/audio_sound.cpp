//
//  audio_sound.cpp
//  toybox
//
//  Created by Fredrik on 2024-02-18.
//

#include "media/audio.hpp"
#include "core/math.hpp"
#include "core/iffstream.hpp"
#include <errno.h>

using namespace toybox;

namespace cc4 {
    static constexpr cc4_t AIFF("AIFF");
    static constexpr cc4_t COMM("COMM");
    static constexpr cc4_t SSND("SSND");
    
    static constexpr cc4_t _8SVX("8SVX");
    static constexpr cc4_t VHDR("VHDR");
    static constexpr cc4_t BODY("BODY");
}

struct  extended80_s {
    uint16_t exp;
    uint16_t fracs[4];
    uint16_t to_uint16() const {
        int16_t exponent = (exp & 0x7fff) - 16383;
        uint16_t significant = 0;
        significant = fracs[0];
        if (exponent > 15) {
            return 0;
        } else {
            return significant >> ( 15 - exponent );
        }
    }
};
static_assert(sizeof(extended80_s) == 10, "extended80_t size mismatch");

struct __attribute__((packed)) aiff_common_s {
    int16_t num_channels;
    uint32_t num_sample_frames;
    int16_t sample_size;
    extended80_s sample_rate;
};
static_assert(sizeof(aiff_common_s) == 18, "aiff_common_t size mismatch");
namespace toybox {
    template<>
    struct struct_layout<aiff_common_s> {
        static constexpr const char* value = "1w1l6w";
    };
}

struct __attribute__((packed)) _vhdr_header_s {
    uint32_t one_shot_samples;      //
    uint32_t repeat_samples;        // If found use for _repeat_length
    uint32_t samples_per_hi_cycles; // Not supported, assume 0
    uint16_t rate;                  // Should be in range 11..14k
    uint8_t ct_octave;              // Not supported, assume 0
    uint8_t compression;            // Not supported, assume 0
    base_fix_t<int32_t, 16> volume; // Not supported assume 1.0
};
static_assert(sizeof(_vhdr_header_s) == 20, "_vhdr_header_s size mismatch");
namespace toybox {
    template<>
    struct struct_layout<_vhdr_header_s> {
        static constexpr const char* value = "3l1w2b1l";
    };
}

struct  aiff_ssnd_data_s {
    uint32_t offset;
    uint32_t block_size;
    uint8_t data[];
};
static_assert(sizeof(aiff_ssnd_data_s) == 8, "ssnd_data_t size mismatch");
namespace toybox {
    template<>
    struct struct_layout<aiff_ssnd_data_s> {
        static constexpr const char* value = "2l";
    };
}

sound_c::sound_c(const char* path) :
    _sample(nullptr),
    _length(0),
    _repeat_length(0),
    _rate(0)
{
    iffstream_c file(path);
    iff_group_s form;
    if (!file.good()) {
        if (errno == 0) errno = EINVAL;
        return;
    }
    if (file.first(cc4::FORM, ::cc4::AIFF, form)) {
        // AIFF format
        iff_chunk_s chunk;
        aiff_common_s common;
        while (file.next(form, cc4::ANY, chunk)) {
            if (chunk.id == ::cc4::COMM) {
                if (!file.read(&common)) {
                    errno = EINVAL;
                    return;
                }
                assert(common.num_channels == 1 && "Only mono audio is supported");
                assert(common.sample_size == 8 && "Only 8-bit audio is supported");
                _length = common.num_sample_frames;
                _rate = common.sample_rate.to_uint16();
                assert(_rate >= 11000 && _rate <= 14000 && "Sample rate must be between 11kHz and 14kHz");
            } else if (chunk.id == ::cc4::SSND) {
                aiff_ssnd_data_s data;
                if (!file.read(&data)) {
                    errno = EINVAL;
                    return;
                }
                assert(data.offset == 0 && "SSND offset must be zero");
                assert(chunk.size - 8 == common.num_sample_frames && "SSND data size must match sample frame count");
                _sample.reset((int8_t*)_malloc(_length + 256));
                file.read(_sample.get(), _length);
            } else {
#ifndef __M68000__
                printf("Skipping '%s'\n", chunk.id.cstring());
#endif
                file.skip(chunk);
            }
        }
    } else {
        // Try 8SVX format
        file.seek(0, stream_c::seekdir_e::beg);
        if (!file.first(cc4::FORM, ::cc4::_8SVX, form)) {
            if (errno == 0) errno = EINVAL;
            return;
        }
        iff_chunk_s chunk;
        _vhdr_header_s vhdr;
        while (file.next(form, cc4::ANY, chunk)) {
            if (chunk.id == ::cc4::VHDR) {
                if (!file.read(&vhdr)) {
                    errno = EINVAL;
                    return;
                }
                assert(vhdr.compression == 0 && "Only uncompressed 8SVX is supported");
                _length = vhdr.one_shot_samples + vhdr.repeat_samples;
                _repeat_length = vhdr.repeat_samples;
                _rate = vhdr.rate;
                assert(_rate >= 11000 && _rate <= 14000 && "Sample rate must be between 11kHz and 14kHz");
            } else if (chunk.id == ::cc4::BODY) {
                assert(_length > 0 && "VHDR must precede BODY");
                _sample.reset((int8_t*)_malloc(_length + 256));
                file.read(_sample.get(), _length);
            } else {
#ifndef __M68000__
                printf("Skipping '%s'\n", chunk.id.cstring());
#endif
                file.skip(chunk);
            }
        }
    }
    // Cleanup: pad lengths to multiples of 4
    if (_sample) {
        _length = _length & ~3;
        _repeat_length = _repeat_length & ~3;
        // Scale all samples by 50%
        for (size_t i = 0; i < _length; i++) {
            _sample.get()[i] /= 2;
        }
        // Fill 256-byte overflow with repeat data or zeros
        if (_repeat_length > 0) {
            const int8_t* repeat_start = _sample.get() + _length - _repeat_length;
            for (size_t i = 0; i < 256; i++) {
                _sample.get()[_length + i] = repeat_start[i % _repeat_length];
            }
        } else {
            memset(_sample.get() + _length, 0, 256);
        }
    }
}
