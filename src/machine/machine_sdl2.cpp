//
//  machine_host.cpp
//  toybox
//
//  Created by Fredrik on 2025-09-27.
//
#ifdef TOYBOX_HOST
#include "machine/machine.hpp"
#include "machine/host_bridge.hpp"
#include "media/viewport.hpp"
#include "media/display_list.hpp"
#include "media/audio.hpp"
#include "machine/timer.hpp"
#include "SDL.h"
#include <atomic>
#include <mutex>
#include <libgen.h>
#include <unistd.h>
#if HAVE_LIBPSGPLAY
extern "C" {
#include <psgplay.h>
}
#endif

using namespace toybox;

class sdl2_host_bridge final : public host_bridge_c {
public:
    sdl2_host_bridge(machine_c& machine) : _machine(machine) {
        const auto screen_size = machine.screen_size();
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
        _window = SDL_CreateWindow("ToyBox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_size.width * 2, screen_size.height * 2, SDL_WINDOW_SHOWN);
        _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
        _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, screen_size.width, screen_size.height);
        SDL_RaiseWindow(_window);

        SDL_AudioSpec desired;
        SDL_zero(desired);
        desired.freq = 12517;                // STe/Falcon lowest compatible rate
        desired.format = AUDIO_S8;           // 8-bit signed audio (based on int8_t sample)
        desired.channels = 1;                // Mono audio
        desired.samples = 4096;              // Buffer size (can be tuned)
        desired.callback = nullptr;          // No callback, we'll use SDL_QueueAudio
        _effectts_device_id = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
        
        // Initialize joystick if available
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                _controller = SDL_GameControllerOpen(i);
                break;
            }
        }
        
        set_shared(this);
    }
    ~sdl2_host_bridge() {
#if HAVE_LIBPSGPLAY
        if (_psg) {
            if (_music_device_id) {
                SDL_PauseAudioDevice(_music_device_id, 1);
                SDL_CloseAudioDevice(_music_device_id);
                _music_device_id = 0;
            }
            psgplay_free(_psg);
            _psg = nullptr;
        }
#endif
        if (_controller) SDL_GameControllerClose(_controller);
        if (_effectts_device_id) SDL_CloseAudioDevice(_effectts_device_id);
        if (_texture) SDL_DestroyTexture(_texture);
        if (_renderer) SDL_DestroyRenderer(_renderer);
        if (_window) SDL_DestroyWindow(_window);
        SDL_Quit();
        set_shared(nullptr);
    }
    
    virtual void yield() override {
        resume_timers();
        SDL_Delay(1);
        pause_timers();
    }

    virtual void pause_timers() override {
        _timer_mutex.lock();
    }
    
    virtual void resume_timers() override {
        _timer_mutex.unlock();
    }

    virtual void play(const sound_c& sound) override {
        // Get the audio sample data, length, and sample rate
        const int8_t* sample_data = sound.sample();
        uint32_t sample_length = sound.length();
        uint16_t sample_rate = sound.rate();
        
        if (!sample_data || sample_length == 0 || sample_rate == 0 || _effectts_device_id == 0) {
            return; // Invalid sound sample, exit early
        }
        
        // Unpause the audio device to start playback
        SDL_PauseAudioDevice(_effectts_device_id, 0);
        
        // Queue the audio sample for playback
        SDL_QueueAudio(_effectts_device_id, sample_data, sample_length);
    }
    
#if HAVE_LIBPSGPLAY
    void fill_music_buffer(uint8_t* stream, int len) {
        if (!_psg) {
            memset(stream, 0, len);
            return;
        }

        // Calculate number of stereo samples needed
        const int sample_count = len / sizeof(psgplay_stereo);
        struct psgplay_stereo buffer[sample_count];

        // Read stereo samples from libpsgplay
        const ssize_t read = psgplay_read_stereo(_psg, buffer, sample_count);

        if (read <= 0) {
            // End of music or error - fill with silence
            memset(stream, 0, len);
            return;
        }

        // Convert psgplay_stereo to interleaved int16_t format for SDL with volume scaling
        int16_t* output = reinterpret_cast<int16_t*>(stream);
        int i;
        do_dbra(i, read - 1) {
            output[i * 2] = static_cast<int16_t>(buffer[i].left * _music_volume);
            output[i * 2 + 1] = static_cast<int16_t>(buffer[i].right * _music_volume);
        } while_dbra(i);

        // Fill remaining with silence if we got fewer samples than requested
        if (read < sample_count) {
            const int remaining_bytes = (sample_count - read) * sizeof(psgplay_stereo);
            memset(stream + (read * sizeof(psgplay_stereo)), 0, remaining_bytes);
        }
    }

    static void music_callback(void* userdata, uint8_t* stream, int len) {
        auto* bridge = static_cast<sdl2_host_bridge*>(userdata);
        bridge->fill_music_buffer(stream, len);
    }

    virtual void play(const music_c& music, int track) override {
        _psg = psgplay_init(music.data(), music.length(), track, 22050);

        if (!_psg) return;

        // Setup audio spec for music playback
        SDL_AudioSpec desired;
        SDL_zero(desired);
        desired.freq = 22050;
        desired.format = AUDIO_S16SYS;
        desired.channels = 2;
        desired.samples = 4096;
        desired.callback = music_callback;
        desired.userdata = this;

        // Open music audio device
        _music_device_id = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);

        if (_music_device_id == 0) {
            psgplay_free(_psg);
            _psg = nullptr;
            return;
        }

        // Start playback
        SDL_PauseAudioDevice(_music_device_id, 0);
    }
#endif
    
    void draw_display_list(const shared_ptr_c<display_list_c>& display) {
        const viewport_c* active_viewport = nullptr;
        const palette_c* active_palette = nullptr;
        for (const auto& entry : *display) {
            switch (entry.item().display_type()) {
                case display_item_c::viewport:
                    active_viewport = &entry.viewport();
                    break;
                case display_item_c::palette:
                    active_palette = &entry.palette();
                    break;
                default:
                    hard_assert(false && "Unsupported pixel format");
                    break;
            }
        }

        // Clear buffer to black
        struct color_s { uint8_t rgb[3]; uint8_t _; };
        const auto screen_size = machine_c::shared().screen_size();
        color_s buffer[screen_size.width * screen_size.height];
        memset(buffer, 0, sizeof(color_s) * screen_size.width * screen_size.height);
        
        if (active_viewport != NULL) {
            // If active image is set...
            {
                const auto size = active_viewport->image().size();
                hard_assert(size.width >= screen_size.width && size.height >= screen_size.height);
            }
            color_s palette[16] = { 0 };
            if (active_palette) {
                // If active palette is set
                for (int i = 0; i < 16; i++) {
                    (*active_palette)[i].get(&palette[i].rgb[0], &palette[i].rgb[1], &palette[i].rgb[2]);
                    buffer[i]._ = 0;
                }
            }
            const point_s offset = active_viewport->offset();
            const image_c& image = active_viewport->image();
            point_s at;
            for (at.y = 0; at.y < screen_size.height; at.y++) {
                for (at.x = 0; at.x < screen_size.width; at.x++) {
                    const auto c = get_pixel(image, at + offset, false);
                    if (c != image_c::MASKED_CIDX) {
                        auto offset = (at.y * screen_size.width + at.x);
                        buffer[offset] = palette[c];
                    }
                }
            }
        }
        void* pixels;
        int pitch;
        SDL_LockTexture(_texture, nullptr, &pixels, &pitch);
        hard_assert(pitch / 4 == screen_size.width && "SDL pitch mismatch");
        memcpy(pixels, buffer, pitch * screen_size.height);
        SDL_UnlockTexture(_texture);
    }
    
    void vbl_interupt() {
        // Called on main thread by SDL timer
        std::lock_guard<std::recursive_mutex> lock(_timer_mutex);
        host_bridge_c::vbl_interupt();
    }

    void clock_interupt() {
        // Called on main thread by SDL timer
        std::lock_guard<std::recursive_mutex> lock(_timer_mutex);
        host_bridge_c::clock_interupt();
    }

    int run(machine_c::machine_f f) {
        static std::atomic<bool> s_should_quit{false};
        static int s_status = 0;
        struct Payload {
            int (*game_func)(machine_c&);
            machine_c* machine_inst;
            sdl2_host_bridge* host_bridge;
        };
        Payload payload{f, &_machine, this};

        _thread = SDL_CreateThread(
            [](void* data) -> int {
                auto* p = static_cast<Payload*>(data);
                p->host_bridge->pause_timers();
                s_status = p->game_func(*p->machine_inst);
                p->host_bridge->resume_timers();
                s_should_quit.store(true);
                return s_status;
            },
            "GameThread",
            static_cast<void*>(&payload)
        );

        timer_c& vbl = timer_c::shared(timer_c::timer_e::vbl);
        _vbl_timer = SDL_AddTimer(1000 / vbl.base_freq(), vbl_cb, this);
        _clock_timer = SDL_AddTimer(5, clock_cb, this);
        directions_e joy_directions = controller_c::none;
        bool joy_fire = false;
        
        while (!s_should_quit.load()) {
            SDL_Event event;
            shared_ptr_c<display_list_c> previous_display_list;
            while (SDL_PollEvent(&event)) {
                bool update_joy = false;
                switch (event.type) {
                    case SDL_QUIT:
                        s_should_quit.store(true);
                        break;
                    case SDL_MOUSEMOTION:
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP: {
                        int x, y;
                        Uint32 buttons = SDL_GetMouseState(&x, &y);
                        bool left = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
                        bool right = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
                        host_bridge_c::shared().update_mouse(point_s(x / 2, y / 2), left, right);
                        break;
                    }
                    case SDL_CONTROLLERAXISMOTION: {
                        static constexpr Sint16 deadzone = 8000;
                        switch (event.caxis.axis) {
                            case SDL_CONTROLLER_AXIS_LEFTY:
                                joy_directions = joy_directions - controller_c::up - controller_c::down;
                                if (event.caxis.value < -deadzone) {
                                    joy_directions = joy_directions + controller_c::up;
                                } else if (event.caxis.value > deadzone) {
                                    joy_directions = joy_directions + controller_c::down;
                                }
                                update_joy = true;
                                break;
                            case SDL_CONTROLLER_AXIS_LEFTX:
                                joy_directions = joy_directions - controller_c::left - controller_c::right;
                                if (event.caxis.value < -deadzone) {
                                    joy_directions = joy_directions + controller_c::left;
                                } else if (event.caxis.value > deadzone) {
                                    joy_directions = joy_directions + controller_c::right;
                                }
                                update_joy = true;
                                break;
                            default:
                        }
                        break;
                    }
                    case SDL_CONTROLLERBUTTONDOWN:
                        if (event.cbutton.button == 0) {
                            joy_fire = true;
                            update_joy = true;
                        }
                        break;
                    case SDL_CONTROLLERBUTTONUP:
                        if (event.cbutton.button == 0) {
                            joy_fire = false;
                            update_joy = true;
                        }
                        break;
                    default:
                        break;
                }
                if (update_joy) {
                    host_bridge_c::shared().update_joystick(joy_directions, joy_fire);
                }
            }

            {
                std::lock_guard<std::recursive_mutex> lock(_timer_mutex);
                auto display_list = _machine.active_display_list();
                if (display_list != previous_display_list) {
                    draw_display_list(display_list);
                    previous_display_list = display_list;
                }
            }

            SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
            SDL_RenderClear(_renderer);
            SDL_RenderCopy(_renderer, _texture, nullptr, nullptr);
            SDL_RenderPresent(_renderer);
        }

        SDL_RemoveTimer(_vbl_timer);
        SDL_RemoveTimer(_clock_timer);

        SDL_DetachThread(_thread);
        _thread = nullptr;
        return s_status;
    }
    
private:
    SDL_Window* _window = nullptr;
    SDL_Renderer* _renderer = nullptr;
    SDL_Texture* _texture = nullptr;
    SDL_Thread* _thread = nullptr;
    SDL_AudioDeviceID _effectts_device_id;
#if HAVE_LIBPSGPLAY
    struct psgplay* _psg = nullptr;
    SDL_AudioDeviceID _music_device_id = 0;
    static constexpr float _music_volume = 0.5f;
#endif
    SDL_GameController* _controller = nullptr;
    machine_c& _machine;
    std::recursive_mutex _timer_mutex;
    Uint32 _vbl_timer = 0;
    Uint32 _clock_timer = 0;

    static Uint32 vbl_cb(Uint32 interval, void* param) {
        static Uint64 last_tick = 0;
        Uint64 tick = SDL_GetTicks64();
        static_cast<sdl2_host_bridge*>(param)->vbl_interupt();
        
        auto& vbl = timer_c::shared(timer_c::timer_e::vbl);
        const Uint64 ideal_interval = (1000ULL * vbl.tick()) / vbl.base_freq();
        const Uint64 elapsed = tick - last_tick;
        last_tick = tick;
        interval = static_cast<Uint32>(ideal_interval - (tick - (1000ULL * vbl.tick() / vbl.base_freq())));
        interval = MIN(MAX(10, interval), 20);
        
        return interval;
    }
    static Uint32 clock_cb(Uint32 interval, void* param) {
        static_cast<sdl2_host_bridge*>(param)->clock_interupt();
        return interval;
    }
};

int machine_c::with_machine(int argc, const char* argv[], machine_f f) {
    assert(_shared_machine == nullptr && "Shared machine already initialized");
    char* dir = dirname((char*)argv[0]);
    hard_assert(chdir(dir) == 0 && "Failed to change directory");
    
    if (argc > 1) {
        _add_searchpath(argv[1]);
    }
    
    machine_c machine;
    _shared_machine = &machine;
    sdl2_host_bridge bridge(machine);
    int status = bridge.run(f);
    _shared_machine = nullptr;
    return status;
}

#endif
