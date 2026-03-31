//
//  audio_music.cpp
//  toybox
//
//  Created by Fredrik on 2024-02-18.
//

#include "media/audio.hpp"
#include <errno.h>

using namespace toybox;

// Case-insensitive check if path ends with given extension
static bool __path_has_suffix(const char* path, const char* suffix) {
    const int path_len = (int)strlen(path);
    const int suffix_len = (int)strlen(suffix);
    if (path_len < suffix_len) return false;
    const char* path_suffix = path + path_len - suffix_len;
    for (int i = 0; i < suffix_len; i++) {
        char c = path_suffix[i];
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (c != suffix[i]) return false;
    }
    return true;
}

music_c::music_c(const char* path) :
    _format(format_e::sndh),
    _data(nullptr),
    _length(0),
    _title(nullptr),
    _composer(nullptr),
    _track_count(1),
    _freq(50)
{
    // Determine format from file extension
    if (__path_has_suffix(path, ".snd")) {
        _format = format_e::sndh;
    } else if (__path_has_suffix(path, ".mod")) {
        _format = format_e::mod;
    } else {
        errno = EINVAL;
        return;
    }

    fstream_c file(path);
    if (!file.good()) {
        if (errno == 0) {
            errno = EINVAL;
        }
        return;
    }
    file.seek(0, stream_c::seekdir_e::end);
    const size_t size = file.tell();
    file.seek(0, stream_c::seekdir_e::beg);

    _data.reset((uint8_t*)_malloc(size));
    _length = size;
    const size_t read = file.read(_data.get(), size);
    assert(read == size && "Failed to read music file");

    if (_format == format_e::sndh) {
        char* header_str = nullptr;
        if (memcmp(_data.get() + 12, "SNDH", 4) == 0) {
            header_str = (char*)(_data.get() + 16);
        } else if (memcmp(_data.get() + 16, "SNDH", 4) == 0) {
            header_str = (char*)(_data.get() + 20);
            _supports_command = true;
        }
        assert(header_str && "File must be valid SNDH");
        while (strncmp(header_str, "HDNS", 4) != 0 && ((uint8_t*)header_str < _data.get() + 200)) {
            const int len = (int)strlen(header_str);
            if (len > 0) {
                if (len > 100) {
                    break;
                }
                if (strncmp(header_str, "TITL", 4) == 0) {
                    _title = header_str + 4;
                } else if (strncmp(header_str, "COMM", 4) == 0) {
                    _composer = header_str + 4;
                } else if (strncmp(header_str, "##", 2) == 0) {
                    _track_count = atoi(header_str + 2);
                } else if (strncmp(header_str, "TA", 2) == 0 ||
                           strncmp(header_str, "TB", 2) == 0 ||
                           strncmp(header_str, "TC", 2) == 0 ||
                           strncmp(header_str, "TD", 2) == 0 ||
                           strncmp(header_str, "!V", 2) == 0) {
                    _freq = atoi(header_str + 2);
                    assert(_freq != 0 && "Music frequency must be non-zero");
                }
            }
            header_str += len;
            while (*++header_str == 0);
        }
    } else if (_format == format_e::mod) {
        // TODO: Parse MOD file header
        hard_assert(false && "MOD format not yet supported");
    }
}
