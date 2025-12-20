//
//  audio_mixer.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-20.
//

#pragma once

#include "media/audio.hpp"

namespace toybox {
    
    /**
     A `audio_mixer_c` is an abstraction for mixing audio.
     TODO: Support multiple sound effects.
     */
    class audio_mixer_c : public nocopy_c {
    public:
        static audio_mixer_c& shared();

        __forceinline int channel_count() const __pure { return 1; }
        void play(const sound_c& sound, uint8_t priority = 0);
        void stop(const sound_c& sound);

        void play(const music_c& music_c, int track = 1); // Track starts at 1, not 0;
        void stop(const music_c& music);
        
        void stop_all();

    private:
        const music_c* _active_music;  // Non-owning, references music owned elsewhere
        int _active_track;
#ifdef __M68000__
        uint16_t _music_init_code[8];
        uint16_t _music_exit_code[8];
        uint16_t _music_play_code[8];
#endif
        audio_mixer_c();
        ~audio_mixer_c();
    };
  
}
