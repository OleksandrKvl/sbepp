# 1.4.1

- fixed `sbepp::size_bytes_checked` example
- fixed `fmt/11` build
- fixed `byteswap` MSVC error

---

# 1.4.0

- added checks to protect against using cursor accessors in a wrong order

---

# 1.3.0

- fixed optional built-in types macro.
- provided `operator->()` for iterators.
- removed `!empty()` pre-condition from `dynamic_array_ref::data()`.
- made `size_bytes()` `constexpr` for composites.
- added `size_bytes()` to message/group/data traits to predict their size.
- added `traits_tag` to map representation types to traits tags.
- implemented `visit()` for enums and sets.
- deprecated `enum_to_string()` and `visit_set()`.
- fixed dependency chain for `sbeppc` CMake helpers.
- added basic string support in form for `assign_string()`, `assign_range()`,
    `strlen()` and `strlen_r()`.
- introduced multi-version documentation.

Thanks to Dmytro Ovdiienko (@ujos) for his feedback that influenced most of the
above changes.

---

# 1.2.0

Add `sbeppc` CMake helpers. (Thanks to @ngrodzitski for the idea and initial
implementation).  
Fix `[[nodiscard]]` detection on Clang.  
Fix `std::make_unsigned`-based SFINAE for floating-point types.  
Use Doxygen 1.9.8 to generate documentation.  
Update `doxygen-awesome-css`.

---

# 1.1.0

Remove specific `fmt` and `pugixml` version requirements from `find_package`.
This is supposed to simplify integration with their older/newer versions leaving
potential problems to client.  
Fix ADL issues when using `std::byte` as a byte type for views.

---

# 1.0.1

Fix floating-point types byte swapping for pre-C++20 compilers.

---

# 1.0.0

Initial public release