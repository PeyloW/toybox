//
//  shared.hpp
//  toybox
//
//  Created by Fredrik on 2025-11-09.
//

#pragma once

#include "core/cincludes.hpp"
#include "core/utility.hpp"

using namespace toybox;

struct non_trivial_s {
    int value;
    int generation;
    bool moved;
    inline static int s_destructors = 0;
    
    non_trivial_s() : value(0), generation(0), moved(false) {}

    explicit non_trivial_s(int v) : value(v), generation(0), moved(false) {}
    
    virtual ~non_trivial_s() {
        ++s_destructors;
    }

    non_trivial_s(const non_trivial_s& other)
        : value(other.value), generation(other.generation + 1), moved(false) {}

    non_trivial_s(non_trivial_s&& other)
        : value(other.value), generation(other.generation), moved(false) {
        other.moved = true;
    }

    non_trivial_s& operator=(const non_trivial_s& other) {
        if (this != &other) {
            value = other.value;
            generation = other.generation + 1;
            moved = false;
        }
        return *this;
    }

    non_trivial_s& operator=(non_trivial_s&& other) {
        if (this != &other) {
            value = other.value;
            generation = other.generation;
            moved = false;
            other.moved = true;
        }
        return *this;
    }
};

// Test function declarations
void test_array_and_vector();
void test_dynamic_vector();
void test_list();
void test_display_list();
void test_algorithms();
void test_math();
void test_math_functions();
void test_lifetime();
void test_shared_ptr();
void test_optionset();
void test_bitset();
