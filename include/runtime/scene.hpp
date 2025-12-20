//
//  scene.hpp
//  toybox
//
//  Created by Fredrik on 2024-03-01.
//

#pragma once

#include "machine/timer.hpp"
#include "machine/input.hpp"
#include "media/viewport.hpp"
#include "media/display_list.hpp"
#include "core/vector.hpp"

namespace toybox {
    
#define DEBUG_CPU_RUN_TRANSITION 0x100
#define DEBUG_CPU_TOP_SCENE_TICK 0x010
#define DEBUG_CPU_PHYS_RESTORE 0x004
#define DEBUG_CPU_DONE 0x000
    
    using namespace toybox;
    
    class scene_manager_c;
        
    /**
     A `scene_c` is an abstraction for managing one screen of content.
     A scene is for example the menu, one level, or the hi-score table.
     */
    class scene_c : public nocopy_c {
    public:
        /**
         The `configuration_s` defines how to display and configures a `scene_c.`
         */
        struct configuration_s {
            const size_s viewport_size;
            const shared_ptr_c<palette_c> palette; // May be empty
            const int buffer_count;
            const bool use_clear;
        };
        scene_c();
        virtual ~scene_c() {};
        
        virtual configuration_s& configuration() const;
        static configuration_s default_configuration;
        
        virtual void will_appear(bool obscured) {};
        virtual void will_disappear(bool obscured) {};
        
        virtual void update(display_list_c& display_list, int ticks) {};

    protected:
        scene_manager_c& manager;
    };
    
    /**
     A `transition_c` manages the visual transition from one scene to another.
     */
    class transition_c : public nocopy_c {
        friend class scene_manager_c;
    public:
        enum class update_state_e : uint8_t {
            repeat, swap, done
        };
        using enum update_state_e;

        transition_c();
        virtual ~transition_c() {}
        
        virtual void will_begin(const scene_c* from, scene_c* to) = 0;
        virtual update_state_e update(display_list_c& display_list, int ticks) = 0;

        static transition_c* create(canvas_c::stencil_e dither);
        static transition_c* create(canvas_c::stencil_e dither, uint8_t through);
        static transition_c* create(color_c through);
    
    protected:
        void to_will_appear(scene_c* to);
        scene_manager_c& manager;
        bool _obscured;
    };
        
    /**
     The `scene_manager_c` handles a stack of scenes and display lists.
     The top-most scene is the active scene currently displayed.
     Display lists include front (being presented), back (being drawn), and
     optionally clear (used for fast restoration).
     As the top-most scene changes the manager creates a transition to handle
     the visual transition.
     */
    class scene_manager_c final : public nocopy_c {
        friend class transition_c;
    public:
        enum class display_list_e : int8_t {
            clear = -1, front, back
        };
        using enum display_list_e;
        static scene_manager_c& shared();

        void run(unique_ptr_c<scene_c> rootscene, unique_ptr_c<transition_c> transition = nullptr);

        __forceinline scene_c& top_scene() const {
            return *_scene_stack.back();
        };
        void push(unique_ptr_c<scene_c> scene, unique_ptr_c<transition_c> transition = nullptr);
        void pop(unique_ptr_c<transition_c> transition = nullptr, int count = 1);
        void replace(unique_ptr_c<scene_c> scene, unique_ptr_c<transition_c> transition = nullptr);

        timer_c& vbl;
        timer_c& clock;

        display_list_c& display_list(display_list_e id);
        int display_list_count() const { return _display_lists.size(); }
        
    private:
        scene_manager_c();
        ~scene_manager_c() = default;

        unique_ptr_c<transition_c> _transition;
        vector_c<unique_ptr_c<scene_c>, 8> _scene_stack;
        vector_c<unique_ptr_c<scene_c>, 8> _deletion_scenes;

        void configure_display_lists(const scene_c::configuration_s& configuration);
        
        void swap_display_lists();

        inline void update_clear();
        inline void update_scene(scene_c& scene, int32_t ticks);

        __forceinline void enqueue_top_scene_for_delete() {
            _deletion_scenes.emplace_back(move(_scene_stack.back()));
            _scene_stack.pop_back();
        }
        inline void begin_transition(unique_ptr_c<transition_c> transition, const scene_c* from, scene_c* to, bool obscured);
        inline transition_c::update_state_e update_transition(int32_t ticks);
        inline void end_transition();

        shared_ptr_c<display_list_c> _clear_display_list;
        vector_c<shared_ptr_c<display_list_c>, 4> _display_lists;
        int _active_display_list;
        const scene_c::configuration_s *_configuration;  // Always valid, never null
    };
    
}
