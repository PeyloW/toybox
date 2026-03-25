//
//  test_memory.cpp
//  toybox - tests
//
//  Created by Fredrik on 2025-11-22.
//

#include "shared.hpp"

#include "core/memory.hpp"

class non_trivial_subclass_s : public non_trivial_s {
public:
    non_trivial_subclass_s() : non_trivial_s() {}
    explicit non_trivial_subclass_s(int v) : non_trivial_s(v) {}
    
    virtual ~non_trivial_subclass_s() {
        ++s_subclass_destructors;
    }
    inline static int s_subclass_destructors = 0;
};

__neverinline void test_shared_ptr() {
    printf("== Start: test_shared_ptr\n\r");
    
    // Test default construction
    {
        shared_ptr_c<int> ptr;
        hard_assert(!ptr && "Default shared_ptr null");
        hard_assert(ptr.use_count() == 0 && "Default shared_ptr use_count 0");
    }
    
    // Test construction with pointer
    {
        shared_ptr_c<int> ptr(new int(42));
        hard_assert(ptr && "Shared_ptr not null");
        hard_assert(*ptr == 42 && "Shared_ptr should point to correct value");
        hard_assert(ptr.use_count() == 1 && "Single shared_ptr use_count 1");
    }
    
    // Test copy construction
    {
        shared_ptr_c<int> ptr1(new int(100));
        hard_assert(ptr1.use_count() == 1 && "Initial shared_ptr use_count 1");
        
        shared_ptr_c<int> ptr2(ptr1);
        hard_assert(ptr2 && "Copied shared_ptr should not be null");
        hard_assert(*ptr2 == 100 && "Copied shared_ptr points to same value");
        hard_assert(ptr1.use_count() == 2 && "Original shared_ptr use_count 2");
        hard_assert(ptr2.use_count() == 2 && "Copied shared_ptr use_count 2");
        hard_assert(ptr1.get() == ptr2.get() && "Both shared_ptrs point to same obj");
    }
    
    // Test move construction
    {
        shared_ptr_c<int> ptr1(new int(200));
        int* raw = ptr1.get();
        hard_assert(ptr1.use_count() == 1 && "Initial use_count should be 1");
        
        shared_ptr_c<int> ptr2(move(ptr1));
        hard_assert(ptr1.get() == nullptr && "Move-from shared_ptr should be null");
        hard_assert(ptr1.use_count() == 0 && "Move-from shared_ptr use_count 0");
        hard_assert(ptr2 && "Moved-to shared_ptr should not be null");
        hard_assert(*ptr2 == 200 && "Moved-to shared_ptr correct value");
        hard_assert(ptr2.get() == raw && "Moved-to shared_ptr same pointer");
        hard_assert(ptr2.use_count() == 1 && "Moved-to shared_ptr use_count 1");
    }
    
    // Test copy assignment
    {
        shared_ptr_c<int> ptr1(new int(300));
        {
            shared_ptr_c<int> ptr2(new int(400));
            hard_assert(ptr1.use_count() == 1 && "ptr1 use_count 1 before assign");
            hard_assert(ptr2.use_count() == 1 && "ptr2 use_count 1 before assign");
            ptr2 = ptr1;
            hard_assert(*ptr2 == 300 && "Assigned shared_ptr correct value");
            hard_assert(ptr1.use_count() == 2 && "ptr1 use_count 2 after assign");
            hard_assert(ptr2.use_count() == 2 && "ptr2 use_count 2 after assign");
        }
        hard_assert(ptr1.use_count() == 1 && "shared_ptrs should have use_count 1");
    }

    // Test assignment (move semantics similar to copy for shared_ptr)
    {
        shared_ptr_c<int> ptr1(new int(500));
        shared_ptr_c<int> ptr2(new int(600));
        int* raw = ptr1.get();

        hard_assert(ptr2.use_count() == 1 && "Use count is 1");
        ptr2 = move(ptr1);
        hard_assert(ptr2.use_count() == 1 && "Use count is still 1");
        hard_assert(ptr1 == nullptr && "Moved-from shared_ptr null");
        hard_assert(ptr2.get() == raw && "Assigned shared_ptr same pointer");
        hard_assert(*ptr2 == 500 && "Assigned shared_ptr correct value");
        hard_assert(ptr2.get() == raw && "Assigned ptr still same pointer");
    }

    // Test reset
    {
        shared_ptr_c<int> ptr(new int(700));
        hard_assert(ptr.use_count() == 1 && "Initial use_count should be 1");

        ptr.reset();
        hard_assert(!ptr && "Reset shared_ptr should be null");
        hard_assert(ptr.use_count() == 0 && "Reset shared_ptr use_count 0");

        ptr.reset(new int(800));
        hard_assert(ptr && "Reset shared_ptr with value not null");
        hard_assert(*ptr == 800 && "Reset shared_ptr correct value");
        hard_assert(ptr.use_count() == 1 && "Reset shared_ptr use_count 1");
    }

    // Test with non-trivial type
    {
        shared_ptr_c<non_trivial_s> ptr1(new non_trivial_s(999));
        hard_assert(ptr1->value == 999 && "Non-trivial shared_ptr value");
        hard_assert(ptr1->generation == 0 && "Non-trivial obj generation 0");
        hard_assert(!ptr1->moved && "Non-trivial obj not moved");
        hard_assert(ptr1.use_count() == 1 && "Initial use_count should be 1");

        shared_ptr_c<non_trivial_s> ptr2 = ptr1;
        hard_assert(ptr2->value == 999 && "Copied non-trivial ptr value");
        hard_assert(ptr2->generation == 0 && "Non-trivial obj still generation 0");
        hard_assert(!ptr2->moved && "Copied non-trivial obj not moved");
        hard_assert(ptr1.use_count() == 2 && "Use_count should be 2 after copy");
        hard_assert(ptr2.use_count() == 2 && "Use_count should be 2 after copy");
        hard_assert(ptr1.get() == ptr2.get() && "Both ptrs point to same object");
    }
    
    // Test with non-trivial casts
    {
        non_trivial_s::s_destructors = 0;
        non_trivial_subclass_s::s_subclass_destructors = 0;
        {
            shared_ptr_c<non_trivial_subclass_s> ptr1(new non_trivial_subclass_s(42));
            shared_ptr_c<non_trivial_s> ptr2 = reinterpret_pointer_cast<non_trivial_s>(ptr1);
            hard_assert(ptr1.use_count() == 2 && "ptr1 use_count 2 after cast");
            hard_assert(ptr2.use_count() == 2 && "ptr2 use_count 2 after cast");
            hard_assert(ptr1->value == ptr2->value && "Same value after static pointer cast.");
            hard_assert(ptr1.get() == ptr2.get() && "Same raw pointer after static cast.");
        }
        hard_assert(non_trivial_s::s_destructors == 1 && "Destructor only called once");
        hard_assert(non_trivial_subclass_s::s_subclass_destructors == 1 && "Subclass destructor was called!");
        {
            non_trivial_s* ptr = new non_trivial_subclass_s(42);
            shared_ptr_c<non_trivial_s> ptr1(ptr);
            shared_ptr_c<non_trivial_subclass_s> ptr2 = reinterpret_pointer_cast<non_trivial_subclass_s>(ptr1);
            hard_assert(ptr1.use_count() == 2 && "ptr1 use_count 2 after cast");
            hard_assert(ptr2.use_count() == 2 && "ptr2 use_count 2 after cast");
            hard_assert(ptr1->value == ptr2->value && "Same value after static pointer cast.");
            hard_assert(ptr1.get() == ptr2.get() && "Same raw pointer after static cast.");
        }
        hard_assert(non_trivial_s::s_destructors == 2 && "Destructor called twice");
        hard_assert(non_trivial_subclass_s::s_subclass_destructors == 2 && "Subclass destructor was not called!");
    }

    printf("== End: test_shared_ptr\n\r");
}
