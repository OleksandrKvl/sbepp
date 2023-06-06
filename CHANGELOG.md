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