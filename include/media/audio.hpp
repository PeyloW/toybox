//
//  audio.hpp
//  toybox
//
//  Created by Fredrik on 2024-02-18.
//

#pragma once

#include "core/cincludes.hpp"
#include "runtime/assets.hpp"
#include "core/geometry.hpp"
#include "core/memory.hpp"

namespace toybox {
    
    /**
     A `sound_c` is an 8 bit PCM sound sample.
     Sounds can be loaded for EA 85 AIFF files., or 8SVX IFF files. 8SVX files support repeats.
     */
    class sound_c final : public asset_c {
        friend class audio_mixer_c;
    public:
        sound_c() = delete;
        sound_c(const char* path);
        virtual ~sound_c() {}

        __forceinline type_e asset_type() const override __pure { return sound; }

        __forceinline const int8_t* sample() const_pure { return _sample.get(); }
        __forceinline size_t length() const_pure { return _length; }
        __forceinline size_t repeat_length() const_pure { return _repeat_length; }
        __forceinline uint16_t rate() const_pure { return _rate; }
        
    private:
        unique_ptr_c<int8_t> _sample;
        size_t _length;
        size_t _repeat_length;
        uint16_t _rate;
    };
    
    /**
     A `music_c` is a collection of music, containing one or more songs.
     Supports SNDH files, with MOD support planned.
     */
    class music_c final : public asset_c {
        friend class audio_mixer_c;
    public:
        enum class format_e : uint8_t {
            sndh,
            mod
        };
        using enum format_e;
        
        music_c(const char* path);
        ~music_c() {}
        
        __forceinline asset_c::type_e asset_type() const override __pure { return music; }
        
        __forceinline format_e format() const_pure { return _format; }
        __forceinline const char* title() const_pure { return _title; }
        __forceinline const char* composer() const_pure { return _composer; }
        __forceinline int track_count() const_pure { return _track_count; }
        __forceinline uint8_t replay_freq() const_pure { return _freq; }
        __forceinline const uint8_t* data() const_pure { return _data.get(); }
        __forceinline size_t length() const_pure { return _length; }
        __forceinline bool supports_command() const_pure { return _supports_command; }

    private:
        unique_ptr_c<uint8_t> _data;
        size_t _length;
        char* _title;  // Non-owning, points into _data
        char* _composer;  // Non-owning, points into _data
        int _track_count;
        format_e _format;
        uint8_t _freq;
        bool _supports_command;
    };

}
