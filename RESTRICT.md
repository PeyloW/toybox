# __restrict Candidates

Candidates for adding `__restrict` qualifier to enable compiler optimizations by indicating pointers don't alias.

## High Value

### dirtymap_c::merge() - `src/media/dirtymap.cpp:207`
```cpp
uint32_t* l_dest = (uint32_t*)_data;
const uint32_t* l_source = (uint32_t*)dirtymap._data;
```
- Hot path for dirty rect tracking
- Two buffers with OR operation in loop
- `const` source guarantees no write aliasing
- **Risk: LOW** - Separate objects, const source

### fill_music_buffer() - `src/machine/machine_sdl2.cpp:109`
```cpp
void fill_music_buffer(uint8_t* stream, int len)
```
- Hot path for audio rendering
- Output buffer written with memset and indexed writes
- Caller controls stream ownership
- **Risk: LOW** - Single output buffer, no aliasing expected

### hton(Type* buf, size_t count) - `include/core/utility.hpp:41`
```cpp
template<class Type>
void hton(Type* buf, size_t count)
```
- Called frequently during serialization/deserialization
- Single buffer walked sequentially
- **Risk: LOW** - Single buffer, no aliasing possible

### image_read() - `src/media/image.cpp:124`
```cpp
static void image_read(iffstream_c& file, uint16_t line_words, int height,
                       uint16_t* bitmap, uint16_t* maskmap)
```
- Image loading with separate bitmap and mask buffers
- Inner loop writes to both buffers
- **Risk: LOW** - Separate allocations, maskmap only used if non-null

### image_read_packbits() - `src/media/image.cpp:152`
```cpp
static void image_read_packbits(iffstream_c& file, uint16_t line_words, int height,
                                uint16_t* bitmap, uint16_t* maskmap)
```
- PackBits decompression to bitmap and mask buffers
- Hot path during image loading
- **Risk: LOW** - Same as image_read()

### image_write() - `src/media/image.cpp:293`
```cpp
static void image_write(iffstream_c& file, uint16_t line_words, uint16_t next_line_words,
                        int height, uint16_t* bitmap, uint16_t* maskmap)
```
- Image saving reads from bitmap and mask buffers
- Both buffers read-only during operation
- **Risk: LOW** - Read-only access to both buffers

### image_write_packbits() - `src/media/image.cpp:396`
```cpp
static void image_write_packbits(iffstream_c& file, uint16_t line_words, uint16_t next_line_words,
                                 int height, uint16_t* bitmap, uint16_t* maskmap)
```
- PackBits compression reading from bitmap and mask
- Significant inner loop work
- **Risk: LOW** - Same as image_write()

## Medium Value

### iffstream_c::write() - `include/core/iffstream.hpp:129`
```cpp
template<typename T>
size_t write(const T* buf, size_t count = 1)
```
- memcpy from const source to local tmp buffer
- **Risk: LOW** - Local destination, const source

### hton_struct() - `src/core/utility.cpp:14`
```cpp
void toybox::hton_struct(void* ptr, const char* layout)
```
- Generic byte-order conversion walking through struct
- Used during file I/O
- **Risk: MEDIUM** - Void pointer, relies on caller ensuring no overlap

### __set_active_stencil() - `src/media/canvas_atari.cpp:33`
```cpp
static __forceinline void __set_active_stencil(
    struct blitter_s* blitter,
    const canvas_c::stencil_t* const stencil)
```
- 32-byte memcpy to hardware register
- **Risk: MEDIUM** - Hardware memory; may need `__target_volatile` instead

### canvas_c::imp_draw_aligned() - `include/media/canvas.hpp:132`
```cpp
void imp_draw_aligned(const image_c& srcImage, const rect_s& rect, point_s point) const
```
- Hot path sprite blitting
- Source image and destination canvas
- **Risk: MEDIUM** - Off-screen rendering could have overlapping buffers

### make_dither_mask() [8x8] - `src/media/canvas_stencil.cpp:69`
```cpp
void make_dither_mask(canvas_c::stencil_t stencil, const uint8_t mask_8x8[8][8], int shade)
```
- Stencil generation from 8x8 dither pattern
- Output stencil written, input mask read-only
- **Risk: LOW** - Const input, separate output

### make_dither_mask() [16x16] - `src/media/canvas_stencil.cpp:84`
```cpp
void make_dither_mask(canvas_c::stencil_t stencil, const uint8_t mask_16x16[16][16], int shade)
```
- Stencil generation from 16x16 dither pattern
- Double nested loop with threshold comparison
- **Risk: LOW** - Const input, separate output

### make_dither_mask() [func] - `src/media/canvas_stencil.cpp:98`
```cpp
void make_dither_mask(canvas_c::stencil_t stencil, int (*func)(int), int shade)
```
- Stencil generation using threshold function
- **Risk: LOW** - Function pointer input, separate output

### tile_s::operator=() - `include/runtime/tilemap.hpp:24-28`
```cpp
tile_s& operator=(const tile_s& o) { memcpy(this, &o, sizeof(tile_s)); return *this; }
tile_s& operator=(tile_s&& o) { memcpy(this, &o, sizeof(tile_s)); return *this; }
```
- Tile copy/move using memcpy
- Called frequently during tilemap updates
- **Risk: MEDIUM** - Self-assignment possible (need guard or accept UB)

## Low Value

### color_c::get() - `include/media/palette.hpp:38`
```cpp
constexpr void get(uint8_t* r_out, uint8_t* g_out, uint8_t* b_out) const
```
- Three output pointers
- **Risk: LOW** - Caller provides distinct pointers
- *Note: constexpr already well-optimized*

### base_rect_s::intersection() - `include/core/geometry.hpp:116`
```cpp
constexpr bool intersection(const base_rect_s& rect, base_rect_s& intersection_out) const
```
- Output reference parameter
- **Risk: MEDIUM** - Output could alias with `this` or `rect`
- *Note: constexpr already well-optimized*

### dirtymap_c::clear() - `src/media/dirtymap.cpp:289`
```cpp
void dirtymap_c::clear()
```
- memset on member buffer
- **Risk: LOW** - No external aliases
- *Note: Member function, compiler already knows object*

## Do NOT Add __restrict

### system_helpers memory operations - `include/core/system_helpers.hpp`
- `move_inc_to()` (line 33)
- `move_inc_from()` (line 54)
- `move_inc_from_to()` (line 71)
- `or_inc_to()` (line 89)
- `move_dec_from_to()` (line 110)

These functions **explicitly support overlapping memory regions** for backwards copying. Adding `__restrict` would break correctness.

## Summary

| Candidate | Value | Risk | Action |
|-----------|-------|------|--------|
| `dirtymap_c::merge()` | HIGH | LOW | Add |
| `fill_music_buffer()` | HIGH | LOW | Add |
| `hton()` template | HIGH | LOW | Add |
| `image_read()` | HIGH | LOW | Add |
| `image_read_packbits()` | HIGH | LOW | Add |
| `image_write()` | HIGH | LOW | Add |
| `image_write_packbits()` | HIGH | LOW | Add |
| `iffstream_c::write()` | MEDIUM | LOW | Add |
| `hton_struct()` | MEDIUM | MEDIUM | Consider |
| `__set_active_stencil()` | MEDIUM | MEDIUM | Consider |
| `imp_draw_aligned()` | MEDIUM | MEDIUM | Consider |
| `make_dither_mask()` (3 overloads) | MEDIUM | LOW | Add |
| `tile_s::operator=()` | MEDIUM | MEDIUM | Consider |
| `color_c::get()` | LOW | LOW | Optional |
| `base_rect_s::intersection()` | LOW | MEDIUM | Skip |
| `dirtymap_c::clear()` | LOW | LOW | Skip |
| system_helpers | N/A | HIGH | Never |
