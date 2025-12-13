//
//  cincludes.hpp
//  toybox
//
//  Created by Fredrik on 2023-12-30.
//  Copyright © 2023 TOYS. All rights reserved.
//

#pragma once

#include "core/config.hpp"

extern "C" {
    
#define __pure __attribute__ ((pure))
#define __forceinline __attribute__((__always_inline__)) inline
#define __forceinline_lambda __attribute__((__always_inline__))
#define __neverinline __attribute__((noinline))
    
#ifdef __M68000__
#define __target_volatile volatile
#else
#define __target_volatile
#endif

#ifndef MAX
#   define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#   define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif

#ifndef ABS
#   define ABS(X) (((X) < 0) ? -(X) : (X))
#endif
      
    extern "C" {
        bool __hard_assert(const char* message);
    }
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)
#define hard_assert(expr) \
    ((void)((expr)?false:__hard_assert("\nHard assert [" STRINGIFY(expr) "]: (" #expr "), in " __FILE__ ": " STRINGIFY(__LINE__) "\n")))

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
}

extern "C" {

    FILE* log_file();
#if TOYBOX_LOG_MALLOC
    extern FILE* g_malloc_log;
    static void* _malloc(size_t n) {
        auto b = (Malloc(-1));
        auto p = malloc(n);
        auto a = (Malloc(-1));
        fprintf(log_file(), "Malloc %ld [%ld -> %ld].\n", n, b, a);
        hard_assert(p != nullptr);
        return p;
    }

    static void* _calloc(size_t c, size_t n) {
        auto b = (Malloc(-1));
        auto p = calloc(c, n);
        auto a = (Malloc(-1));
        fprintf(log_file(), "Calloc %ld [%ld -> %ld].\n", (c * n), b, a);
        hard_assert(p != nullptr);
        return p;
    }
    static void* _free(void* p) {
        auto b = (Malloc(-1));
        free(p);
        auto a = (Malloc(-1));
        fprintf(log_file(), "Free [%ld -> %ld].\n", b, a);
    }
#else
#   define _malloc malloc
#   define _calloc calloc
#   define _free free
#endif
    
#ifdef TOYBOX_HOST
    void _add_searchpath(const char* path);
    FILE* _fopen(const char* path, const char* mode);
#else
#   define _fopen fopen
#endif
    
}

#ifndef TOYBOX_HOST
// Required for inplace new
extern void* operator new (size_t count, void* p);
#else
#include <new>
#endif

namespace toybox {
    using nullptr_t = decltype(nullptr);
}

#include "core/system_helpers.hpp"
