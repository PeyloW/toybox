## TOYBOX ROADMAP

Legend:

- [ ] Pending
- [-] In progress
- [x] Completed

### v1.1 - A modern toolchain

Split out ToyBox from ChromaGrid, allowing it to be used as a standalone piece of software for building simple mouse driven games.

- [x] Move toybox code into its own git repository
- [x] Separate Makefiles for toybox and ChromaGrid
- [x] Unified Makefile for Atari target and macOS host
    - [x] Move game loop to `machine_c::with_machine(...)`
    - [x] Add new Xcode project with external build system
- [x] Update to gcc 15.2 mintelf toolchain
    - [x] Use link time optimizations
    - [x] Update to use libcmini top of tree (0.54 too old for elf)
    - [x] Remove libcmini 0.47 workarounds
- [x] Update sources to ~~C++20~~ C++23
- [x] Add unit tests
- [x] Add sample project
- [x] Update documentation
- [x] Add Xcode IDE to toybox project


### v2.0 - A basic general purpose game engine

Support simple horizontally scrolling games controlled with joystick. ETA Summer 2026.

- [x] `fixed16_t` math library, 12:4 bits
- [x] `viewport_c` as a subclass of `canvas_c`
    - [x] Wrapper for an `image_c` with its own size and offset
    - [x] Separate viewport size from image size
    - [-] Move drawing primitives with translate, clip, and dirty to superclass
- [x] Basic `display_list_c` supporting single `viewport_c` and `palette_c`
    - [x] Use active `display_list_c` not `image_c`
    - [x] Hardware scroll viewport on Atari target
    - [x] Hardware scrolled display on host
- [ ] Rewrite `transition_c` system for display lists
    - [ ] Cross-fade through color
    - [ ] Update existing transitions
- [x] `tilemap_c` for defining a tiled display from 16x16 blocks
    - [x] Source from `tileset_c` and solid color
        - [x] Optimized batch drawing primitives for repeated 16x16 blocks
    - [x] General tile types; empty, solid, climbable, hurts, etc.
    - [x] Subtilemap splicing with dirty tracking
    - [x] Tilemap file format loading (Blocked on Editor)
- [x] `entity_c` for defining basic game AI
    - [x] User controllable entity
    - [x] Collision detection with tiles
    - [x] Collision detection with other entities
    - [ ] Optimize AI and rendering for visible entities only
- [ ] Implement `audio_mixer_c`
    - [ ] Four channels of mixed audio
- [x] Implement `controller_c` to read ST joystick 0 or 1
    - [ ] Basic `state_recognizer_c`
    - [ ] Implementations for taps and holds
- [ ] Implement `keyboard_c`
    - [ ] TAB key background color performance debugging
- [-] Tilemap editor
    - [x] Generate tileset from multiple images
    - [x] Edit subtilemaps
    - [ ] Edit entities and entity defs
    - [x] Implement file format
- [ ] Refine core APIs
    - [x] Improve geometry API clarity
    - [ ] Fix stencil mask bug on Atari target 
- [x] Update sample project
    - [x] Tilemap based scene
    - [x] Basic entities
        - [x] Player controlled with joystick
        - [x] Wall collision for the player
        - [x] Boxes can be pushed around  
    - [x] Hardware scroll horizontally a little bit

### v3.0 - A complete general purpose game engine

Support static one screen or eight way scrolling games with rasters and split-screen. Controlled by mouse, joystick, jagpad and/or keyboard. ETA Summer 2028.

- [ ] `display_list_c` with multiple items
    - [ ] Multiple `viewport_c` for viewport splits
    - [ ] Multiple `palette_c` for palette splits
    - [ ] Multiple `raster_c` for rasters and color cycling
- [ ] `viewport_c` with arbitrary horizontal and vertical offset
- [ ] Advanced game entities
    - [ ] Bullet AI
    - [ ] Path following AI
    - [ ] Ground walking AI
    - [ ] Flying AI
    - [ ] Decision tree AI
- [ ] `level_c` for persistent game levels
    - [ ] Dynamic level loading from `scene_c`
    - [ ] Persist level state when unloaded
    - [ ] Save and load game state
- [ ] Add jagpad support to `controller_c`
- [ ] Add `modmusic_c` as a concrete `music_c` subclass
    

### v4.0 - Support all the things!

- [ ] Amiga target
    - [ ] 32 color palette support
- [ ] Atari STfm target
- [ ] Jaguar64 target
- [ ] Sega Genesis target

