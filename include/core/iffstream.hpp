//
//  iffstream.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-15.
//

#pragma once

#include "core/stream.hpp"
#include "core/utility.hpp"

namespace toybox {

    struct cc4_t {
        union {
            uint32_t ulong;
            uint8_t  ubytes[4];
        };

        constexpr cc4_t() : ulong(0x20202020) {}

        // Must be max 4 characters in range 32 to 127 (except '?'), ubytes is padded with ' '.
        consteval cc4_t(const char* cc4) {
            assert(cc4 != nullptr && "CC4 must not be null.");
            for (int i = 0; i < 4; i++) {
                ubytes[i] = *cc4 == '*' ? '?' : (*cc4 ? *cc4++ : ' ');
                assert(ubytes[i] >= 32 && "Invalid CC4 character.");
            }
        }
        // Must be initialized with big endian uint32_t.
        constexpr cc4_t(uint32_t ul) : ulong(ul) {
#ifndef __M68000__
            for (int i = 0; i < 4; i++) {
                assert(ubytes[i] >= 32 && "Invalid CC4 character.");
            }
#endif
        }
        constexpr cc4_t(const uint8_t ub[4]) {
            for (int i = 0; i < 4; i++) {
                ubytes[i] = ub[i];
                assert(ubytes[i] >= 32 && "Invalid CC4 character.");
            }
        };

        constexpr bool operator==(cc4_t o) const { return ulong == o.ulong; }
        constexpr bool operator==(uint32_t ul) const { return ulong == ul; }
        constexpr bool operator==(uint8_t ub[4]) const { return ubytes == ub; }

        // Allow ? to match any character, and * to match any until end.
        // Example: "?LVL" matches "1LVL" and "2LVL". "LVL*" matches any cc4 starting with LVL.
        bool matches(cc4_t m) const;

        // Return an inner pointer to a cstring representation valid until next call to cstring().
        const char* cstring() const;
    };
    static_assert(sizeof(cc4_t) == 4);

    template<>
    struct struct_layout<cc4_t> {
        static constexpr const char* value = "4b";
    };

    namespace cc4 {
        static constexpr cc4_t FORM("FORM");
        static constexpr cc4_t LIST("LIST");
        static constexpr cc4_t CAT("CAT");
        static constexpr cc4_t TEXT("TEXT");
        static constexpr cc4_t NAME("NAME");
        static constexpr cc4_t NULL_("");
        static constexpr cc4_t ANY("*");
    }
    
    struct iff_chunk_s {
        long offset;
        cc4_t id;
        uint32_t size;
    };

    struct iff_group_s : public iff_chunk_s {
        cc4_t subtype;
    };

    /// An `iffstream_c` handles reading and writing to an EA IFF file.
    class iffstream_c final : public stream_c {
    public:
        using unknown_reader = function_c<bool(iffstream_c& stream,iff_chunk_s& chunk)>;
        inline static const constexpr unknown_reader null_reader{};
        using unknown_writer = function_c<bool(iffstream_c& stream)>;
        inline static const constexpr unknown_writer null_writer{};

        iffstream_c(shared_ptr_c<stream_c> stream);
        iffstream_c(const char* path, fstream_c::openmode_e mode = fstream_c::openmode_e::input);
        ~iffstream_c() = default;
                
        virtual bool good() const override __pure;
        virtual ptrdiff_t tell() const override __pure;
        virtual ptrdiff_t seek(ptrdiff_t pos, seekdir_e way) override;

        bool first(cc4_t id, iff_chunk_s& chunk_out);
        bool first(cc4_t id, cc4_t subtype, iff_group_s& group_out);
        bool next(const iff_group_s& in_group, cc4_t id, iff_chunk_s& chunk_out);
        bool expand(const iff_chunk_s& chunk, iff_group_s& group_out);

        bool reset(const iff_chunk_s& chunk);
        bool skip(const iff_chunk_s& chunk);
        bool align(bool for_write);

        bool begin(cc4_t id, iff_chunk_s& chunk_out);
        bool end(iff_chunk_s& chunk);
        
        using stream_c::read;
        virtual size_t read(uint8_t* buf, size_t count = 1) override;

        using stream_c::write;
        virtual size_t write(const uint8_t* buf, size_t count = 1) override;

        
#ifndef __M68000__
        template<typename T> requires (!same_as<T, uint8_t>)
        size_t read(T* buf, size_t count = 1) {
            auto result = read(reinterpret_cast<uint8_t*>(buf), count * sizeof(T));
            if (result) {
                hton(buf, count);
            }
            return  result;
        }
 
        template<typename T> requires (!same_as<T, uint8_t>)
        size_t write(const T* buf, size_t count = 1) {
            T tmp[count];
            memcpy(tmp, buf, count * sizeof(T));
            hton(&tmp[0], count);
            return write(reinterpret_cast<const uint8_t*>(&tmp), count * sizeof(T));
        }
#endif

    private:
        bool read(iff_group_s& group_out);
        bool read(iff_chunk_s& chunk_out);

        shared_ptr_c<stream_c> _stream;
    };
        
}
