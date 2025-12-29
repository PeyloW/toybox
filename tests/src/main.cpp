//
//  main.cpp
//  toybox - tests
//
//  Created by Fredrik on 2025-10-12.
//

#include "shared.hpp"

int main(int argc, const char* argv[]) {
    // Test collections
    test_array_and_vector();
    test_dynamic_vector();
    test_list();
    test_map();
    
    // Test display list
    // Disable for now, display list destruction now requires the maschine_s singleton to be alive.
    //test_display_list();
    
    // Test algorithms
    test_algorithms();
    
    // Test math, especially fix16_t
    test_math();
    test_math_functions();
    
    // Test copy/move works as expected to lifetimes
    test_lifetime();

    // Test shared_ptr
    test_shared_ptr();

    // Test optionset and bitset
    test_optionset();
    test_bitset();

    printf("All pass.\n\r");
#ifndef TOYBOX_HOST
    while (getc(stdin) != ' ');
#endif
    return 0;
}
