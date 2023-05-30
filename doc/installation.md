# Installation {#installation}

### CMake

Use classic CMake approach:

```sh
mkdir build
cd build
cmake ..
cmake --build .
cmake --install .
```

After the build/installation, there'll be 2 CMake targets: `sbepp::sbeppc` and
`sbepp::sbepp`, check out the [Integration](#integration) section to see how
they can be used.

`sbepp` is a C++11 header-only library which depends only on the STL.

`sbeppc` requires C++17. It depends on:
- [fmt](https://github.com/fmtlib/fmt)
- [pugixml](https://github.com/zeux/pugixml)

Available CMake options (`name = default_value`):

- `SBEPP_BUILD_SBEPPC = ON`, controls whether `sbeppc` should be built
- `SBEPP_DEV_MODE = OFF`, enables developer mode.
- `SBEPP_BUILD_BENCHMARK = OFF`, requires `SBEPP_DEV_MODE=ON`, controls whether
benchmarks should be built. If
`ON`, requires [benchmark](https://github.com/google/benchmark)
- `SBEPP_BUILD_TESTS = OFF`, requires `SBEPP_DEV_MODE=ON`, controls whether
tests should be built. If `ON`,
requires [googletest](https://github.com/google/googletest) and
[fmt](https://github.com/fmtlib/fmt)
- `SBEPP_BUILD_DOCS = OFF`, controls whether documentation should be built. If
`ON` requires [Doxygen](https://doxygen.nl/)

There's a `conanfile.txt` which can be used to simplify dependency management.

### Conan package

Now available in [ConanCenter](https://conan.io/center/sbepp). Use `with_sbeppc`
Conan option to enable/disable `sbeppc` building (defaults to `True`).