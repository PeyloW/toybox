//
//  audio_mixer.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-20.
//

#include "media/audio_mixer.hpp"
#include "machine/machine.hpp"
#include "machine/timer.hpp"
#include "core/memory.hpp"
#include "core/system_helpers.hpp"
#ifdef TOYBOX_HOST
#include "machine/host_bridge.hpp"
#endif

#if TOYBOX_TARGET_ATARI

using namespace toybox;

audio_mixer_c& audio_mixer_c::shared() {
    static audio_mixer_c s_mixer;
    return s_mixer;
}

struct channel_t {
    const int8_t* current = nullptr; // nullptr if not playing
    const int8_t* end = nullptr;     // undefined if not playing
    size_t  repeat_length = 0;  // 0 if no repeat, otherwise decrease current with this if past end
    uint8_t priority = 0;       // undefined if not playing
};

namespace sfx_state {
    static channel_t s_channels[4];
    static int8_t* s_dma_buffer = nullptr;  // 1024 bytes allocated
    static int8_t* s_mix_start = nullptr;   // 256-byte-aligned start within s_dma_buffer (3 × 256 = 768 usable)
    static split_hw_addr_c<0xffff8903> s_dma_start;
    static split_hw_addr_c<0xffff8909> s_dma_current;
    static split_hw_addr_c<0xffff890f> s_dma_end;
    static uint8_t s_start_mid;     // mid byte of s_mix_start, used to convert DMA mid byte to buffer index
    static uint8_t s_prev_buf_idx;  // buffer index (0-2) to fill next
}

__forceinline static void sfx_mix_channel(const int8_t* _samples, int8_t* _mix_buffer, bool first) {
    // For perf reasons we mix 32 bit values to get a 4x speedup.
    // The added audio noice is worth it.
    const int32_t* samples = reinterpret_cast<const int32_t*>(_samples);
    int32_t* mix_buffer = reinterpret_cast<int32_t*>(_mix_buffer);
    if (first) {
        #pragma GCC unroll 8
        for (int i = 0; i < 256 / 4; i++) {
            *mix_buffer++ = *samples++;
        }
    } else {
        #pragma GCC unroll 8
        for (int i = 0; i < 256 / 4; i++) {
            *mix_buffer++ += *samples++;
        }
    }
}

__forceinline static void sfx_mix_clear(int8_t* _mix_buffer) {
    int32_t* mix_buffer = reinterpret_cast<int32_t*>(_mix_buffer);
    #pragma GCC unroll 8
    for (int i = 0; i < 256 / 4; i++) {
        *mix_buffer++ = 0;
    }
}

extern "C" void g_sfx_mixer_callback(int8_t* mix_buffer) {
    bool first = true;
    for (int i = 0; i < 4; i++) {
        channel_t& channel = sfx_state::s_channels[i];
        if (channel.current) {
            sfx_mix_channel(channel.current, mix_buffer, first);
            channel.current += 256;
            if (channel.current >= channel.end) {
                if (channel.repeat_length) {
                    channel.current -= channel.repeat_length;
                } else {
                    channel.current = nullptr;
                }
            }
            first = false;
        }
    }
    if (first) {
        sfx_mix_clear(mix_buffer);
    }
}

#ifdef __M68000__
static void target_mixer_callback() {
    const uint8_t buf_idx = sfx_state::s_dma_current[2] - sfx_state::s_start_mid;
    __assume_count(buf_idx, 3);
    if (buf_idx == sfx_state::s_prev_buf_idx) return;
    // s_prev_buf_idx is the buffer DMA just left — fill it before DMA wraps back
    int8_t* mix_buffer = sfx_state::s_mix_start + sfx_state::s_prev_buf_idx * 256;
    sfx_state::s_prev_buf_idx = buf_idx;
    g_sfx_mixer_callback(mix_buffer);
}
#endif

static void setup_mixer() {
    sfx_state::s_dma_buffer = (int8_t*)_calloc(256, 4);
    sfx_state::s_mix_start = (int8_t*)(((size_t)sfx_state::s_dma_buffer + 255) & ~255);
#ifdef __M68000__
    timer_c::with_paused_timers([]{
        *(volatile uint8_t*)0xffff8901 &= 0xFE;       // Stop DMA
        sfx_state::s_dma_start.set(sfx_state::s_mix_start);
        sfx_state::s_dma_end.set(sfx_state::s_mix_start + 768);
        *(volatile uint8_t*)0xffff8921 = 0x81;         // 8-bit mono @ 12.5kHz
        *(volatile uint8_t*)0xffff8901 = 3;            // Start DMA loop
        sfx_state::s_start_mid = reinterpret_cast<size_t>(sfx_state::s_mix_start) >> 8;
        sfx_state::s_prev_buf_idx = 0;
        timer_c& clock = timer_c::shared(timer_c::timer_e::clock);
        clock.add_func((timer_c::func_t)target_mixer_callback, 50);
    });
#else
    host_bridge_c::shared().setup_mixer();
#endif
}

static void teardown_mixer() {
#ifdef __M68000__
    timer_c::with_paused_timers([]{
        timer_c& clock = timer_c::shared(timer_c::timer_e::clock);
        clock.remove_func((timer_c::func_t)target_mixer_callback);
        *(volatile uint8_t*)0xffff8901 &= 0xFE;       // Stop DMA
    });
#else
    host_bridge_c::shared().teardown_mixer();
#endif
}

void audio_mixer_c::play(const sound_c& sound, uint8_t priority) {
    timer_c::with_paused_timers([&] {
        uint8_t max_prio = 0;
        int idx = -1;
        for (int i = 0; i < 4; i++) {
            channel_t& channel = sfx_state::s_channels[i];
            if (channel.current == nullptr) {
                idx = i;
                break;
            } else if (channel.priority > priority && channel.priority > max_prio) {
                idx = i;
                max_prio = channel.priority;
            }
        }
        if (idx >= 0) {
            channel_t& channel = sfx_state::s_channels[idx];
            channel.current = sound.sample();
            channel.end = sound.sample() + sound.length();
            channel.repeat_length = sound.repeat_length();
            channel.priority = priority;
        }
    });
}

void audio_mixer_c::stop(const sound_c& sound) {
    // No-op for now.
}

void audio_mixer_c::play(const music_c& music, int track) {
    if (_active_music) {
        stop(*_active_music);
    }
    assert(track > 0 && "Track number must be positive");
    assert(music.format() == music_c::sndh && "Only SNDH supported");
#ifdef __M68000__
    // Generate trampolines for this music, if new song
    if (_active_music != &music) {
        uint8_t* data = const_cast<uint8_t*>(music.data());
        codegen_s::make_trampoline(_music_init_code, data + 0, false);
        codegen_s::make_trampoline(_music_exit_code, data + 4, false);
        codegen_s::make_trampoline(_music_play_code, data + 8, false);
        if (music.supports_command()) {
            codegen_s::make_trampoline(_music_cmd_code, data + 12, false);
        }
    }
    timer_c::with_paused_timers([this, track, &music] {
        timer_c& clock = timer_c::shared(timer_c::timer_e::clock);
        // init driver
        ((timer_c::func_i_t)_music_init_code)(track);
        // add VBL
        clock.add_func((timer_c::func_t)_music_play_code, music.replay_freq());
    });
#else
    host_bridge_c::shared().play(music, track);
#endif
    _active_music = &music;
    _active_track = track;
}

void audio_mixer_c::stop(const music_c& music) {
    assert(_active_music == &music && "Music being stopped must be active");
#ifdef __M68000__
    timer_c::with_paused_timers([this] {
        timer_c& clock = timer_c::shared(timer_c::timer_e::clock);
        // Exit driver
        ((timer_c::func_t)_music_exit_code)();
        // remove timer func
        clock.remove_func((timer_c::func_t)_music_play_code);
    });
#endif
    _active_music = nullptr;
    _active_track = 0;
}

long audio_mixer_c::command(const music_c& music, int cmd, long data, void* ctx) {
    assert(_active_music == &music && "Music being stopped must be active");
    assert(music.supports_command() && "Music does not support commands");
    long result = 0;
#ifdef __M68000__
    timer_c::with_paused_timers([&] {
        // Command driver
        result = ((long(*)(int cmd, long data, void* ctx))_music_cmd_code)(cmd, data, ctx);
    });
#endif
    return result;
}


void audio_mixer_c::stop_all() {
    if (_active_music) {
        stop(*_active_music);
    }
}

extern "C" void g_microwire_write(uint16_t value);

audio_mixer_c::audio_mixer_c() : _active_music(nullptr), _active_track(0) {
    setup_mixer();
#ifdef __M68000__
    g_microwire_write(0x4c | 40); // Max master volume (0 to 40)
    g_microwire_write(0x50 | 20); // Right volume (0 to 20)
    g_microwire_write(0x54 | 20); // Left volume (0 to 20)
#endif
}

audio_mixer_c::~audio_mixer_c() {
    stop_all();
    teardown_mixer();
};

#endif
