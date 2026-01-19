//
//  test_algorithms.cpp
//  toybox - tests
//
//  Created by Fredrik on 2025-11-09.
//

#include "shared.hpp"

#include "core/algorithm.hpp"
#include "core/list.hpp"

__neverinline void test_algorithms() {
    printf("== Start: test_algorithms\n\r");
    constexpr int numbers[4] = { 1, 7, 2, 0 };
    int buffer[4];

    hard_assert(!is_sorted(begin(numbers), end(numbers)) && "Unsorted array should not be sorted");

    copyn(begin(numbers), 4, begin(buffer));
    hard_assert(!is_sorted(begin(buffer), end(buffer)) && "Copied unsorted array should not be sorted");
    sort(begin(buffer), end(buffer));
    hard_assert(is_sorted(begin(buffer), end(buffer)) && "Array should be sorted after sort");

    hard_assert(binary_search(begin(buffer), end(buffer), 2) && "Binary search should find 2");
    hard_assert(!binary_search(begin(buffer), end(buffer), 3) && "Binary search should not find 3");

    copy_backward(begin(numbers), end(numbers), end(buffer));
    hard_assert(!is_sorted(begin(buffer), end(buffer)) && "Array should not be sorted after copy_backward");

    auto f = find_if(begin(buffer), end(buffer), [](auto&v) { return v == 2; });
    hard_assert(*f == 2 && "find_if should find value 2");
    f = find_if(begin(buffer), end(buffer), [](auto&v) { return v == 3; });
    hard_assert(f == end(buffer) && "find_if should not find value 3");

    list_c<int, 4> list;
    list.push_front(4);
    list.push_front(5);
    list.push_front(0);
    hard_assert(!is_sorted(list.begin(), list.end()) && "List should not be sorted initially");

    copy(list.begin(), list.end(), begin(buffer));
    hard_assert(buffer[0] == 0 && buffer[1] == 5 && buffer[2] == 4 && "Copied list values should match");
    sort(list.begin(), list.end());
    move(list.begin(), list.end(), begin(buffer));
    hard_assert(buffer[0] == 0 && buffer[1] == 4 && buffer[2] == 5 && "Moved sorted list values should match");

    printf("test_algorithms pass.\n\r");
}
