//
//  iffstream.cpp
//  toybox
//
//  Created by Fredrik on 2024-04-15.
//

#include "core/iffstream.hpp"
#include "core/util_stream.hpp"
#include "core/expected.hpp"

using namespace toybox;

bool cc4_t::matches(cc4_t match) const {
    for (int i = 0; i < 4; i++) {
        if (match.ubytes[i] == '?') continue;
        if (match.ubytes[i] != ubytes[i]) return false;
    }
    return true;
}

const char* cc4_t::cstring() const {
    static char buf[5];
    for (int i = 0; i < 4; i++) buf[i] = ubytes[i];
    buf[4] = 0;
    return buf;
}

iffstream_c::iffstream_c(shared_ptr_c<stream_c> stream) :
    stream_c(), _stream(move(stream))
{
    assert(_stream && "Stream must not be null");
}

iffstream_c::iffstream_c(const char* path, fstream_c::openmode_e mode) {
    auto fstream = new expected_c<fstream_c>(failable, path, mode);
    if (*fstream) {
        construct_at(this, shared_ptr_c<stream_c>(expected_cast(fstream)));
    } else {
        errno = fstream->error();
        delete fstream;
    }
}

bool iffstream_c::good() const { return _stream->good(); }
ptrdiff_t iffstream_c::tell() const { return _stream->tell(); }
ptrdiff_t iffstream_c::seek(ptrdiff_t pos, seekdir_e way) { return _stream->seek(pos, way); }

bool iffstream_c::first(cc4_t id, iff_chunk_s& chunk_out) {
    bool result = false;
    if (seek(0, seekdir_e::beg) == 0) {
        if (read(chunk_out)) {
            result = chunk_out.id.matches(id);
        }
    }
    return result;
}

bool iffstream_c::first(cc4_t id, cc4_t subtype, iff_group_s& group_out) {
    bool result = false;
    if (seek(0, seekdir_e::beg) == 0) {
        if (read(group_out)) {
            result = group_out.id.matches(id) && group_out.subtype.matches(subtype);
        }
    }
    return result;
}

bool iffstream_c::next(const iff_group_s& in_group, cc4_t id, iff_chunk_s& chunk_out) {
    // Hard assert handled by called functions.
    const long end = in_group.offset + sizeof(uint32_t) * 2 + in_group.size;
    long pos = tell();
    while (tell() < end  && read(chunk_out)) {
        if (chunk_out.id.matches(id)) {
            return true;
        }
        if (!skip(chunk_out)) {
            break;
        }
    }
    seek(pos, seekdir_e::beg);
    return false;
}


bool iffstream_c::expand(const iff_chunk_s& chunk, iff_group_s& group_out) {
    // Hard assert handled by called functions.
    group_out.offset = chunk.offset;
    group_out.id = chunk.id;
    group_out.size = chunk.size;
    if (reset(group_out)) {
        return read(&group_out.subtype);
    }
    return false;
}


bool iffstream_c::reset(const iff_chunk_s& chunk) {
    return seek(chunk.offset + sizeof(uint32_t) * 2, seekdir_e::beg) >= 0;
}

bool iffstream_c::skip(const iff_chunk_s& chunk) {
    bool result = false;
    long end = chunk.offset + sizeof(uint32_t) * 2 + chunk.size;
    if ((result = seek(end, seekdir_e::beg) >= 0)) {
        result = align(false);
    }
    return result;
}

bool iffstream_c::align(bool for_write) {
    long pos = tell();
    bool result = pos >= 0;
    if (result) {
        if ((pos & 1) != 0) {
            uint8_t tmp = 0;
            result = (for_write ? write(&tmp) : read(&tmp)) == 1;
            goto done;
        }
    }
done:
    return result;
}

bool iffstream_c::begin(cc4_t id, iff_chunk_s& chunk_out) {
    bool result = false;
    if (align(true)) {
        chunk_out.offset = tell();
        if (chunk_out.offset >= 0) {
            chunk_out.id = id;
            chunk_out.size = -1;
            result = write(&chunk_out.id) && write(&chunk_out.size);
        }
    }
    return result;
}

bool iffstream_c::end(iff_chunk_s& chunk) {
    bool result = false;
    long pos = tell();
    if (pos >= 0) {
        uint32_t size = static_cast<uint32_t>(pos - (chunk.offset + 8));
        chunk.size = size;
        if (seek(chunk.offset + 4, seekdir_e::beg) >= 0) {
            if (write(&size)) {
                result = seek(pos, seekdir_e::beg) >= 0;
            }
        }
    }
    return result;
}

bool iffstream_c::read(iff_group_s& group_out) {
    bool result = false;
    if (read(static_cast<iff_chunk_s&>(group_out))) {
        result = read(&group_out.subtype);
    }
    return result;
}

bool iffstream_c::read(iff_chunk_s& chunk_out) {
    bool result = align(false);
    if (result) {
        chunk_out.offset = tell();
        if (chunk_out.offset >= 0) {
            result = read(&chunk_out.id) && read(&chunk_out.size);
        }
    }
    return result;
}

size_t iffstream_c::read(uint8_t* buf, size_t count) { return _stream->read(buf, count); }

size_t iffstream_c::write(const uint8_t* buf, size_t count) { return _stream->write(buf, count); };
