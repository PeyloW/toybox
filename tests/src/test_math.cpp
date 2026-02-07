//
//  test_math.cpp
//  toybox - tests
//
//  Created by Fredrik on 2025-11-09.
//

#include "shared.hpp"

#include "core/math.hpp"

__neverinline void test_math() {
    printf("== Start: test_math\n\r");
    auto r1 = mul_fast((uint16_t)50000, (uint16_t)2);
    auto r2 = mul_fast((int16_t)30000, (int16_t)-3);
    hard_assert(sizeof(r1) == 4 && sizeof(r2) == 4 && "Result is 32 bit");
    hard_assert(r1 == 100000 && "Result is did not overflow");
    hard_assert(r2 == -90000 && "Result is did not overflow");

    auto r3 = div_fast((uint32_t)100000, (uint16_t)2);
    auto r4 = div_fast((int32_t)-90000, (int16_t)3);
    hard_assert(sizeof(r3.quot) == 2 && sizeof(r4.quot) == 2 && "Result is 16 bit");
    hard_assert(r3.quot == 50000 && "Result is correct");
    hard_assert(r4.quot == -30000 && "Result is correct");

    hard_assert(numbers::one);
    hard_assert(!fix16_t(0));

    fix16_t f1 = numbers::one;
    hard_assert(f1);
    hard_assert(f1.raw == 0x10);
    fix16_t zero = 0;
    hard_assert(!zero);

    hard_assert(fix16_t(1.5) * fix16_t(1.5) == fix16_t(2.25));
    hard_assert(fix16_t(1.5) * 2 == 3);
    hard_assert(fix16_t(-10) * fix16_t(-3.1415) == fix16_t(31.25)); // Should be 31.375, but * does not round.

    hard_assert(fix16_t(10) / 3 == fix16_t(3.333f));
    hard_assert(fix16_t(10) % 3 == fix16_t(1.0f));
    hard_assert(fix16_t(12.25) / 3 == fix16_t(4.0833f));
    hard_assert(fix16_t(12.25) % 3 == fix16_t(0.25f));
    hard_assert(fix16_t(7.5) / fix16_t(3.25) == fix16_t(2.25f)); // Should be 2.3125, but / does not round.
    hard_assert(fix16_t(7.5) % fix16_t(3.25) == 1);

    hard_assert(truncf(3.14) == 3);
    hard_assert(roundf(3.14) == 3);
    hard_assert(truncf(2.72) == 2);
    hard_assert(roundf(2.72) == 3);
    hard_assert(truncf(-3.14) == -3);
    hard_assert(roundf(-3.14) == -3);
    hard_assert(truncf(-2.72) == -2);
    hard_assert(roundf(-2.72) == -3);
    hard_assert(roundf(-0.914518 * 16) / 16 == -0.937500);

    auto pi2 = numbers::pi * 2;
    hard_assert(pi2.raw == 100);

    hard_assert(trunc(numbers::pi) == 3);
    hard_assert(floor(numbers::pi) == 3);
    hard_assert(ceil(numbers::pi) == 4);
    hard_assert(round(numbers::pi) == 3);

    hard_assert(trunc(numbers::e) == 2);
    hard_assert(floor(numbers::e) == 2);
    hard_assert(ceil(numbers::e) == 3);
    hard_assert(round(numbers::e) == 3);


    printf("test_math pass.\n\r");
}

__neverinline void test_math_functions() {
    printf("== Start: test_math_functions\n\r");
    using namespace rel_ops;
    using namespace numbers;

    hard_assert(pow(fix16_t(2), fix16_t(2)) == fix16_t(4));
    hard_assert(pow(fix16_t(3.0f), fix16_t(3.0f)) == fix16_t(27.0f));
    hard_assert(pow(fix16_t(25), fix16_t(0.5f)) == fix16_t(5));
    hard_assert(pow(fix16_t(10), fix16_t(1.5f)) == fix16_t(31.6228));

    hard_assert(sqrti(25) == 5);
    hard_assert(sqrti(22500) == 150);
    hard_assert(sqrti(10) == 3);

    hard_assert(sqrt(25) == 5);
    hard_assert(sqrt(10) == fix16_t(3.1622f));
    hard_assert(sqrt(2) == fix16_t(1.4142f));

    hard_assert(sin(0) == fix16_t(0));
    hard_assert(sin(1) == fix16_t(0.841471f));
    hard_assert(sin(2) == fix16_t(0.909297f));
    hard_assert(sin(3) == fix16_t(0.141120f));
    hard_assert(sin(4) == fix16_t(-0.756802f));
    hard_assert(sin(pi) == fix16_t(0));
    hard_assert(sin(pi2x) == fix16_t(0));
    hard_assert(sin(pi_2) == fix16_t(1));
    hard_assert(sin(-1) == fix16_t(-0.8750f)); // Should be 0.8125, but double rounding
    hard_assert(sin(-pi) == fix16_t(0));
    hard_assert(sin(-pi2x) == fix16_t(0));
    hard_assert(sin(-pi_2) == fix16_t(-1));

    hard_assert(sin(0) == cos(pi_2));

    hard_assert(tan(0) == 0);
    hard_assert(tan(pi_2) > fix16_t(1000));
    hard_assert(tan(-(pi / 4)) == -tan(pi / 4));
    hard_assert(tan(pi) == 0);

    printf("test_math_functions pass.\n\r");
}
