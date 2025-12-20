//
//  util_stream.hpp
//  toybox
//
//  Created by Fredrik on 2024-05-19.
//

#pragma once

#include "core/stream.hpp"

namespace toybox {

    class substream_c final : public stream_c {
    public:
        substream_c(shared_ptr_c<stream_c> stream, ptrdiff_t origin, ptrdiff_t length) : _stream(move(stream)), _origin(origin), _length(length) {}
        virtual ~substream_c() {};

        virtual ptrdiff_t tell() const override __pure;
        virtual ptrdiff_t seek(ptrdiff_t pos, seekdir_e way) override;

        virtual size_t read(uint8_t* buf, size_t count = 1) override;
        virtual size_t write(const uint8_t* buf, size_t count = 1) override;

    private:
        shared_ptr_c<stream_c> _stream;
        ptrdiff_t _origin;
        ptrdiff_t _length;
    };
    
}
