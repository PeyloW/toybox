//
//  test_collections.cpp
//  toybox - tests
//
//  Created by Fredrik on 2025-11-09.
//

#include "shared.hpp"

#include "core/array.hpp"
#include "core/vector.hpp"
#include "core/list.hpp"
#include "core/map.hpp"

__neverinline void test_array_and_vector() {
    printf("== Start: test_array_and_vector\n\r");
    array_s<int, 4> arr = { 1, 5, 2, 1 };
    hard_assert(arr.size() == 4);
    hard_assert(!is_sorted(arr.begin(), arr.end()));

    vector_c<int, 5> vec = { 1, 2, 3 };
    hard_assert(vec.size() == 3);
    vec.push_back(4);
    hard_assert(vec.size() == 4);
    vec.emplace(0, 0);
    hard_assert(vec.size() == 5);
    hard_assert(is_sorted(vec.begin(), vec.end()));
    hard_assert(binary_search(vec.begin(), vec.end(), 0));
    hard_assert(binary_search(vec.begin(), vec.end(), 2));
    hard_assert(binary_search(vec.begin(), vec.end(), 4));
    hard_assert(*vec.erase(vec.begin()) == 1);
    hard_assert(vec.erase(vec.end() - 1) == vec.end());
    hard_assert(*vec.erase(1) == 3);
    hard_assert(vec.size() == 2);
    hard_assert(vec[0] == 1);
    hard_assert(vec[1] == 3);
    vec.clear();
    hard_assert(vec.size() == 0);

    for (const auto i : arr) {
        vec.insert(vec.begin(), i);
    }
    hard_assert(vec.size() == 4);
    for (int i = 0; i < 4; i++) {
        hard_assert(arr[3 - i] == vec[i]);
    }

    // Test resize() on static vector - grow
    vec.clear();
    vec.push_back(10);
    vec.push_back(20);
    vec.resize(4);
    hard_assert(vec.size() == 4 && "Size should be 4 after resize grow");
    hard_assert(vec[0] == 10 && "First element should be preserved");
    hard_assert(vec[1] == 20 && "Second element should be preserved");
    hard_assert(vec[2] == 0 && "Third element should be default-constructed");
    hard_assert(vec[3] == 0 && "Fourth element should be default-constructed");

    // Test resize() on static vector - shrink
    vec.resize(1);
    hard_assert(vec.size() == 1 && "Size should be 1 after resize shrink");
    hard_assert(vec[0] == 10 && "Element should be preserved after resize shrink");

    // Test resize() on static vector - same size (no-op)
    vec.resize(1);
    hard_assert(vec.size() == 1 && "Size should remain 1 after resize to same size");
    hard_assert(vec[0] == 10 && "Element should be unchanged after resize to same size");

    // Test resize() on static vector - to zero
    vec.resize(0);
    hard_assert(vec.size() == 0 && "Size should be 0 after resize to zero");

    printf("test_array_and_vector pass.\n\r");
}

__neverinline void test_dynamic_vector() {
    printf("== Start: test_dynamic_vector\n\r");
    // Test dynamic vector with Count == 0
    vector_c<int, 0> vec;
    hard_assert(vec.size() == 0 && "Initial size should be 0");
    hard_assert(vec.capacity() == 0 && "Initial capacity should be 0");

    // Test automatic growth on push_back
    vec.push_back(1);
    hard_assert(vec.size() == 1 && "Size should be 1 after first push");
    hard_assert(vec.capacity() >= 1 && "Capacity should be at least 1");
    hard_assert(vec[0] == 1 && "First element should be 1");

    // Test growth to default capacity
    for (int i = 2; i <= 10; ++i) {
        vec.push_back(i);
    }
    hard_assert(vec.size() == 10 && "Size should be 10");
    hard_assert(vec.capacity() >= 10 && "Capacity should be at least 10");

    // Verify all elements
    for (int i = 0; i < 10; ++i) {
        hard_assert(vec[i] == i + 1 && "Element values should be correct");
    }

    // Test reserve
    vec.reserve(100);
    hard_assert(vec.capacity() >= 100 && "Capacity should be at least 100 after reserve");
    hard_assert(vec.size() == 10 && "Size should remain 10 after reserve");

    // Test that elements are preserved after reserve
    for (int i = 0; i < 10; ++i) {
        hard_assert(vec[i] == i + 1 && "Elements should be preserved after reserve");
    }

    // Test emplace_back
    vec.emplace_back(11);
    hard_assert(vec.size() == 11 && "Size should be 11 after emplace");
    hard_assert(vec[10] == 11 && "Emplaced element should be 11");

    // Test insert
    vec.insert(vec.begin(), 0);
    hard_assert(vec.size() == 12 && "Size should be 12 after insert");
    hard_assert(vec[0] == 0 && "First element should be 0 after insert");
    hard_assert(vec[1] == 1 && "Second element should be 1 after insert");

    // Test erase
    vec.erase(vec.begin());
    hard_assert(vec.size() == 11 && "Size should be 11 after erase");
    hard_assert(vec[0] == 1 && "First element should be 1 after erase");

    // Test front and back
    hard_assert(vec.front() == 1 && "Front should be 1");
    hard_assert(vec.back() == 11 && "Back should be 11");

    // Test pop_back
    vec.pop_back();
    hard_assert(vec.size() == 10 && "Size should be 10 after pop_back");
    hard_assert(vec.back() == 10 && "Back should be 10 after pop_back");

    // Test clear
    vec.clear();
    hard_assert(vec.size() == 0 && "Size should be 0 after clear");
    hard_assert(vec.capacity() >= 100 && "Capacity should remain allocated after clear");

    // Test growth after clear
    for (int i = 0; i < 5; ++i) {
        vec.push_back(i * 2);
    }
    hard_assert(vec.size() == 5 && "Size should be 5 after refill");
    for (int i = 0; i < 5; ++i) {
        hard_assert(vec[i] == i * 2 && "Elements should be correct after refill");
    }

    // Test resize() - grow
    vec.resize(10);
    hard_assert(vec.size() == 10 && "Size should be 10 after resize grow");
    for (int i = 0; i < 5; ++i) {
        hard_assert(vec[i] == i * 2 && "Existing elements should be preserved after resize grow");
    }
    for (int i = 5; i < 10; ++i) {
        hard_assert(vec[i] == 0 && "New elements should be default-constructed (zero) after resize grow");
    }

    // Test resize() - shrink
    vec.resize(3);
    hard_assert(vec.size() == 3 && "Size should be 3 after resize shrink");
    for (int i = 0; i < 3; ++i) {
        hard_assert(vec[i] == i * 2 && "Elements should be preserved after resize shrink");
    }

    // Test resize() - same size (no-op)
    vec.resize(3);
    hard_assert(vec.size() == 3 && "Size should remain 3 after resize to same size");
    for (int i = 0; i < 3; ++i) {
        hard_assert(vec[i] == i * 2 && "Elements should be unchanged after resize to same size");
    }

    // Test resize() - grow from zero
    vec.clear();
    vec.resize(7);
    hard_assert(vec.size() == 7 && "Size should be 7 after resize from zero");
    for (int i = 0; i < 7; ++i) {
        hard_assert(vec[i] == 0 && "All elements should be default-constructed after resize from zero");
    }

    // Test resize() - shrink to zero
    vec.resize(0);
    hard_assert(vec.size() == 0 && "Size should be 0 after resize to zero");

    printf("test_dynamic_vector pass.\n\r");
}

struct test_list_state_s {
    list_c<non_trivial_s, 0> list;
    int first_gen;
    bool first_moved;
};

__neverinline void test_list_basic_insert(test_list_state_s& state) {
    // Test 1: push_front with rvalue - should not affect existing elements
    state.list.push_front(non_trivial_s(100));
    hard_assert(state.list.size() == 1 && "List size should be 1");
    hard_assert(state.list.front().value == 100 && "Front value should be 100");
    hard_assert(state.list.front().generation == 1 && "Front generation should be 1 (copied from temporary)");
    hard_assert(!state.list.front().moved && "Front should not be marked as moved");

    // Store generation of first element to verify it's not copied/moved later
    state.first_gen = state.list.front().generation;
    state.first_moved = state.list.front().moved;

    // Test 2: push_front with lvalue - existing element should not be affected
    non_trivial_s lvalue1(200);
    state.list.push_front(lvalue1);
    hard_assert(state.list.size() == 2 && "List size should be 2");
    hard_assert(state.list.front().value == 200 && "Front value should be 200");
    hard_assert(state.list.front().generation == 1 && "Front generation should be 1");
    hard_assert(!state.list.front().moved && "Front should not be moved");

    // Verify first element (now second) was not affected
    auto it = state.list.begin();
    ++it;
    hard_assert(it->value == 100 && "Second element value should be 100");
    hard_assert(it->generation == state.first_gen && "Second element generation should not change");
    hard_assert(it->moved == state.first_moved && "Second element moved flag should not change");

    // Test 3: emplace_front - existing elements should not be affected
    state.list.emplace_front(300);
    hard_assert(state.list.size() == 3 && "List size should be 3");
    hard_assert(state.list.front().value == 300 && "Front value should be 300");
    hard_assert(state.list.front().generation == 0 && "Front generation should be 0 (direct construction)");

    // Verify previous elements unchanged
    it = state.list.begin();
    ++it;
    hard_assert(it->value == 200 && "Second element should be 200");
    hard_assert(it->generation == 1 && "Second element generation unchanged");
    hard_assert(!it->moved && "Second element not moved");
    ++it;
    hard_assert(it->value == 100 && "Third element should be 100");
    hard_assert(it->generation == state.first_gen && "Third element generation unchanged");
    hard_assert(it->moved == state.first_moved && "Third element moved flag unchanged");
    
    printf("  test_list_basic_insert pass.\n\r");
}

__neverinline void test_list_insert_after(test_list_state_s& state) {
    // Test 4: insert_after - verify existing elements not affected
    auto it = state.list.begin();
    non_trivial_s lvalue2(250);
    state.list.insert_after(it, lvalue2);
    hard_assert(state.list.size() == 4 && "List size should be 4");

    // Check inserted element
    it = state.list.begin();
    ++it;
    hard_assert(it->value == 250 && "Inserted element should be 250");
    hard_assert(it->generation == 1 && "Inserted element generation should be 1");

    // Verify all other elements unchanged
    it = state.list.begin();
    hard_assert(it->value == 300 && "1st element unchanged");
    hard_assert(it->generation == 0 && "1st element generation unchanged");
    ++it; ++it; // Skip to third
    hard_assert(it->value == 200 && "3rd element unchanged");
    hard_assert(it->generation == 1 && "3rd element generation unchanged");
    ++it;
    hard_assert(it->value == 100 && "4th element unchanged");
    hard_assert(it->generation == state.first_gen && "4th element generation unchanged");

    // Test 5: emplace_after
    it = state.list.begin();
    ++it; ++it; // At element 200
    state.list.emplace_after(it, 150);
    hard_assert(state.list.size() == 5 && "List size should be 5");

    // Verify emplaced element
    ++it;
    hard_assert(it->value == 150 && "Emplaced element should be 150");
    hard_assert(it->generation == 0 && "Emplaced element generation should be 0");

    printf("  test_list_insert_after pass.\n\r");
}

__neverinline void test_list_removal(test_list_state_s& state) {
    // Test 6: pop_front - verify remaining elements not affected
    state.list.pop_front();
    hard_assert(state.list.size() == 4 && "List size should be 4 after pop_front");
    hard_assert(state.list.front().value == 250 && "New front should be 250");

    // Verify remaining elements unchanged
    auto it = state.list.begin();
    ++it;
    hard_assert(it->value == 200 && "Element unchanged after pop");
    hard_assert(it->generation == 1 && "Element generation unchanged after pop");
    ++it;
    hard_assert(it->value == 150 && "Element unchanged after pop");
    ++it;
    hard_assert(it->value == 100 && "Element unchanged after pop");
    hard_assert(it->generation == state.first_gen && "Element generation unchanged after pop");

    // Test 7: erase_after - verify remaining elements not affected
    it = state.list.begin();
    state.list.erase_after(it); // Erase 200
    hard_assert(state.list.size() == 3 && "List size should be 3 after erase_after");

    // Verify remaining elements unchanged
    it = state.list.begin();
    hard_assert(it->value == 250 && "1st element unchanged");
    ++it;
    hard_assert(it->value == 150 && "2nd element unchanged (200 erased)");
    hard_assert(it->generation == 0 && "2nd element generation unchanged");
    ++it;
    hard_assert(it->value == 100 && "3rd element unchanged");
    hard_assert(it->generation == state.first_gen && "3rd element generation unchanged");
    
    printf("  test_list_removal pass.\n\r");
}

__neverinline void test_list_splice_single(test_list_state_s& state) {
    // Test 8: splice_after - verify elements not copied/moved, just linked
    list_c<non_trivial_s, 0> list2;
    list2.push_front(non_trivial_s(400));
    list2.push_front(non_trivial_s(500));

    // Store generation of element to be spliced
    int splice_gen = list2.front().generation;

    // Splice 500 from list2 into list after 250
    auto it = state.list.begin();
    state.list.splice_after(it, list2, list2.before_begin());

    hard_assert(state.list.size() == 4 && "List size should be 4 after splice");
    hard_assert(list2.size() == 1 && "List2 size should be 1 after splice");

    // Verify spliced element retains its properties (no copy/move)
    it = state.list.begin();
    ++it;
    hard_assert(it->value == 500 && "Spliced element should be 500");
    hard_assert(it->generation == splice_gen && "Spliced element generation unchanged (no copy)");
    hard_assert(!it->moved && "Spliced element not marked as moved");

    // Verify other elements in list unchanged
    it = state.list.begin();
    hard_assert(it->value == 250 && "1st element unchanged");
    ++it; ++it; // Skip spliced element
    hard_assert(it->value == 150 && "3rd element unchanged");
    hard_assert(it->generation == 0 && "3rd element generation unchanged");
    ++it;
    hard_assert(it->value == 100 && "4th element unchanged");
    hard_assert(it->generation == state.first_gen && "4th element generation unchanged");

    // Test 9: splice_front - verify elements not copied/moved
    int splice_gen2 = list2.front().generation;
    state.list.splice_front(list2, list2.before_begin());

    hard_assert(state.list.size() == 5 && "List size should be 5 after splice_front");
    hard_assert(list2.size() == 0 && "List2 size should be 0 after splice_front");

    // Verify spliced element at front retains properties
    hard_assert(state.list.front().value == 400 && "Front should be 400 (spliced)");
    hard_assert(state.list.front().generation == splice_gen2 && "Spliced front generation unchanged");
    hard_assert(!state.list.front().moved && "Spliced front not marked as moved");

    // Verify all other elements unchanged
    it = state.list.begin();
    ++it;
    hard_assert(it->value == 250 && "2nd element unchanged");
    ++it;
    hard_assert(it->value == 500 && "3rd element unchanged");
    ++it;
    hard_assert(it->value == 150 && "4th element unchanged");
    ++it;
    hard_assert(it->value == 100 && "5th element unchanged");
    hard_assert(it->generation == state.first_gen && "5th element generation unchanged");
    
    printf("  test_list_splice_single pass.\n\r");
}

__neverinline void test_list_verify_no_moves(test_list_state_s& state) {
    // Test 10: Verify final state - no element should have been copied or moved
    // during list operations (only during initial insertions)
    auto it = state.list.begin();
    int count = 0;
    while (it != state.list.end()) {
        hard_assert(!it->moved && "No element should be marked as moved");
        ++it;
        ++count;
    }
    hard_assert(count == 5 && "Final list should have 5 elements");
    
    printf("  test_list_verify_no_moves pass.\n\r");
}

__neverinline void test_list_splice_range(test_list_state_s& state) {
    // Test 11: splice_after with range (first, last)
    list_c<non_trivial_s, 0> list3;
    list3.push_front(non_trivial_s(1000));
    list3.push_front(non_trivial_s(2000));
    list3.push_front(non_trivial_s(3000));
    list3.push_front(non_trivial_s(4000));
    // list3: 4000 -> 3000 -> 2000 -> 1000

    // Store generations to verify no copy/move
    auto it3 = list3.begin();
    ++it3; // At 3000
    int gen_3000 = it3->generation;
    ++it3; // At 2000
    int gen_2000 = it3->generation;

    // Splice range (after 4000, before 1000) = [3000, 2000] into list after 400
    auto it = state.list.begin(); // At 400
    auto splice_first = list3.begin(); // At 4000
    auto splice_last = list3.begin();
    ++splice_last; ++splice_last; ++splice_last; // At 1000

    state.list.splice_after(it, list3, splice_first, splice_last);

    hard_assert(state.list.size() == 7 && "List size should be 7 after range splice");
    hard_assert(list3.size() == 2 && "List3 size should be 2 after range splice");

    // Verify list3 has 4000 -> 1000
    hard_assert(list3.front().value == 4000 && "List3 front should be 4000");
    it3 = list3.begin();
    ++it3;
    hard_assert(it3->value == 1000 && "List3 second should be 1000");

    // Verify spliced elements in list retained their properties
    // list should be: 400 -> 3000 -> 2000 -> 250 -> 500 -> 150 -> 100
    it = state.list.begin();
    hard_assert(it->value == 400 && "1st element should be 400");
    ++it;
    hard_assert(it->value == 3000 && "2nd element should be 3000 (spliced)");
    hard_assert(it->generation == gen_3000 && "Spliced 3000 generation unchanged");
    hard_assert(!it->moved && "Spliced 3000 not moved");
    ++it;
    hard_assert(it->value == 2000 && "3rd element should be 2000 (spliced)");
    hard_assert(it->generation == gen_2000 && "Spliced 2000 generation unchanged");
    hard_assert(!it->moved && "Spliced 2000 not moved");
    ++it;
    hard_assert(it->value == 250 && "4th element should be 250");
    ++it;
    hard_assert(it->value == 500 && "5th element should be 500");
    ++it;
    hard_assert(it->value == 150 && "6th element should be 150");
    ++it;
    hard_assert(it->value == 100 && "7th element should be 100");
    
    printf("  test_list_splice_range pass.\n\r");
}

__neverinline void test_list_splice_front_range(test_list_state_s& state) {
    // Test 12: splice_front with range
    list_c<non_trivial_s, 0> list4;
    list4.push_front(non_trivial_s(7000));
    list4.push_front(non_trivial_s(8000));
    list4.push_front(non_trivial_s(9000));
    // list4: 9000 -> 8000 -> 7000

    // Store generations
    auto it4 = list4.begin();
    int gen_9000 = it4->generation;
    ++it4;
    int gen_8000 = it4->generation;

    // Splice range [9000, 8000] to front of list
    auto splice_first = list4.before_begin();
    auto splice_last = list4.begin();
    ++splice_last; ++splice_last; // At 7000

    state.list.splice_front(list4, splice_first, splice_last);

    hard_assert(state.list.size() == 9 && "List size should be 9 after front range splice");
    hard_assert(list4.size() == 1 && "List4 size should be 1 after front range splice");
    hard_assert(list4.front().value == 7000 && "List4 should only have 7000");

    // Verify spliced elements at front
    auto it = state.list.begin();
    hard_assert(it->value == 9000 && "1st element should be 9000 (spliced to front)");
    hard_assert(it->generation == gen_9000 && "Spliced 9000 generation unchanged");
    hard_assert(!it->moved && "Spliced 9000 not moved");
    ++it;
    hard_assert(it->value == 8000 && "2nd element should be 8000 (spliced to front)");
    hard_assert(it->generation == gen_8000 && "Spliced 8000 generation unchanged");
    hard_assert(!it->moved && "Spliced 8000 not moved");
    ++it;
    hard_assert(it->value == 400 && "3rd element should be 400");

    // Test 13: Empty range splice should do nothing
    auto empty_first = list4.begin();
    auto empty_last = list4.begin();
    ++empty_last; // Points to end, so range is empty

    int size_before = state.list.size();
    state.list.splice_after(state.list.begin(), list4, empty_first, empty_last);
    hard_assert(state.list.size() == size_before && "Size unchanged after empty range splice");
    
    printf("  test_list_splice_front_range pass.\n\r");
}

void test_list() {
    printf("== Start: test_list\n\r");
    test_list_state_s state;
    test_list_basic_insert(state);
    test_list_insert_after(state);
    test_list_removal(state);
    test_list_splice_single(state);
    test_list_verify_no_moves(state);
    test_list_splice_range(state);
    test_list_splice_front_range(state);
    printf("test_list pass.\n\r");
}

void test_map() {
    printf("== Start: test_map\n\r");
    map_c<int, int, 6> map1({{6,0},{2,2}, {4,1}});
    hard_assert(map1.size() == 3);
    hard_assert(map1[2] == 2);
    hard_assert(map1[4] == 1);
    hard_assert(map1[6] == 0);

    map1.insert({2,10});
    map1.insert({7,7});
    map1.insert({1,1});
    map1.insert({3,3});
    // map1 now contains keys: 1, 2, 3, 4, 6, 7 (sorted)
    hard_assert(map1.size() == 6);

    // Test find() - positive cases
    auto it_first = map1.find(1);  // First element
    hard_assert(it_first != map1.end() && "Should find first element");
    hard_assert(it_first->first == 1 && it_first->second == 1);

    auto it_last = map1.find(7);  // Last element
    hard_assert(it_last != map1.end() && "Should find last element");
    hard_assert(it_last->first == 7 && it_last->second == 7);

    auto it_mid = map1.find(4);  // Middle element
    hard_assert(it_mid != map1.end() && "Should find middle element");
    hard_assert(it_mid->first == 4 && it_mid->second == 1);

    // Test find() - negative cases
    hard_assert(map1.find(0) == map1.end() && "Should not find key before first");
    hard_assert(map1.find(5) == map1.end() && "Should not find missing key in middle");
    hard_assert(map1.find(8) == map1.end() && "Should not find key after last");

    // Test push_back(), emplace_back(), pop_back() - for bulk loading sorted data
    {
        map_c<int, int, 8> map3;

        // First element must be inserted normally (push_back checks against back())
        map3.insert({10, 100});
        hard_assert(map3.size() == 1);
        hard_assert(map3.back().first == 10 && map3.back().second == 100);

        // push_back requires ascending keys (must be > back().first)
        map3.push_back({20, 200});
        hard_assert(map3.size() == 2);
        hard_assert(map3.back().first == 20 && map3.back().second == 200);

        // emplace_back also requires ascending keys
        map3.emplace_back(30, 300);
        hard_assert(map3.size() == 3);
        hard_assert(map3.back().first == 30 && map3.back().second == 300);

        // Verify all elements are in order and findable
        hard_assert(map3.find(10) != map3.end() && map3[10] == 100);
        hard_assert(map3.find(20) != map3.end() && map3[20] == 200);
        hard_assert(map3.find(30) != map3.end() && map3[30] == 300);

        // pop_back removes the last element
        map3.pop_back();
        hard_assert(map3.size() == 2);
        hard_assert(map3.find(30) == map3.end() && "Key 30 should be removed");
        hard_assert(map3.back().first == 20);

        map3.pop_back();
        hard_assert(map3.size() == 1);
        hard_assert(map3.back().first == 10);

        map3.pop_back();
        hard_assert(map3.size() == 0);
    }

    // Test with non_trivial_s to verify memory management
    {
        map_c<int, non_trivial_s, 8> map2;

        // Test insert with rvalue
        map2.insert({1, non_trivial_s(100)});
        hard_assert(map2.size() == 1);
        hard_assert(map2[1].value == 100);
        // Generation: 0 (temp) -> 1 (into pair) -> 2 (into map)
        hard_assert(map2[1].generation == 2 && "Copied twice: into pair, into map");

        // Test insert with lvalue
        pair_c<int, non_trivial_s> pair1(2, non_trivial_s(200));
        map2.insert(pair1);
        hard_assert(map2.size() == 2);
        hard_assert(map2[2].value == 200);

        // Test emplace (constructs pair in place)
        map2.emplace(3, non_trivial_s(300));
        hard_assert(map2.size() == 3);
        hard_assert(map2[3].value == 300);
        // Generation: 0 (temp) -> 1 (copied into pair via construct_at)
        hard_assert(map2[3].generation == 1 && "Copied once into pair");

        // Test insert replacing existing key
        const int destructors_before_replace = non_trivial_s::s_destructors;
        map2.insert({2, non_trivial_s(250)});
        hard_assert(map2.size() == 3 && "Size unchanged when replacing");
        hard_assert(map2[2].value == 250);
        hard_assert(non_trivial_s::s_destructors > destructors_before_replace && "Old value destroyed");

        // Test erase by iterator
        const int destructors_before_erase = non_trivial_s::s_destructors;
        auto it = map2.begin();
        hard_assert(it->first == 1 && "First key should be 1");
        map2.erase(it);
        hard_assert(map2.size() == 2);
        hard_assert(non_trivial_s::s_destructors > destructors_before_erase && "Erased value destroyed");

        // Verify remaining elements are sorted
        it = map2.begin();
        hard_assert(it->first == 2 && "First remaining key should be 2");
        hard_assert(it->second.value == 250);
        ++it;
        hard_assert(it->first == 3 && "Second remaining key should be 3");
        hard_assert(it->second.value == 300);

        // Test erase by key
        const int destructors_before_key_erase = non_trivial_s::s_destructors;
        map2.erase(3);
        hard_assert(map2.size() == 1);
        hard_assert(non_trivial_s::s_destructors > destructors_before_key_erase && "Erased value destroyed");

        // Verify remaining element
        hard_assert(map2.begin()->first == 2);
        hard_assert(map2.begin()->second.value == 250);

        // Test clear
        const int destructors_before_clear = non_trivial_s::s_destructors;
        map2.clear();
        hard_assert(map2.size() == 0);
        hard_assert(non_trivial_s::s_destructors > destructors_before_clear && "Cleared values destroyed");

        // Destructor count should increase when map2 goes out of scope
        // (but map2 is already empty, so no change expected from that)
    }

    printf("test_map pass.\n\r");
}
