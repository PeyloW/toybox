# C++ Project Guidelines

## Platform Requirements

- **Compiler:** All code must compile with gcc and clang with C++23 enabled.
- **Standard Library:** No standard libraries linked.
- **Exceptions:** No exceptions. Do not use `noexcept` specifications.
- **Integer/Pointer Sizes:** Make no assumptions about integer/pointer size.
	- Host may use 32-bit integers, target **must** use 16-bit integers.
	- Whenever possible use explicitly sized types: `int16_t` not `short`.
	- Use `int` instead of `size_t` when sizes are reasonably within 30k.
		- Collection sizes, etc.
		- Use `size_t` for file sizes, or when the C++ standard absolutely requires. 
- **Struct Sizes:** Use `static_assert` to ensure expected sizes for structs are correct.
- **Compiler Attributes:** Common attributes used throughout the codebase:
	- `__forceinline` - Force function inlining (for single-statement functions).
	- `__neverinline` - Prevent function inlining.
	- `__forceinline_lambda` - Force lambda inlining for performance.
	- `__pure` - Mark functions with no side effects (enables compiler optimizations).
	- `const_pure` - Combine `const` with `__pure` for const member functions.
	- `const_purest` - Combine `const` with `__purest` for const member functions that do not read `this` or any memory (return compile-time constants).
	- `__packed` - Pack struct/class with no padding.
	- `__packed_struct` - Pack struct with 2-byte alignment (critical for Atari target memory layout).
	- `__target_volatile` - Conditional volatile for target-specific code.

## Naming Conventions

- **General Rule:** `snake_case` (e.g., `my_class`, `point_3d`, `process_data`).
- **Type Suffixes:**
	- `_t` - POD (plain old data) types.
	- `_e` - Enumerations.
	- `_s` - Simple structs (for direct access to all members).
	- `_c` - Classes.
		- Classes **must** support move semantics.
		- **Never** assume copy semantics are available.
		- Most classes explicitly forbid copy semantics by subclassing `nocopy_c`.
	- `_f` - Function pointer and callable types.
- **Variable Prefixes:**
	- `_` - Private/protected member variables.
	- `s_` - Static variables.
	- `g_` - Global variables.
	- No prefix - Local variables, public members, or function arguments.
- **Out Arguments** Out arguments should be last, and suffixed with `_out`.
	- Use out by reference if required, and out by pointer if optional.
	- In-our arguments should not have any special suffix, just non const regular arguments.
- **Constants and Macros:** `UPPER_SNAKE_CASE` (e.g., `MAX_SIZE`, `PI_VALUE`).
- **Template Parameters:** Use longer template names for templates over about 5 lines of code, examples:
	- Use `Type` for type parameters (e.g., `template<class Type>`).
	- Use `Count` or `Size` for size parameters (e.g., `template<size_t Count>`).

## Code Organization

- **Encapsulation:** Wrap implementation details in `detail` namespace or nested `detail` class or struct.
- **Concepts:** Use concepts to constrain types.
	- Prefer in templates (e.g., `template<forward_iterator I>`).
	- If require clause becomes complex ask user if a new named concept should be created.
- **Typed Enums:** Use typed class enums.
	- Place inside class if used by single class (including subclasses).
		- Re-export with `using enum` for easier access, while still local.
- **Bit Sets:** Use the `is_optionset` helper for enums that are bit sets.
- **Header Guards:** Use `#pragma once`.

## Code Formatting

- **Indentation:** Use 4 spaces for indentation.
- **Brace Style:** Opening brace on same line.
	- Use braces for all statements except single `return`/`continue`/`break` after `if` statements.
	- Keep braces and statement on single line if not exceeding 80 characters.
- **Type Declarations** Type declaration operator should be next to the type.
	- Use `int&` and `my_type_t&`, never `bool *` or `char *func()`.
- **Qualifier Order:**
	1. Storage class (`static`, `extern`, `mutable`)
	2. Compile-time semantics (`inline`, `__forceinline`,`constexpr`)
	3. Type qualifiers (`const`, `volatile`)
	4. Other attributes (`explicit`, `friend`, `virtual`)
	5. Return type
	- Function qualifiers after parameters: `const`, `override`, `final` (in that order).
	- If several qualifiers from same group appears, use order as defined for the group.
- **Function Inlining:**
	- Use `__forceinline` for single-statement functions.
	- Exclude assert statements and non-Atari preprocessor blocks from statement count.
	- Assert statements do not count as statements for inline classification purposes.
	- Static functions named with a `__` prefix can use `__forceinline`, these are helper functions fo more readable code.
	- Functions in `detail` namespace can also be `__forceinline`, these are also helper functions.

## Documentation

- **Comment Syntax:** Use `//` for single-line comments and `/* ... */` for multi-line comments.
- **API Documentation:** Use `/** ... */` for multi-line and `///` for single-line.
	- For classes: two or three sentences, for functions: one or two.
	- Call out main usage, and if needed deviations from std lib API.
	- Do not document what would be self-evident to 95% of all C++ programmers.
- **Implementation Comments:** Comment complex implementation details inline.

## Best Practices

- **Resource Management:** Employ RAII (Resource Acquisition Is Initialization) principles.
- **Memory Management:** Prefer static allocation over heap allocations.
	- You may allocate memory up front, **never** in hot paths.
	- Use `unique_ptr_c` and `shared_ptr_c` to manage memory that outlives current scope.
		- Always prefer `unique_ptr_c` when possible.
- **Argument Passing:** Any type that does not fit in 4 bytes should be passed as a reference.
- **Const Usage** Always prefer using `const`.
	- Also for temporaries, and arguments.
	- Do not use `const` for pod arguments that are implied `const`.
	- Use `const_pure` instead of `const` for const member functions with no side effects.
	- Use `const_purest` for const member functions that return compile-time constants without reading `this`.
	- When `const` is followed by `override` or `final`, use `const override __pure` instead (GCC rejects attributes between `const` and virt-specifiers).
	- Do not use `const_pure` on template member function definitions in template bodies (GCC `-Wtemplate-body` rejects this). Use plain `const` for these.
- **Error Handling:**
	- Use `assert()` to catch errors when building for host machine.
	- All `assert()` statements should include a **succinct** error message: `assert(condition && "Error message")`.
		- Message can be excluded if check is obvious (eg. `_count < Count`).
		- Message may **never** be longer than 40 characters. 
	- Use `assert()` liberally for error checking, as asserts are compiled out in release builds.
	- Use `hard_assert()` for exceptional error conditions that must also be caught on target machine.
- **Performance:** Optimize critical sections of code.
	- Prefer succinct and readable code, but you may sacrifice both if m68k target benefits.
	- When applicable use helper functions in `system_helpers.hpp` to boost target performance.
	- Make use of `constexpr` instead of run-time when possible.
	- Use `fix16_t` instead of floating point math.
- **Type Casting** Prefer to use `static_cast`.
	- Use `const_cast` and `reinterpret_cast` when applicable.
	- Never use `dynamic_cast`. 
- **Standard Library Usage:** You **must never** use the standard C++ library.
	- When needed minimal implementations of stdlib functionality is recreated, such as `vector_c` for `std::vector` and `forward(...)` for `std::forward(...)`.

## Project-Specific Instructions

- **Project Setup/Philosophy:** Read README.md for further information.
	- Keep information that humans would need in README.md.
- **Dependencies:** All dependencies are managed in `common.mk`.
	- Do not add or remove any dependencies without verifying with user.
- **Build System:** The project builds using make.
	- `common.mk`, `product.mk` are shared across all makefiles.
	- Each sub project has their own minimal `Makefile`.
	- Each sub project must always build both for host and target machine.
		- Builds for target Atari machine by default.
		- Adding `HOST=sdl2` to build for host machine.
		- Validate host machine builds/tests first.
			- Only build target machine at end, or when requested.
- **Testing:** All unit tests are in the tests sub project.
	- Only add unit tests for core functionality that does not generate visuals or audio.
	- Ask user to verify visuals and audio by running the sample sub project on target machine.
- **Logging:** Do not add logging.

## Workflows

- **Bug Fixes:** When implementing a bug fix
	- Prefer small and targeted bug fixes.
	- Do not make changes outside the requested functionality.
	- If bug fix requires further changes explain to, and verify with user.
	- If a bug fix requires more than ten lines of change; first show proposed changes to user and ask for confirmation.
- **New Features:** When asked to implement a new feature
	- **Always** search the project for similar features first.
	- Follow existing architecture style.
	- Do not duplicate code, ask user if and where to refactor out common functionality.
- **Documentation:** Never add new documentation files, unless explicitly asked to.
	- Documentation in code only, according to style described above.
