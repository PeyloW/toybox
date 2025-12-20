//
//  stream.hpp
//  toybox
//
//  Created by Fredrik on 2024-04-15.
//

#pragma once

#include "core/cincludes.hpp"
#include "core/optionset.hpp"
#include "core/utility.hpp"
#include "core/memory.hpp"

namespace toybox {
    
    /**
     `stream_c` is vaguely related to `std::iostream`.
     */
    class stream_c : public nocopy_c {
    public:
        enum class seekdir_e : uint8_t {
            beg = SEEK_SET,
            cur = SEEK_CUR,
            end = SEEK_END
        };
        using manipulator_f = stream_c&(*)(stream_c&);
        
        stream_c();
        virtual ~stream_c() { flush(); }
            
        virtual bool good() const __pure;
        virtual ptrdiff_t tell() const __pure = 0;
        virtual ptrdiff_t seek(ptrdiff_t pos, seekdir_e way) = 0;
        virtual bool flush();

        virtual size_t read(uint8_t* buf, size_t count = 1) = 0;
        virtual size_t write(const uint8_t* buf, size_t count = 1) = 0;

        template<typename T> requires (!same_as<T, uint8_t>)
        __forceinline size_t read(T* buf, size_t count = 1) { return read(reinterpret_cast<uint8_t*>(buf), count * sizeof(T)); }
        template<typename T> requires (!same_as<T, uint8_t>)
        __forceinline size_t write(const T* buf, size_t count = 1) { return write(reinterpret_cast<const uint8_t*>(buf), count * sizeof(T)); }

        __forceinline int width() const { return _width; }
        int width(int w) { int t = _width; _width = w; return t; }
        __forceinline char fill() const { return _fill; }
        char fill(char d) { int t = _fill; _fill = d; return t; }

        stream_c& operator<<(manipulator_f m);
        stream_c& operator<<(const char* str);
        stream_c& operator<<(char c);
        stream_c& operator<<(unsigned char c);
        stream_c& operator<<(int16_t i);
        stream_c& operator<<(uint16_t i);
        stream_c& operator<<(int32_t i);
        stream_c& operator<<(uint32_t i);

    protected:
        int _width;
        char _fill;
    };
    
    extern stream_c& tbin;
    extern stream_c& tbout;
    extern stream_c& tberr;

    extern stream_c::manipulator_f endl;
    extern stream_c::manipulator_f ends;
    extern stream_c::manipulator_f flush;

    namespace detail {
        struct setw_s { int w; };
        static inline stream_c& operator<<(stream_c& s, const setw_s& m) { s.width(m.w); return s; }
        struct setfill_s { char c; };
        static inline stream_c& operator<<(stream_c& s, const setfill_s& m) { s.fill(m.c); return s; }
    }

    static __forceinline constexpr detail::setw_s setw(int w) { return (detail::setw_s){ w }; };
    static __forceinline constexpr detail::setfill_s setfill(char c) { return (detail::setfill_s){ c }; };
    

    class fstream_c final : public stream_c {
    public:
        enum class openmode_e : uint8_t {
            none = 0,
            input = 1 << 0,
            output = 1 << 1,
            append = 1 << 2
        };

        fstream_c(FILE* file);
        fstream_c(const char* path, openmode_e mode = openmode_e::input);
        virtual ~fstream_c();

        __forceinline openmode_e mode() const __pure { return _mode; }
        __forceinline bool is_open() const __pure { return _file != nullptr; }
        bool open();
        bool close();

        virtual bool good() const override __pure;
        virtual ptrdiff_t tell() const override __pure;
        virtual ptrdiff_t seek(ptrdiff_t pos, seekdir_e way) override;
        virtual bool flush() override;

        using stream_c::read;
        virtual size_t read(uint8_t* buf, size_t count = 1) override;
        using stream_c::write;
        virtual size_t write(const uint8_t* buf, size_t count = 1) override;

    private:
        const char* _path;  // Non-owning, caller must ensure lifetime
        openmode_e _mode;
        FILE* _file;
    };
    template<>
    struct is_optionset<fstream_c::openmode_e> : public true_type {};


    class strstream_c final : public stream_c {
    public:
        strstream_c(size_t len);
        strstream_c(char* buf, size_t len);
        virtual ~strstream_c() {};

        __forceinline void reset() { _pos = 0; }
        __forceinline char* str() { return _buf; };

        virtual ptrdiff_t tell() const override __pure;
        virtual ptrdiff_t seek(ptrdiff_t pos, seekdir_e way) override;

        using stream_c::read;
        virtual size_t read(uint8_t* buf, size_t count = 1) override;
        using stream_c::write;
        virtual size_t write(const uint8_t* buf, size_t count = 1) override;

    private:
        unique_ptr_c<char> _owned_buf;
        char* const _buf;
        const size_t _len;
        size_t _pos;
        size_t _max;
    };
    
}
