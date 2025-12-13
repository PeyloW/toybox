//
//  stream.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-15.
//

#include "core/stream.hpp"
#include "core/util_stream.hpp"

using namespace toybox;

stream_c::stream_c() : _width(0), _fill(' ') {};

bool stream_c::good() const { return true; };
bool stream_c::flush() { return true; }

stream_c& stream_c::operator<<(manipulator_f m) {
    return m(*this);
}

stream_c& stream_c::operator<<(const char* str) {
    auto len = strlen(str);
    write(reinterpret_cast<const uint8_t*>(str), len);
    return *this;
}

stream_c& stream_c::operator<<(char c) {
    write(reinterpret_cast<const uint8_t*>(&c), 1);
    return *this;
}
stream_c& stream_c::operator<<(unsigned char c) {
    write(reinterpret_cast<const uint8_t*>(&c), 1);
    return *this;
}
stream_c& stream_c::operator<<(int16_t i) {
    return *this << static_cast<int32_t>(i);
}
stream_c& stream_c::operator<<(uint16_t i) {
    return *this << static_cast<uint32_t>(i);
}
stream_c& stream_c::operator<<(int32_t i) {
    char buf[12];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#ifdef __M68000__
    sprintf(buf, "%ld", i);
#else
    sprintf(buf, "%d", i);
#endif
#pragma GCC diagnostic pop
    if (_width > 0) {
        int len = static_cast<int>(strlen(buf));
        int fillc = _width - len;
        if (fillc > 0) {
            char buf[fillc];
            int i;
            while_dbra_count(i, fillc) buf[i] = _fill;
            write(reinterpret_cast<uint8_t*>(buf), fillc);
        }
    }
    return *this << static_cast<const char*>(buf);
}
stream_c& stream_c::operator<<(uint32_t i) {
    if (i > 0x7fffffff) {
        if (_width > 6) {
            int ow = _width;
            _width -= 6;
            *this << (i / 1000000);
            _width = 0;
            *this << (i % 1000000);
            _width = ow;
        } else {
            *this << (i / 1000000);
            *this << (i % 1000000);
        }
        return *this;
    } else {
        return *this << static_cast<int32_t>(i);
    }
}

static fstream_c s_tbin(stdin);
static fstream_c s_tbout(stdout);
static fstream_c s_tberr(stderr);

stream_c& toybox::tbin = s_tbin;
stream_c& toybox::tbout = s_tbout;
stream_c& toybox::tberr = s_tberr;

static stream_c& s_endl(stream_c& s) {
#ifdef __M68000__
    return s << "\n\r";
#else
    return s << '\n';
#endif
}
static stream_c& s_ends(stream_c& s) {
    s.write(reinterpret_cast<const uint8_t*>(""), 1);
    return s;
}
static stream_c& s_flush(stream_c& s) {
    s.flush();
    return s;
}

stream_c::manipulator_f toybox::endl = &s_endl;
stream_c::manipulator_f toybox::ends = &s_ends;
stream_c::manipulator_f toybox::flush = &s_flush;

fstream_c::fstream_c(FILE* file) :
    stream_c(), _path(nullptr), _mode(openmode_e::input), _file(file)
{}

fstream_c::fstream_c(const char* path, openmode_e mode) :
    stream_c(), _path(path), _mode(mode), _file(nullptr)
{
    open();
}

fstream_c::~fstream_c() {
    if (is_open() && _path) {
        close();
    }
}

static constexpr const char* mode_for_mode(fstream_c::openmode_e mode) {
    constexpr const char* const s_table[8] = {
        nullptr, "r", "w", "w+",
        "a", "a+", "a+", nullptr
    };
    assert((uint8_t)mode < 8 && "Mode value must be less than 8");
    assert(s_table[(uint8_t)mode] && "Mode must have a valid string representation");
    return s_table[(uint8_t)mode];
}

bool fstream_c::open() {
    bool r = false;
    if (_file == nullptr) {
        _file = _fopen(_path, mode_for_mode(_mode));
        r = _file != nullptr;
    }
    return r;
}

bool fstream_c::close() {
    bool r = false;
    if (_file) {
        r = fclose(_file) == 0;
        _file = nullptr;
    }
    return r;
}

bool fstream_c::good() const { return is_open(); };

ptrdiff_t fstream_c::tell() const {
    auto r = ftell(_file);
    return r;
}

ptrdiff_t fstream_c::seek(ptrdiff_t pos, stream_c::seekdir_e way) {
    auto r = fseek(_file, pos, (int)way);
    return r;
}
bool fstream_c::flush() { return true; }

size_t fstream_c::read(uint8_t* buf, size_t count) {
    return fread(buf, 1, count, _file);
}
size_t fstream_c::write(const uint8_t* buf, size_t count) {
    return fwrite(buf, 1, count, _file);
}

strstream_c::strstream_c(size_t len) :
    _owned_buf(static_cast<char*>(_malloc(len))), _buf(_owned_buf.get()), _len(len), _pos(0), _max(0)
{}

strstream_c::strstream_c(char* buf, size_t len) :
    _owned_buf(), _buf(buf), _len(len), _pos(0), _max(0)
{}

ptrdiff_t strstream_c::tell() const {
    return _pos;
}
ptrdiff_t strstream_c::seek(ptrdiff_t pos, seekdir_e way) {
    assert(ABS(pos <= _len) && "Seek position must be within buffer length");
    switch (way) {
        case seekdir_e::beg:
            _pos = static_cast<int>(pos);
            break;
        case seekdir_e::cur:
            _pos += pos;
            break;
        case seekdir_e::end:
            _pos = _max - static_cast<int>(pos);
            break;
    }
    if (_pos < 0 || _pos >= _len) {
        _pos = -1;
    }
    return _pos;
}

size_t strstream_c::read(uint8_t* buf, size_t count) {
    count = MIN(count, _max - _pos);
    memcpy(buf, _buf + _pos, count);
    _pos += count;
    return count;
}

size_t strstream_c::write(const uint8_t* buf, size_t count) {
    count = MIN(count, _len - _pos);
    memcpy(_buf + _pos, buf, count);
    _pos += count;
    _max = MAX(_max, _pos);
    return count;
}
