# TOYBOX

A minimal C++ framework for writing Atari ST<sup>E</sup> entertainment software.

### Project Requirements

* GCC-15.2 (https://tho-otto.de/crossmint.php)
    * For fastcall support, and link time optimizations
* libcmini (top of tree) (https://github.com/freemint/libcmini)
    * Because we need a small libc alternative!

### Optional Project Requirements

* PSG play v0.7 (https://github.com/frno7/psgplay/tree/main)
    * If static library is available SNDH music can be played on host machine

### LLDB setup

If using lldb you can add to you `~/.lldbinit`:

```
command source <root_path>/toybox/ide/Xcode/.lldbinit
```

This will make the debugger treat custom classes like `vector_c` and `rect_s` render as standard C++ and system library types for easier debugging.

## Project Philosophy

ToyBox should be small, fast and convenient. In order to be small, toybox shall use a bare minimum of libcmini, and not include or implement anything not directly needed by a client program. In order to be fast, toybox shall rely on C++ compiler optimizations, and rely on error checking on host machine not M68k target. In order to be convenient, API shall be designed similar to C++ standard library and/or boost.

All code must compile with gcc and clang with C++23 enabled, no standard libraries linked!

Make no assumption of integer/pointer size. Host may use 32 bit integers, target **must** use 16 bit integers. Whenever possible use explicitly sized types, `int16_t` not `short`.

Try to avoid multiple inheritance, and when done only the first inherited class can be polymorphic. Add static asserts before the class definition to ensure this.

For memory management pass referenses wheneever possible. Use `unique_ptr_c` and `shared_ptr_c` to manage life time, and signal that a function takes ownership of a pointer. Functions returninga newly allocated object returns a plain pointer. Pointer arguments are only guaranteed to outlive the call to the function, no further.

Rely on `static_assert` to ensure expected sizes for structs are correct. Asserts are enabled on host, but not on Atari target. Asserts are used liberally to ensure correctness, with `assert` that is not compiled on Atari target, and `hard_assert` for critical errors that must be caught on all targets.

For known failable methods, such as IO we rely on `errno`, and `expected_c` helper class.
    
### Game Setup

ToyBox games are intended to run from identical code on an Atari target machine, as on a modern host such as macOS. This is abstracted away in the `machine_c` class.

* `machine_c` the machine singleton.
    * `with_machine` sets up the machine in supervisor mode, or configures the host emulation. Run the game in the provided function or lambda.

### Game Life-cycle

A game is intended to be implemented as a stack of scenes. Navigating to a new screen such as from the menu to a level involves either pushing a new scene onto the stack, or replacing the top scene. Navigating back is popping the stack; popping the last scene exits the game.

* `scene_manager_c` - The manager singleton.
    * `push(...)`, `pop(...)`, `replace(...)` to manage the scene stack.
    * `display_list(id)` access a specific display list by ID (clear=-1, front=0, back=1): front is being presented, back is being drawn, and clear is used for restoring other viewports from their dirtymaps.
    * Display lists can be accessed via the `display_list_e` enum with values: `clear`, `front`, `back`.
* `scene_c` - The abstract scene baseclass.
    * `configuration()` the scene configuration with viewport_size, palette, buffer_count, and use_clear flag.
    * `will_appear(bool obscured)` called when scene becomes the top scene and will appear.
        * Implement to draw initial content to the clear viewport.
    * `update(display_list_c& display_list, int ticks)` update the scene, drawing to the provided display list's viewport.
* `transition_c` - A transition between two scenes, run for push, pop and replace operations.

### Displaying Graphics

What is shown in screen is defined by the currently active display list, the scene manager sets the front display list as the active display list when swapping display lists.

* `display_list_c` - A sorted list of display items (viewports and palettes) to be presented.
    * `PRIMARY_VIEWPORT` constant (-1) for the main viewport.
    * `PRIMARY_PALETTE` constant (-2) for the main palette.
    * As of ToyBox 2.0 only one item per type is allowed, in future version additional items will be used to do raster and screen splits.
    * A display list is in concept similar to an Amiga copper list.
* `viewport_c` - A viewport for displaying content, analogous to an Amiga viewport.
	* The view port has a size, and an offset for controlling hardware scrolling.
		* A size larger than a screen means the viewport is scrollable.
			* As of ToyBox v2.0 the view port can be up to about 6 screens wide, but must be one screen tall.
		* The offset is the top left visible pixel of the viewport visible on screen.
			* Setting the offset updates the clipping rectangle to the potentially visible rect of the screen.   
	* Subclass of `canvas_c` for drawing operations.
		* **Warning**: The viewport is responsible for managing the clip rectangle.
	* Contains an `image_c` for the bitmap buffer and a `dirtymap_c` for dirty region tracking.
		* **Warning**: The image size may be smaller than the viewport size, with the canvas intentionally drawing out of bounds.
			* For horizontal HW scroll an image only 16px wider than the screen is needed, with additional lines at the bottom to allow for the bitmap data to be shifted.
			* Example: With an offset of {50,0} the visible first pixel on screen is {48,0}. Drawing to pixel {350,0} will draw on screen space pixel {302,0} and in image space pixel {30,1}.
		* The dirtymap is in viewport size, it is the responsibility of the client to only restore within the clip rect.
			* Masking the dirtymap to the clip rect before restore is the easiest solution 

### Drawing Graphics

Graphics are drawn onto a canvas; a viewport is a subclass of canvas. Source of drawing operations can be an image, a tile from a tileset, or text using a font. 

* `canvas_c` The manager of drawing operations onto a destination `image_c`.
	* By default drawing operations are clipped against the clip rect, and dirties the dirty rect. You may want to override this:
		* `with_clipping()` draw with clip rect enabled/disabled
			* Clipping is expensive, avoid if possible
		* `with_dirtymap()` draw with dirtymap enabled/disabled
			* Must be disabled for restore operations.   
	* `draw_aligned()` Draws graphics that are aligned to even 16px.
		* Use for performance when drawing for example tile graphics.
		* Only image and tileset source allowed
		* Source must not be masked.
		* Operation may use a stencil
	* `draw()` All other drawing operations
		* Any source allowed.
		* Masked sources are drawn as sprites.
		* A masked source may be drawn as a solid color only
		* Stencil is always ignored
	* `draw_3_patch()` Draw an image with fixed width left and right caps, repeating/clipping the middle part as needed.
		* Use to draw UI elements such as buttons.  
* `image_c` A bitmap image, optionally with a mask
	* The backing store for all graphics
	* Create programmatically by size, or load from ILBM IFF files.
* `tileset_c` A wrapper of an image to represent a set of equally sized tile images. 
	* Use for tileset or sprite sheets.
	* Create from an image and tile size.
* `font_c` Conceptually similar to a tileset, with each tile image representing an ASCII character.
	* Supports monospaced and variable width characters.
	* Create from an image and a character size.
		* Variable width character fonts have equal sized characters in image, the font searches empty space to size characters at construction.

### Asset Management

Assets are images, sound effects, music, levels, or any other data the game needs. All assets may not fit in memory at once, and thus need to be loaded and unloaded on demand.

* `asset_manager_c` - The manager singleton, intended to be subclassed for adding typed custom assets.
    * `shared()` get the shared singleton.
    * `image(id)`, `tileset(id)`, `font(id)`, etc. get an asset, load if needed.
    * `preload(sets)` preload assets in batches.
    * `add_asset_def(id, def)` add an asset definition, with an ID, batch sets to be included in, and optionally a lambda for how to load and construct the asset.
* `image_c` an image asset, loaded from EA IFF 85 ILBM files (.iff).
* `sound_c` a sound asset, loaded from Audio Interchange File Format (.aif).
* `ymmusic_c` a music asset, loaded from SNDH files (.sndh).
* `font_c` a font asset, based on an image asset, monospace or variable width characters.
* `tileset_c` a tileset asset, based on an image asset, defaults to 16x16 blocks.

### Tilemaps and Levels

ToyBox provides a tilemap system for creating tile-based game worlds with entities and AI.

* `tilemap_c` - A 2D grid of tiles for defining game levels.
    * `tile_s` structure with index, type (none, solid, platform, water), flags, and custom data.
    * Supports accessing tiles with `operator[x, y]`.
    * Can be used as a base class or standalone.
* `tilemap_level_c` - An extended tilemap with entity support and rendering.
    * Inherits from both `tilemap_c` and `asset_c`.
    * Can be loaded from files or created procedurally.
    * `update(viewport_c& viewport, int display_id, int ticks)` to update and render the level.
    * Manages entities with AI actions.
    * Handles dirty region tracking for tile updates.
* `entity_s` - A game entity structure for sprites, enemies, items, etc.
    * Type, group, action index, frame index, flags (e.g., hidden).
    * Position as fixed-point rectangle (`frect_s`).
    * Custom data storage for entity-specific and action-specific data.
* `action_f` - Function type for AI actions: `void(*)(tilemap_level_c&, entity_s&)`.
    * Actions are registered in the level's action vector.
    * Each entity references an action by index.
