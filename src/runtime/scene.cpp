//
//  scene.cpp
//  toybox
//
//  Created by Fredrik on 2024-03-01.
//

#include "runtime/scene.hpp"
#include "machine/machine.hpp"
#include "core/algorithm.hpp"

using namespace toybox;

scene_c::scene_c() : manager(scene_manager_c::shared()) {}
scene_c::configuration_s scene_c::default_configuration = { viewport_c::min_size, nullptr, 2, true };
scene_c::configuration_s& scene_c::configuration() const {
    return default_configuration;
}

// TODO: Transitions needs to be completely rewritten for display_lists.
// NOTE: Cross fade through a color may be the only option worth it for now.
transition_c::transition_c() : manager(scene_manager_c::shared()) {}

class no_transition_c final : public transition_c {
public:
    no_transition_c() : transition_c() {
        _full_restores_left = 2;
    }
    virtual void will_begin(const scene_c* from, const scene_c* to) override {
        auto to_pal = to->configuration().palette.get();
        if (to_pal) {
            // Copy the configuration palette to the primary palette of each display list
            for (int i = 0; i < 2; ++i) {
                auto& palette = manager.display_list((scene_manager_c::display_list_e)i).get(PRIMARY_PALETTE).palette();
                copy(to_pal->begin(), to_pal->end(), palette.begin());
            }
        }
    };
    virtual bool tick(int ticks) override {
        return --_full_restores_left <= 0;
    }
private:
    int _full_restores_left;
};

scene_manager_c::scene_manager_c() :
    vbl(timer_c::shared(timer_c::timer_e::vbl)),
    clock(timer_c::shared(timer_c::timer_e::clock))
{
    machine_c::shared();
    _active_display_list = 0;
    _transition = nullptr;
    configure_display_lists(scene_c::default_configuration);
    srand48(time(nullptr));
}

scene_manager_c& scene_manager_c::shared()
{
    static scene_manager_c s_shared;
    return s_shared;
}

#define DEBUG_NO_SET_SCREEN 0

void scene_manager_c::run(scene_c* rootscene, transition_c* transition) {
    push(rootscene, transition);

    vbl.reset_tick();
    int32_t previous_tick = vbl.tick();
    while (_scene_stack.size() > 0) {
        vbl.wait();
        int32_t tick = vbl.tick();
        int32_t ticks = tick - previous_tick;
        previous_tick = tick;
                
        if (_transition) {
            update_transition(ticks);
        } else {
            debug_cpu_color(DEBUG_CPU_TOP_SCENE_TICK);
            if (_scene_stack.size() > 0) {
                auto& scene = top_scene();
                auto& config = scene.configuration();
                // Merge dirty maps here!
                if (config.use_clear) {
                    auto& back_viewport = update_clear();
                    back_viewport.with_dirtymap(back_viewport.dirtymap(), [&] {
                        update_scene(scene, ticks);
                    });
                } else {
                    update_scene(scene, ticks);
                }
            }
            _deletion_scenes.clear();
            _deletion_display_lists.clear();
        }
        debug_cpu_color(DEBUG_CPU_DONE);
        timer_c::with_paused_timers([&] {
            display_list_c& back = display_list(display_list_e::back);
            //sort(back.begin(), back.end());
            machine_c::shared().set_active_display_list(&back);
            swap_display_lists();
        });
    }
}

void scene_manager_c::push(scene_c* scene, transition_c* transition) {
    scene_c* from = nullptr;
    if (_scene_stack.size() > 0) {
        from = &top_scene();
        from->will_disappear(true);
    }
    _scene_stack.push_back(scene);
    begin_transition(transition, from, scene, false);
}

void scene_manager_c::pop(transition_c* transition, int count) {
    scene_c* from = nullptr;
    while (count-- > 0) {
        from = &top_scene();
        from->will_disappear(false);
        enqueue_delete(from);
        _scene_stack.pop_back();
    }
    scene_c* to = nullptr;
    if (_scene_stack.size() > 0) {
        to = &top_scene();
    }
    if (to) {
        begin_transition(transition, from, to, true);
    }
}

void scene_manager_c::replace(scene_c* scene, transition_c* transition) {
    scene_c* from = &top_scene();
    from->will_disappear(false);
    enqueue_delete(from);
    _scene_stack.back() = scene;
    begin_transition(transition, from, scene, false);
}

display_list_c& scene_manager_c::display_list(display_list_e id) const {
    if (id == display_list_e::clear) {
        return *_clear_display_list;
    } else {
        int idx = ((int)id + _active_display_list);
        if (idx >=  _configuration->buffer_count) {
            idx -= _configuration->buffer_count;
        }
        return (display_list_c&)*_display_lists[idx];
    }
}

void scene_manager_c::configure_display_lists(const scene_c::configuration_s& configuration) {
    assert(configuration.buffer_count >= 2 && configuration.buffer_count <= 4);
    // If size changes, we must clear all
    if (_configuration == nullptr || viewport_c::backing_size(_configuration->viewport_size) != viewport_c::backing_size(configuration.viewport_size)) {
        for (auto p : _display_lists) {
            _deletion_display_lists.emplace_back(p);
        }
        _display_lists.clear();
        if (_clear_display_list) {
            _deletion_display_lists.emplace_back(_clear_display_list);
            _clear_display_list = nullptr;
        }
    }
    // Remove excess lists
    while (_display_lists.size() > configuration.buffer_count) {
        int idx = _active_display_list - 1;
        if (idx < 0) {
            idx += _display_lists.size();
        } else {
            --_active_display_list;
        }
        _deletion_display_lists.emplace_back(_display_lists[idx]);
        _display_lists.erase(idx);
    }
    if (!configuration.use_clear && _clear_display_list != nullptr) {
        _deletion_display_lists.emplace_back(_clear_display_list);
        _clear_display_list = nullptr;
    }
    // Update palette for remaining lists
    if (configuration.palette) {
        for (int i = 0; i < _display_lists.size(); ++i) {
            auto pal = _display_lists[i]->get(PRIMARY_PALETTE).palette_ptr().get();
            copy(configuration.palette->begin(), configuration.palette->end(), pal->begin());
        }
    }
    // Add new required lists
    auto make_list = [&]() {
        auto listptr = new display_list_c();
        auto pal = new palette_c();
        if (configuration.palette) {
            copy(configuration.palette->begin(), configuration.palette->end(), pal->begin());
        }
        auto vpt = new viewport_c(configuration.viewport_size);
        vpt->set_offset(point_s(0,0));
        listptr->emplace_front(PRIMARY_PALETTE, -1, pal);
        listptr->emplace_front(PRIMARY_VIEWPORT, -1, vpt);
        return listptr;
    };
    while (_display_lists.size() < configuration.buffer_count) {
        _display_lists.emplace_back(make_list());
    }
    if (configuration.use_clear && _clear_display_list == nullptr) {
        _clear_display_list = make_list();
    }
    _configuration = &configuration;
}

void scene_manager_c::swap_display_lists() {
    ++_active_display_list;
    if (_active_display_list >= _display_lists.size()) {
        _active_display_list -= _display_lists.size();
    }
}


viewport_c& scene_manager_c::update_clear() {
    auto& clear_viewport = _clear_display_list->get(PRIMARY_VIEWPORT).viewport();
    for (auto list : _display_lists) {
        auto& viewport = list->get(PRIMARY_VIEWPORT).viewport();
        viewport.dirtymap()->merge(*clear_viewport.dirtymap());
    }
    clear_viewport.dirtymap()->clear();
    debug_cpu_color(DEBUG_CPU_PHYS_RESTORE);
    auto& back_viewport = display_list(back).get(PRIMARY_VIEWPORT).viewport();
    back_viewport.dirtymap()->restore(back_viewport, clear_viewport.image());
    return back_viewport;
}

void scene_manager_c::update_scene(scene_c& scene, int32_t ticks) {
    debug_cpu_color(DEBUG_CPU_TOP_SCENE_TICK);
    display_list_c& back = display_list(display_list_e::back);
    scene.update(back, ticks);
}

void scene_manager_c::begin_transition(transition_c* transition, const scene_c* from, scene_c* to, bool obscured) {
    if (to) {
        const auto& config = to->configuration();
        // TODO: This needs to move to transition eventually.
        configure_display_lists(config);
        if (config.use_clear) {
            auto& clear = display_list(display_list_e::clear);
            auto& viewport = clear.get(PRIMARY_VIEWPORT).viewport();
            viewport.with_dirtymap(viewport.dirtymap(), [&]{
                to->will_appear(obscured);
            });
        } else {
            to->will_appear(obscured);
        }
    }
    if (_transition) delete _transition;
    if (transition) {
        _transition = transition;
    } else {
        _transition = new no_transition_c();
    }
    _transition->will_begin(from, to);
}

void scene_manager_c::update_transition(int32_t ticks) {
    assert(_transition && "Transition must not be null");
    debug_cpu_color(DEBUG_CPU_RUN_TRANSITION);
    bool done = _transition->tick(ticks);
    if (done) {
        end_transition();
    }
}

void scene_manager_c::end_transition() {
    assert(_transition && "Transition must not be null");
    delete _transition;
    _transition = nullptr;
}
