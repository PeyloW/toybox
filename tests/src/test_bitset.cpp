//
//  test_bitset.cpp
//  toybox - tests
//
//  Created by Fredrik on 2025-12-18.
//

#include "shared.hpp"

#include "core/optionset.hpp"
#include "core/bitset.hpp"

// Define a test enum with is_optionset
enum class test_flags_e : uint8_t {
    none = 0,
    flag_a = 1 << 0,
    flag_b = 1 << 1,
    flag_c = 1 << 2,
    flag_d = 1 << 3,
    all = flag_a | flag_b | flag_c | flag_d
};

template<>
struct toybox::is_optionset<test_flags_e> : true_type {};

void test_optionset() {
    printf("== Start: test_optionset\n\r");

    // Test default value
    test_flags_e flags = test_flags_e::none;
    hard_assert(flags == false && "none should equal false");
    hard_assert(!(flags == true) && "none should not equal true");

    // Test single flag
    flags = test_flags_e::flag_a;
    hard_assert(flags == true && "single flag should equal true");
    hard_assert(!(flags == false) && "single flag should not equal false");

    // Test OR operator
    flags = test_flags_e::flag_a | test_flags_e::flag_b;
    hard_assert(flags == true);
    hard_assert((flags & test_flags_e::flag_a) == true && "flag_a should be set");
    hard_assert((flags & test_flags_e::flag_b) == true && "flag_b should be set");
    hard_assert((flags & test_flags_e::flag_c) == false && "flag_c should not be set");

    // Test + operator (same as OR)
    flags = test_flags_e::flag_c + test_flags_e::flag_d;
    hard_assert((flags & test_flags_e::flag_c) == true);
    hard_assert((flags & test_flags_e::flag_d) == true);
    hard_assert((flags & test_flags_e::flag_a) == false);

    // Test - operator (remove flag)
    flags = test_flags_e::all;
    flags = flags - test_flags_e::flag_b;
    hard_assert((flags & test_flags_e::flag_a) == true);
    hard_assert((flags & test_flags_e::flag_b) == false && "flag_b should be removed");
    hard_assert((flags & test_flags_e::flag_c) == true);
    hard_assert((flags & test_flags_e::flag_d) == true);

    // Test |= operator
    flags = test_flags_e::flag_a;
    flags |= test_flags_e::flag_b;
    hard_assert((flags & test_flags_e::flag_a) == true);
    hard_assert((flags & test_flags_e::flag_b) == true);

    // Test += operator
    flags = test_flags_e::none;
    flags += test_flags_e::flag_c;
    hard_assert((flags & test_flags_e::flag_c) == true);

    // Test &= operator
    flags = test_flags_e::all;
    flags &= test_flags_e::flag_a | test_flags_e::flag_b;
    hard_assert((flags & test_flags_e::flag_a) == true);
    hard_assert((flags & test_flags_e::flag_b) == true);
    hard_assert((flags & test_flags_e::flag_c) == false);
    hard_assert((flags & test_flags_e::flag_d) == false);

    // Test -= operator
    flags = test_flags_e::all;
    flags -= test_flags_e::flag_a;
    flags -= test_flags_e::flag_c;
    hard_assert((flags & test_flags_e::flag_a) == false);
    hard_assert((flags & test_flags_e::flag_b) == true);
    hard_assert((flags & test_flags_e::flag_c) == false);
    hard_assert((flags & test_flags_e::flag_d) == true);

    // Test bool == optionset (reversed comparison)
    flags = test_flags_e::flag_a;
    hard_assert(true == flags);
    hard_assert(!(false == flags));
    flags = test_flags_e::none;
    hard_assert(false == flags);
    hard_assert(!(true == flags));

    printf("test_optionset pass.\n\r");
}

__neverinline static void __test_bitset_basic() {
    // Test default construction
    bitset_c<uint16_t> bs;
    hard_assert(!bs && "default bitset should be false");

    // Test single bit construction
    bitset_c<uint16_t> bs_single(3);
    hard_assert(bs_single && "bitset with bit set should be true");
    hard_assert(bs_single[3] && "bit 3 should be set");
    hard_assert(!bs_single[0] && "bit 0 should not be set");
    hard_assert(!bs_single[2] && "bit 2 should not be set");
    hard_assert(!bs_single[4] && "bit 4 should not be set");

    // Test operator[] assignment
    bs[0] = true;
    bs[5] = true;
    bs[15] = true;
    hard_assert(bs[0] && "bit 0 should be set");
    hard_assert(bs[5] && "bit 5 should be set");
    hard_assert(bs[15] && "bit 15 should be set");
    hard_assert(!bs[1] && "bit 1 should not be set");
    hard_assert(!bs[14] && "bit 14 should not be set");

    // Test clearing bits
    bs[5] = false;
    hard_assert(!bs[5] && "bit 5 should be cleared");
    hard_assert(bs[0] && "bit 0 should still be set");
    hard_assert(bs[15] && "bit 15 should still be set");
}

__neverinline static void __test_bitset_operators() {
    // Test + operator (union)
    bitset_c<uint8_t> a(1);
    bitset_c<uint8_t> b(2);
    const auto c = a + b;
    hard_assert(c[1] && "union should have bit 1");
    hard_assert(c[2] && "union should have bit 2");
    hard_assert(!c[0] && "union should not have bit 0");

    // Test - operator (difference)
    bitset_c<uint8_t> d(1);
    d[2] = true;
    d[3] = true;
    bitset_c<uint8_t> e(2);
    const auto f = d - e;
    hard_assert(f[1] && "difference should have bit 1");
    hard_assert(!f[2] && "difference should not have bit 2");
    hard_assert(f[3] && "difference should have bit 3");

    // Test & operator (intersection)
    bitset_c<uint8_t> g(1);
    g[2] = true;
    bitset_c<uint8_t> h(2);
    h[3] = true;
    const auto i = g & h;
    hard_assert(!i[1] && "intersection should not have bit 1");
    hard_assert(i[2] && "intersection should have bit 2");
    hard_assert(!i[3] && "intersection should not have bit 3");

    // Test equality
    bitset_c<uint8_t> j(4);
    bitset_c<uint8_t> k(4);
    hard_assert(j == k && "identical bitsets should be equal");
    k[1] = true;
    hard_assert(!(j == k) && "different bitsets should not be equal");

    // Test equality with int (checks if specific bit is set)
    bitset_c<uint8_t> l(3);
    l[5] = true;
    hard_assert(l == 3 && "should match bit 3");
    hard_assert(l == 5 && "should match bit 5");
    hard_assert(!(l == 0) && "should not match bit 0");
    hard_assert(!(l == 4) && "should not match bit 4");
}

__neverinline static void __test_bitset_iterator() {
    // Test empty bitset iteration
    bitset_c<uint8_t> empty;
    int count = 0;
    for (const int bit : empty) {
        (void)bit;
        ++count;
    }
    hard_assert(count == 0 && "empty bitset should iterate zero times");

    // Test single bit iteration
    bitset_c<uint8_t> single(3);
    count = 0;
    int last_bit = -1;
    for (const int bit : single) {
        last_bit = bit;
        ++count;
    }
    hard_assert(count == 1 && "single bit should iterate once");
    hard_assert(last_bit == 3 && "should iterate bit 3");

    // Test multiple bits iteration
    bitset_c<uint8_t> multi;
    multi[1] = true;
    multi[3] = true;
    multi[5] = true;
    multi[7] = true;

    int bits[4];
    count = 0;
    for (const int bit : multi) {
        hard_assert(count < 4);
        bits[count++] = bit;
    }
    hard_assert(count == 4 && "should iterate 4 times");
    hard_assert(bits[0] == 1 && "first bit should be 1");
    hard_assert(bits[1] == 3 && "second bit should be 3");
    hard_assert(bits[2] == 5 && "third bit should be 5");
    hard_assert(bits[3] == 7 && "fourth bit should be 7");

    // Test iterator with all bits set
    bitset_c<uint8_t> all;
    for (int i = 0; i < 8; ++i) {
        all[i] = true;
    }
    count = 0;
    for (const int bit : all) {
        hard_assert(bit == count && "bits should be in order");
        ++count;
    }
    hard_assert(count == 8 && "should iterate 8 times");

    // Test post-increment
    bitset_c<uint8_t> bs(2);
    bs[5] = true;
    auto it = bs.begin();
    hard_assert(*it == 2 && "first bit should be 2");
    auto prev = it++;
    hard_assert(*prev == 2 && "post-increment should return old value");
    hard_assert(*it == 5 && "after increment should be 5");
}

void test_bitset() {
    printf("== Start: test_bitset\n\r");
    __test_bitset_basic();
    __test_bitset_operators();
    __test_bitset_iterator();
    printf("test_bitset pass.\n\r");
}
