//
//  display_list.hpp
//  toybox
//
//  Created by Fredrik on 2025-10-18.
//

#pragma once

#include "core/list.hpp"
#include "core/memory.hpp"

namespace toybox {

    class viewport_c;
    class palette_c;

    class display_item_c {
    public:
        enum class type_e : uint8_t {
            viewport, palette
        };
        using enum type_e;
        virtual type_e display_type() const __pure = 0;
        virtual ~display_item_c() = default;
    };

    enum {
        PRIMARY_VIEWPORT = -1,
        PRIMARY_PALETTE = -2
    };
    struct display_list_entry_s {
        int id;
        int row;
        shared_ptr_c<display_item_c> item_ptr;
        __forceinline display_item_c& item() const {
            assert(item_ptr != nullptr);
            return *item_ptr;
        }
        __forceinline shared_ptr_c<viewport_c>& viewport_ptr() {
            assert(item_ptr->display_type() == display_item_c::viewport && "Display item is not a viewport");
            return reinterpret_pointer_cast<viewport_c>(item_ptr);
        }
        __forceinline const shared_ptr_c<const viewport_c>& viewport_ptr() const {
            assert(item_ptr->display_type() == display_item_c::viewport && "Display item is not a viewport");
            return reinterpret_pointer_cast<const viewport_c>(item_ptr);
        }
        __forceinline viewport_c& viewport() {
            assert(item_ptr != nullptr);
            assert(item_ptr->display_type() == display_item_c::viewport && "Display item is not a viewport");
            return (viewport_c&)*item_ptr;
        }
        __forceinline const viewport_c& viewport() const {
            assert(item_ptr != nullptr);
            assert(item_ptr->display_type() == display_item_c::viewport && "Display item is not a viewport");
            return (viewport_c&)*item_ptr;
        }
        __forceinline shared_ptr_c<palette_c>& palette_ptr() {
            assert(item_ptr->display_type() == display_item_c::palette && "Display item is not a palette");
            return reinterpret_pointer_cast<palette_c>(item_ptr);
        }
        __forceinline const shared_ptr_c<const palette_c>& palette_ptr() const {
            assert(item_ptr->display_type() == display_item_c::palette && "Display item is not a palette");
            return reinterpret_pointer_cast<const palette_c>(item_ptr);
        }
        __forceinline palette_c& palette() {
            assert(item_ptr != nullptr);
            assert(item_ptr->display_type() == display_item_c::palette && "Display item is not a palette");
            return (palette_c&)*item_ptr;
        }
        __forceinline const palette_c& palette() const {
            assert(item_ptr != nullptr);
            assert(item_ptr->display_type() == display_item_c::palette && "Display item is not a palette");
            return (palette_c&)*item_ptr;
        }
        __forceinline bool operator<(const display_list_entry_s& rhs) const {
            return row < rhs.row;
        }
    };
    
    class display_list_c : public list_c<display_list_entry_s> {
    public:
        ~display_list_c() = default;
        const_iterator insert_sorted(const_reference value) {
            auto pos = iterator_before(value.row);
            return insert_after(pos, value);
        }
        template<class ...Args>
        iterator emplace_sorted(int id, int row, Args&& ...args) {
            auto pos = iterator_before(row);
            return emplace_after(pos, id, row, forward<Args>(args)...);
        }

        __forceinline display_list_entry_s& get(int id) const {
            return *get_if(id);
        }

        display_list_entry_s* get_if(int id) const {
            for (auto& item : *this) {
                if (item.id == id) return const_cast<display_list_entry_s*>(&item);
            }
            return nullptr;
        }

    private:
        const_iterator iterator_before(int row) const {
            auto iter = before_begin();
            while (iter._node->next) {
                if (iter._node->next->value.row >= row) {
                    break;
                }
                ++iter;
            }
            return iter;
        }
        
    };
    
}
