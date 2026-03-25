//
//  test_lifetime.cpp
//  toybox - tests
//
//  Created by Fredrik on 2025-11-09.
//

#include "shared.hpp"

#include "core/array.hpp"
#include "core/vector.hpp"
#include "core/list.hpp"

struct lifetime_test_state_s {
    // Empty for now, allows future state sharing if needed
};

__neverinline void test_lifetime_direct_semantics() {
    // Test direct copy semantics
    non_trivial_s obj1(42);
    hard_assert(obj1.value == 42 && "Initial value should be 42");
    hard_assert(obj1.generation == 0 && "Initial generation should be 0");
    hard_assert(!obj1.moved && "Initial object should not be moved");

    non_trivial_s obj2(obj1);
    hard_assert(obj2.value == 42 && "Copied value should be 42");
    hard_assert(obj2.generation == 1 && "Copied generation should be 1");
    hard_assert(!obj2.moved && "Copied object not moved");
    hard_assert(!obj1.moved && "Original not moved after copy");

    non_trivial_s obj3(100);
    obj3 = obj2;
    hard_assert(obj3.value == 42 && "Assigned value should be 42");
    hard_assert(obj3.generation == 2 && "Assigned generation should be 2");
    hard_assert(!obj3.moved && "Assigned object not moved");

    // Test direct move semantics
    non_trivial_s obj4(move(obj3));
    hard_assert(obj4.value == 42 && "Moved value should be 42");
    hard_assert(obj4.generation == 2 && "Moved generation should remain 2");
    hard_assert(!obj4.moved && "Move dest not moved");
    hard_assert(obj3.moved && "Source object is moved");

    non_trivial_s obj5(200);
    obj5 = move(obj4);
    hard_assert(obj5.value == 42 && "Move-assigned value should be 42");
    hard_assert(obj5.generation == 2 && "Move-assigned generation should remain 2");
    hard_assert(!obj5.moved && "Move dest not moved");
    hard_assert(obj4.moved && "Source object is moved");

    printf("  test_lifetime_direct_semantics pass.\n\r");
}

__neverinline void test_lifetime_static_vector_insert(lifetime_test_state_s& state) {
    // Test with static vector_c
    vector_c<non_trivial_s, 10> static_vec;

    // 1. Push back with rvalue
    static_vec.push_back(non_trivial_s(10));
    hard_assert(static_vec.size() == 1 && "Static vector size should be 1");
    hard_assert(static_vec[0].value == 10 && "Back element value should be 10");
    hard_assert(static_vec[0].generation == 1 && "Back element gen is 1 (from temp)");
    hard_assert(!static_vec[0].moved && "Back element not moved");

    // 2. Push back with lvalue (copy)
    non_trivial_s lvalue_vec1(15);
    static_vec.push_back(lvalue_vec1);
    hard_assert(static_vec.size() == 2 && "Static vector size should be 2");
    hard_assert(static_vec[1].value == 15 && "Back element value should be 15");
    hard_assert(static_vec[1].generation == 1 && "Back element gen is 1 (from lvalue)");
    hard_assert(!static_vec[1].moved && "Back element should not be moved");
    hard_assert(!lvalue_vec1.moved && "Original lvalue should not be moved");

    // 3. Emplace back (direct construction)
    static_vec.emplace_back(17);
    hard_assert(static_vec.size() == 3 && "Static vector size should be 3");
    hard_assert(static_vec[2].value == 17 && "Emplaced element value should be 17");
    hard_assert(static_vec[2].generation == 0 && "Emplaced gen is 0 (in-place)");
    hard_assert(!static_vec[2].moved && "Emplaced element should not be moved");

    // 4. Insert front (existing elements moved during shift)
    static_vec.insert(static_vec.begin(), non_trivial_s(5));
    hard_assert(static_vec.size() == 4 && "Static vector size should be 4");
    hard_assert(static_vec[0].value == 5 && "Front element should be 5");
    hard_assert(static_vec[0].generation == 1 && "Front element gen is 1 (from temp)");
    hard_assert(!static_vec[0].moved && "Front element should not be moved");
    hard_assert(static_vec[1].value == 10 && "Element should be 10");
    hard_assert(static_vec[1].generation == 1 && "Element gen remains 1 (moved)");
    hard_assert(!static_vec[1].moved && "Move dest not moved");
    hard_assert(static_vec[2].value == 15 && "Element should be 15");
    hard_assert(static_vec[3].value == 17 && "Element should be 17");

    // 5. Insert middle with lvalue
    non_trivial_s lvalue_vec2(12);
    static_vec.insert(static_vec.begin() + 2, lvalue_vec2);
    hard_assert(static_vec.size() == 5 && "Static vector size should be 5");
    hard_assert(static_vec[2].value == 12 && "Inserted element value should be 12");
    hard_assert(static_vec[2].generation == 1 && "Inserted element gen is 1 (copy)");
    hard_assert(!static_vec[2].moved && "Inserted element should not be moved");
    hard_assert(!lvalue_vec2.moved && "Original lvalue should not be moved");

    // 6. Emplace middle (direct construction)
    static_vec.emplace(3, 13);
    hard_assert(static_vec.size() == 6 && "Static vector size should be 6");
    hard_assert(static_vec[3].value == 13 && "Emplaced element value should be 13");
    hard_assert(static_vec[3].generation == 0 && "Emplaced gen is 0 (in-place)");
    hard_assert(!static_vec[3].moved && "Emplaced element should not be moved");

    printf("  test_lifetime_static_vector_insert pass.\n\r");
}

__neverinline void test_lifetime_static_vector_remove(lifetime_test_state_s& state) {
    // Recreate the vector state from previous test
    vector_c<non_trivial_s, 10> static_vec;
    static_vec.push_back(non_trivial_s(10));
    non_trivial_s lvalue_vec1(15);
    static_vec.push_back(lvalue_vec1);
    static_vec.emplace_back(17);
    static_vec.insert(static_vec.begin(), non_trivial_s(5));
    non_trivial_s lvalue_vec2(12);
    static_vec.insert(static_vec.begin() + 2, lvalue_vec2);
    static_vec.emplace(3, 13);

    // 7. Push back again
    static_vec.push_back(non_trivial_s(20));
    hard_assert(static_vec.size() == 7 && "Static vector size should be 7");
    hard_assert(static_vec[6].value == 20 && "New back element value should be 20");
    hard_assert(static_vec[6].generation == 1 && "New back element gen is 1 (from temp)");
    hard_assert(!static_vec[6].moved && "New back element not moved");

    // 8. Pop back
    static_vec.pop_back();
    hard_assert(static_vec.size() == 6 && "Static vector size is 6 after pop");
    hard_assert(static_vec[5].value == 17 && "New back element should be 17");
    hard_assert(static_vec[5].generation == 0 && "Back element generation should be 0");
    hard_assert(!static_vec[5].moved && "Back element should not be moved");

    // 9. Erase front (elements moved during shift)
    static_vec.erase(static_vec.begin());
    hard_assert(static_vec.size() == 5 && "Static vector size is 5 after erase");
    hard_assert(static_vec[0].value == 10 && "First element is 10 after erase");
    hard_assert(static_vec[0].generation == 1 && "First element gen remains 1 (moved)");
    hard_assert(!static_vec[0].moved && "First element move dest not moved");
    hard_assert(static_vec[1].value == 12 && "Element should be 12");
    hard_assert(static_vec[2].value == 13 && "Element should be 13");
    hard_assert(static_vec[3].value == 15 && "Element should be 15");
    hard_assert(static_vec[4].value == 17 && "Element should be 17");

    // 10. Erase middle
    static_vec.erase(static_vec.begin() + 2);
    hard_assert(static_vec.size() == 4 && "Static vector size is 4 after erase");
    hard_assert(static_vec[0].value == 10 && "Element should be 10");
    hard_assert(static_vec[1].value == 12 && "Element should be 12");
    hard_assert(static_vec[2].value == 15 && "Element should be 15 (13 erased)");
    hard_assert(static_vec[2].generation == 1 && "Element generation should be 1");
    hard_assert(!static_vec[2].moved && "Element should not be moved");
    hard_assert(static_vec[3].value == 17 && "Element should be 17");

    printf("  test_lifetime_static_vector_remove pass.\n\r");
}

__neverinline void test_lifetime_dynamic_vector_insert(lifetime_test_state_s& state) {
    // Test with dynamic vector_c
    vector_c<non_trivial_s, 0> dynamic_vec;
    hard_assert(dynamic_vec.size() == 0 && "Dynamic vector initial size is 0");

    // 1. Push back with rvalue
    dynamic_vec.push_back(non_trivial_s(100));
    hard_assert(dynamic_vec.size() == 1 && "Dynamic vector size should be 1");
    hard_assert(dynamic_vec[0].value == 100 && "Back element value should be 100");
    hard_assert(dynamic_vec[0].generation == 1 && "Back element gen is 1 (from temp)");
    hard_assert(!dynamic_vec[0].moved && "Back element not moved");

    // 2. Push back with lvalue (copy)
    non_trivial_s lvalue_dyn1(150);
    dynamic_vec.push_back(lvalue_dyn1);
    hard_assert(dynamic_vec.size() == 2 && "Dynamic vector size should be 2");
    hard_assert(dynamic_vec[1].value == 150 && "Back element value should be 150");
    hard_assert(dynamic_vec[1].generation == 1 && "Back element gen is 1 (from lvalue)");
    hard_assert(!dynamic_vec[1].moved && "Back element should not be moved");
    hard_assert(!lvalue_dyn1.moved && "Original lvalue should not be moved");

    // 3. Emplace back (direct construction)
    dynamic_vec.emplace_back(175);
    hard_assert(dynamic_vec.size() == 3 && "Dynamic vector size should be 3");
    hard_assert(dynamic_vec[2].value == 175 && "Emplaced element value should be 175");
    hard_assert(dynamic_vec[2].generation == 0 && "Emplaced gen is 0 (in-place)");
    hard_assert(!dynamic_vec[2].moved && "Emplaced element should not be moved");

    // 4. Insert front (existing elements moved during shift)
    dynamic_vec.insert(dynamic_vec.begin(), non_trivial_s(50));
    hard_assert(dynamic_vec.size() == 4 && "Dynamic vector size should be 4");
    hard_assert(dynamic_vec[0].value == 50 && "Front element should be 50");
    hard_assert(dynamic_vec[0].generation == 1 && "Front element gen is 1 (from temp)");
    hard_assert(!dynamic_vec[0].moved && "Front element should not be moved");
    hard_assert(dynamic_vec[1].value == 100 && "Element should be 100");
    hard_assert(dynamic_vec[1].generation == 1 && "Element gen remains 1 (moved)");
    hard_assert(!dynamic_vec[1].moved && "Move dest not moved");
    hard_assert(dynamic_vec[2].value == 150 && "Element should be 150");
    hard_assert(dynamic_vec[3].value == 175 && "Element should be 175");

    // 5. Insert middle with lvalue
    non_trivial_s lvalue_dyn2(125);
    dynamic_vec.insert(dynamic_vec.begin() + 2, lvalue_dyn2);
    hard_assert(dynamic_vec.size() == 5 && "Dynamic vector size should be 5");
    hard_assert(dynamic_vec[2].value == 125 && "Inserted element value should be 125");
    hard_assert(dynamic_vec[2].generation == 1 && "Inserted element gen is 1 (copy)");
    hard_assert(!dynamic_vec[2].moved && "Inserted element should not be moved");
    hard_assert(!lvalue_dyn2.moved && "Original lvalue should not be moved");

    // 6. Emplace middle (direct construction)
    dynamic_vec.emplace(3, 137);
    hard_assert(dynamic_vec.size() == 6 && "Dynamic vector size should be 6");
    hard_assert(dynamic_vec[3].value == 137 && "Emplaced element value should be 137");
    hard_assert(dynamic_vec[3].generation == 0 && "Emplaced gen is 0 (in-place)");
    hard_assert(!dynamic_vec[3].moved && "Emplaced element should not be moved");

    printf("  test_lifetime_dynamic_vector_insert pass.\n\r");
}

__neverinline void test_lifetime_dynamic_vector_remove(lifetime_test_state_s& state) {
    // Recreate the vector state from previous test
    vector_c<non_trivial_s, 0> dynamic_vec;
    dynamic_vec.push_back(non_trivial_s(100));
    non_trivial_s lvalue_dyn1(150);
    dynamic_vec.push_back(lvalue_dyn1);
    dynamic_vec.emplace_back(175);
    dynamic_vec.insert(dynamic_vec.begin(), non_trivial_s(50));
    non_trivial_s lvalue_dyn2(125);
    dynamic_vec.insert(dynamic_vec.begin() + 2, lvalue_dyn2);
    dynamic_vec.emplace(3, 137);

    // 7. Push back again
    dynamic_vec.push_back(non_trivial_s(200));
    hard_assert(dynamic_vec.size() == 7 && "Dynamic vector size should be 7");
    hard_assert(dynamic_vec[6].value == 200 && "New back element value should be 200");
    hard_assert(dynamic_vec[6].generation == 1 && "New back element gen is 1 (from temp)");
    hard_assert(!dynamic_vec[6].moved && "New back element not moved");

    // 8. Pop back
    dynamic_vec.pop_back();
    hard_assert(dynamic_vec.size() == 6 && "Dynamic vector size is 6 after pop");
    hard_assert(dynamic_vec[5].value == 175 && "New back element should be 175");
    hard_assert(dynamic_vec[5].generation == 0 && "Back element generation should be 0");
    hard_assert(!dynamic_vec[5].moved && "Back element should not be moved");

    // 9. Erase front (elements moved during shift)
    dynamic_vec.erase(dynamic_vec.begin());
    hard_assert(dynamic_vec.size() == 5 && "Dynamic vector size is 5 after erase");
    hard_assert(dynamic_vec[0].value == 100 && "First element is 100 after erase");
    hard_assert(dynamic_vec[0].generation == 1 && "First element gen remains 1 (moved)");
    hard_assert(!dynamic_vec[0].moved && "First element move dest not moved");
    hard_assert(dynamic_vec[1].value == 125 && "Element should be 125");
    hard_assert(dynamic_vec[2].value == 137 && "Element should be 137");
    hard_assert(dynamic_vec[3].value == 150 && "Element should be 150");
    hard_assert(dynamic_vec[4].value == 175 && "Element should be 175");

    // 10. Erase middle
    dynamic_vec.erase(dynamic_vec.begin() + 2);
    hard_assert(dynamic_vec.size() == 4 && "Dynamic vector size is 4 after erase");
    hard_assert(dynamic_vec[0].value == 100 && "Element should be 100");
    hard_assert(dynamic_vec[1].value == 125 && "Element should be 125");
    hard_assert(dynamic_vec[2].value == 150 && "Element should be 150 (137 erased)");
    hard_assert(dynamic_vec[2].generation == 1 && "Element generation should be 1");
    hard_assert(!dynamic_vec[2].moved && "Element should not be moved");
    hard_assert(dynamic_vec[3].value == 175 && "Element should be 175");

    printf("  test_lifetime_dynamic_vector_remove pass.\n\r");
}

__neverinline void test_lifetime_list_insert(lifetime_test_state_s& state) {
    // Test with list_c
    list_c<non_trivial_s, 10> list;

    // 1. Test push_front with rvalue (move)
    list.push_front(non_trivial_s(1000));
    hard_assert(list.size() == 1 && "List size should be 1");
    hard_assert(list.front().value == 1000 && "List front value should be 1000");
    hard_assert(list.front().generation == 1 && "Front gen is 1 (from temp)");
    hard_assert(!list.front().moved && "Front not moved");

    // 2. Test push_front with lvalue (copy)
    non_trivial_s lvalue1(2000);
    list.push_front(lvalue1);
    hard_assert(list.size() == 2 && "List size should be 2");
    hard_assert(list.front().value == 2000 && "List front value should be 2000");
    hard_assert(list.front().generation == 1 && "Front gen is 1 (from lvalue)");
    hard_assert(!list.front().moved && "Front not moved");
    hard_assert(!lvalue1.moved && "Original not moved after copy");

    // 3. Test emplace_front (direct construction)
    list.emplace_front(3000);
    hard_assert(list.size() == 3 && "List size should be 3");
    hard_assert(list.front().value == 3000 && "List front value should be 3000");
    hard_assert(list.front().generation == 0 && "Front gen is 0 (in-place)");
    hard_assert(!list.front().moved && "Front not moved");

    // 4. Test insert_after with lvalue (copy)
    non_trivial_s lvalue2(4000);
    auto it = list.begin();
    list.insert_after(it, lvalue2);
    hard_assert(list.size() == 4 && "List size should be 4");
    ++it;
    hard_assert(it->value == 4000 && "Inserted element value should be 4000");
    hard_assert(it->generation == 1 && "Inserted element is copied (gen 1)");
    hard_assert(!it->moved && "Inserted element not moved");
    hard_assert(!lvalue2.moved && "Original not moved after copy");

    // 5. Test insert_after with rvalue (move)
    it = list.begin();
    ++it; // Move to second position
    list.insert_after(it, non_trivial_s(5000));
    hard_assert(list.size() == 5 && "List size should be 5");
    ++it;
    hard_assert(it->value == 5000 && "Inserted element value should be 5000");
    hard_assert(it->generation == 1 && "Inserted gen is 1 (from temp)");
    hard_assert(!it->moved && "Inserted element not moved");

    // 6. Test emplace_after (direct construction)
    it = list.begin();
    ++it;
    ++it; // Move to third position
    list.emplace_after(it, 6000);
    hard_assert(list.size() == 6 && "List size should be 6");
    ++it;
    hard_assert(it->value == 6000 && "Emplaced element value should be 6000");
    hard_assert(it->generation == 0 && "Emplaced gen is 0 (in-place)");
    hard_assert(!it->moved && "Emplaced element not moved");

    // Verify current list order: 3000 -> 4000 -> 5000 -> 6000 -> 2000 -> 1000
    it = list.begin();
    hard_assert(it->value == 3000 && "1st element should be 3000");
    hard_assert(it->generation == 0 && "1st element generation should be 0");
    hard_assert(!it->moved && "1st element should not be moved");
    ++it;
    hard_assert(it->value == 4000 && "2nd element should be 4000");
    hard_assert(it->generation == 1 && "2nd element generation should be 1");
    hard_assert(!it->moved && "2nd element should not be moved");
    ++it;
    hard_assert(it->value == 5000 && "3rd element should be 5000");
    hard_assert(it->generation == 1 && "3rd element generation should be 1");
    hard_assert(!it->moved && "3rd element should not be moved");
    ++it;
    hard_assert(it->value == 6000 && "4th element should be 6000");
    hard_assert(it->generation == 0 && "4th element generation should be 0");
    hard_assert(!it->moved && "4th element should not be moved");
    ++it;
    hard_assert(it->value == 2000 && "5th element should be 2000");
    hard_assert(it->generation == 1 && "5th element generation should be 1");
    hard_assert(!it->moved && "5th element should not be moved");
    ++it;
    hard_assert(it->value == 1000 && "6th element should be 1000");
    hard_assert(it->generation == 1 && "6th element generation should be 1");
    hard_assert(!it->moved && "6th element should not be moved");

    printf("  test_lifetime_list_insert pass.\n\r");
}

__neverinline void test_lifetime_list_remove(lifetime_test_state_s& state) {
    // Recreate the list state from previous test
    list_c<non_trivial_s, 10> list;
    list.push_front(non_trivial_s(1000));
    non_trivial_s lvalue1(2000);
    list.push_front(lvalue1);
    list.emplace_front(3000);
    non_trivial_s lvalue2(4000);
    auto it = list.begin();
    list.insert_after(it, lvalue2);
    it = list.begin();
    ++it;
    list.insert_after(it, non_trivial_s(5000));
    it = list.begin();
    ++it;
    ++it;
    list.emplace_after(it, 6000);

    // 7. Test pop_front
    list.pop_front();
    hard_assert(list.size() == 5 && "List size should be 5 after pop_front");
    hard_assert(list.front().value == 4000 && "Front value is 4000 after pop");
    hard_assert(list.front().generation == 1 && "List front generation should be 1");
    hard_assert(!list.front().moved && "List front should not be marked as moved");

    // 8. Test erase_after
    it = list.begin();
    ++it; // Now at 5000
    list.erase_after(it);
    hard_assert(list.size() == 4 && "List size should be 4 after erase_after");
    // Verify remaining elements: 4000 -> 5000 -> 2000 -> 1000
    it = list.begin();
    hard_assert(it->value == 4000 && "1st element should be 4000");
    hard_assert(it->generation == 1 && "1st element generation should be 1");
    hard_assert(!it->moved && "1st element should not be moved");
    ++it;
    hard_assert(it->value == 5000 && "2nd element should be 5000");
    hard_assert(it->generation == 1 && "2nd element generation should be 1");
    hard_assert(!it->moved && "2nd element should not be moved");
    ++it;
    hard_assert(it->value == 2000 && "3rd element should be 2000 (6000 erased)");
    hard_assert(it->generation == 1 && "3rd element generation should be 1");
    hard_assert(!it->moved && "3rd element should not be moved");
    ++it;
    hard_assert(it->value == 1000 && "4th element should be 1000");
    hard_assert(it->generation == 1 && "4th element generation should be 1");
    hard_assert(!it->moved && "4th element should not be moved");

    printf("  test_lifetime_list_remove pass.\n\r");
}

__neverinline void test_lifetime_list_splice(lifetime_test_state_s& state) {
    // Recreate a simple list state: 4000 -> 2000 -> 1000
    list_c<non_trivial_s, 10> list;
    list.push_front(non_trivial_s(1000));
    non_trivial_s lvalue1(2000);
    list.push_front(lvalue1);
    non_trivial_s lvalue2(4000);
    list.push_front(lvalue2);
    // list: 4000 -> 2000 -> 1000 (size = 3)

    // 9. Test splice_after
    list_c<non_trivial_s, 10> list2;
    list2.push_front(non_trivial_s(7000));
    list2.push_front(non_trivial_s(8000));
    hard_assert(list2.size() == 2 && "List2 size should be 2");
    hard_assert(list2.front().value == 8000 && "List2 front should be 8000");
    hard_assert(list2.front().generation == 1 && "List2 front generation should be 1");
    hard_assert(!list2.front().moved && "List2 front should not be moved");

    // Splice first element of list2 (8000) into list after first element (4000)
    auto it = list.begin();
    list.splice_after(it, list2, list2.before_begin());
    hard_assert(list.size() == 4 && "List size should be 4 after splice");
    hard_assert(list2.size() == 1 && "List2 size should be 1 after splice");

    // Verify list: 4000 -> 8000 -> 2000 -> 1000
    it = list.begin();
    hard_assert(it->value == 4000 && "1st element should be 4000");
    ++it;
    hard_assert(it->value == 8000 && "2nd element should be 8000 (spliced)");
    hard_assert(it->generation == 1 && "Spliced element generation should be 1");
    hard_assert(!it->moved && "Spliced element should not be moved");
    ++it;
    hard_assert(it->value == 2000 && "3rd element should be 2000");
    ++it;
    hard_assert(it->value == 1000 && "4th element should be 1000");

    // Verify list2: 7000
    hard_assert(list2.front().value == 7000 && "List2 front should be 7000");
    hard_assert(list2.front().generation == 1 && "List2 front generation should be 1");
    hard_assert(!list2.front().moved && "List2 front should not be moved");

    printf("  test_lifetime_list_splice pass.\n\r");
}

void test_lifetime() {
    printf("== Start: test_lifetime\n\r");
    lifetime_test_state_s state;
    test_lifetime_direct_semantics();
    test_lifetime_static_vector_insert(state);
    test_lifetime_static_vector_remove(state);
    test_lifetime_dynamic_vector_insert(state);
    test_lifetime_dynamic_vector_remove(state);
    test_lifetime_list_insert(state);
    test_lifetime_list_remove(state);
    test_lifetime_list_splice(state);
    printf("test_lifetime pass.\n\r");
}
