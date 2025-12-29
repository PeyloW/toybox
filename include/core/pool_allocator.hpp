//
//  pool_allocator.hpp
//  toybox
//
//  Created by Fredrik Olsson on 2024-03-24.
//

#pragma once

#include "core/algorithm.hpp"

namespace toybox {

/**
 A `pool_allocator_c` is a fixed-size memory pool allocator.
 Pre-allocates a pool of fixed-size blocks and manages them via a free list.
 This exists for performance, as `malloc`can be expensive.
 */
template <class T, size_t Count>
class pool_allocator_c {
    struct block_t;
public:
    static constexpr size_t alloc_size = sizeof(T);
    static constexpr size_t max_alloc_count = Count;
    using type = block_t*;
    static void* allocate() {
        assert(first_block && "Allocator pool exhausted");
#ifndef __M68000__
        _alloc_count++;
        _peak_alloc_count = MAX(_peak_alloc_count, _alloc_count);
#endif
        auto ptr = reinterpret_cast<T*>(&first_block->data[0]);
        first_block = first_block->next;
        return ptr;
    };
    static void deallocate(void* ptr) {
#ifndef __M68000__
        _alloc_count--;
#endif
        block_t* block = reinterpret_cast<block_t*>(static_cast<void**>(ptr) - 1);
        block->next = first_block;
        first_block = block;
    }
#ifndef __M68000__
    static int peak_alloc_count() { return _peak_alloc_count; }
#endif
private:
    struct block_t {
        block_t* next;
        uint8_t data[alloc_size];
    };
#ifndef __M68000__
    static inline int _alloc_count = 0;
    static inline int _peak_alloc_count = 0;
#endif
    static inline block_t* first_block = [] {
        static block_t s_blocks[Count];
        for (int i = 0; i < Count - 1; i++) {
            s_blocks[i].next = &s_blocks[i + 1];
        }
        s_blocks[Count - 1].next = nullptr;
        return &s_blocks[0];
    }();
};

/**
 Dynamic specialization of `pool_allocator_c` for Count == 0.
 Grows the pool automatically by allocating chunks as needed.
 Chunk sizes start at 8 and double each time (capped at 256).
 */
template <class T>
class pool_allocator_c<T, 0> {
    struct block_t;
public:
    static constexpr size_t alloc_size = sizeof(T);
    using type = block_t*;
    static void* allocate() {
        if (!first_block) _grow_pool();
        auto ptr = reinterpret_cast<T*>(&first_block->data[0]);
        first_block = first_block->next;
        return ptr;
    }
    static void deallocate(void* ptr) {
        block_t* block = reinterpret_cast<block_t*>(static_cast<void**>(ptr) - 1);
        block->next = first_block;
        first_block = block;
    }
private:
    struct block_t {
        block_t* next;
        uint8_t data[alloc_size];
    };
    static void _grow_pool() {
        const size_t chunk_size = MIN(s_next_chunk_size, size_t(256));
        block_t* chunk = new block_t[chunk_size];
        for (size_t i = 0; i < chunk_size - 1; ++i) {
            chunk[i].next = &chunk[i + 1];
        }
        chunk[chunk_size - 1].next = first_block;
        first_block = chunk;
        if (s_next_chunk_size < 256) {
            s_next_chunk_size *= 2;
        }
    }
    static inline block_t* first_block = nullptr;
    static inline size_t s_next_chunk_size = 8;
};

}
