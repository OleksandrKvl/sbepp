# Integration into your project {#integration}
[TOC]

After schema compilation, the only thing you need to use it is to include
generated files into your project and link to `sbepp::sbepp` CMake target.

---

## Compiling schema on-the-fly

The most simple way is to compile schema on-the-fly, that is, to store only
schema XML in your source tree and generate C++ headers from it during the build
process.

---

### CMake helpers

`sbepp` provides a couple of helpers to make on-the-fly schema compilation as
easy as possible.

`sbeppc_compile_schema` - creates `INTERFACE` library that compiles SBE schema
using `sbeppc`.  
Parameters:
- `SCHEMA_FILE`, the path to schema XML file.
- `TARGET_NAME`, the name of the generated `INTERFACE` library. Same target can
    be used to compile multiple schemas.
- `OUTPUT_DIR`, optional, the directory in which `sbeppc` will generate headers.
    Defaults to `${CMAKE_BINARY_DIR}/sbeppc_generated`.
- `SCHEMA_NAME`, optional, overrides schema name from XML.
- `SBEPPC_PATH`, optional, the path to `sbeppc` binary, uses `sbepp::sbeppc`
    CMake target by default.

This function creates custom target using `sbeppc_make_schema_target` (described
below) and then creates `INTERFACE` library on top of it. This library carries
all necessary dependencies and settings so there's a minimal amount of work left
to user:

```cmake
sbeppc_compile_schema(
    TARGET_NAME compiled_schemas
    SCHEMA_FILE "schema1.xml"
)

sbeppc_compile_schema(
    # note that we can use the same target name for many schemas
    TARGET_NAME compiled_schemas
    SCHEMA_FILE "schema2.xml"
)

add_executable(exe)
# generated target carries all necessary dependencies, just link to it
target_link_libraries(exe PRIVATE compiled_schemas)
```

---

`sbeppc_make_schema_target` - creates a custom target that compiles SBE schema
using `sbeppc`.  
Parameters:
- `SCHEMA_FILE`, the path to schema XML file.
- `TARGET_NAME`, the name of generated custom target. Same target can be used to
    compile multiple schemas.
- `OUTPUT_DIR`, the directory in which `sbeppc` will generate headers.
- `SCHEMA_NAME`, optional, overrides schema name from XML.
- `SBEPPC_PATH`, optional, the path to `sbeppc` binary, uses `sbepp::sbeppc`
    CMake target by default.

This is a lower-level function than `sbeppc_compile_schema`. It only creates
a custom command and a custom target to compile the schema leaving the rest of
work up to user. When `SCHEMA_NAME` is not provided, it also creates a special
empty anchor file with a name like `<HASH>.anchor` for a schema. This file is
indirectly included into all generated headers and required for CMake and
underlying build systems to establish correct dependency graph and enforce
automatic recompilation of consumers of this target when either schema XML or
`sbeppc` itself changes.

Example:

```cmake
sbeppc_make_schema_target(
    SCHEMA_FILE "schema1.xml"
    TARGET_NAME compiled_schemas
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)

sbeppc_make_schema_target(
    SCHEMA_FILE "schema2.xml"
    TARGET_NAME compiled_schemas
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(exe)
add_dependencies(exe compiled_schemas)
target_include_directories(exe SYSTEM PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(exe PRIVATE sbepp::sbepp)
```

---

### Manual schema compilation

Here's an example of how to implement the above functionality by-hand:

```cmake
set(schema "schema_name")
set(schema_path "${CMAKE_CURRENT_LIST_DIR}/schemas/${schema}.xml")

# mention one of the output files
set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${schema}/${schema}.hpp")

# add a command which compiles the schema
add_custom_command(
    OUTPUT ${output_file}
    COMMAND $<TARGET_FILE:sbepp::sbeppc> "${schema_path}"
    DEPENDS "${schema_path}"
)

# add a custom target which depends on the above
add_custom_target(compile_schema
    DEPENDS ${output_file}
)

# add it to dependencies of your target
add_executable(my_project)
add_dependencies(my_project compile_schema)

# add CMAKE_CURRENT_BINARY_DIR to be able to access generated headers
target_include_directories(my_project
    SYSTEM PRIVATE                      # Note `SYSTEM`
    ${CMAKE_CURRENT_BINARY_DIR}
)

# link to sbepp::sbepp
target_link_libraries(my_project
    PRIVATE
    sbepp::sbepp
)
```

---

## Using pre-compiled schema

Another approach is to use pre-compiled schema headers, that is, generate
headers once and store them in the source tree. This might be useful when it's
not desired/possible to build/install `sbeppc` in developer environment (e.g.
if C++17 required for `sbeppc` is not available). It requires building `sbeppc`
once somewhere, generating schema headers using it and pushing them to the
source tree to be used by others. In this case care should be taken to keep
once-generated headers [compatible](#versioning) with `sbepp` if it's installed
automatically via some dependency management mechanism.

@note
Check out [SBEPP_BUILD_SBEPPC](#installation-cmake) and
[with_sbeppc](#installation-conan) options to see how to disable `sbeppc`
building for such cases.

Also, since it's a third-party code, it's recommended to mark include path for
generated headers as `SYSTEM` to hide potential warnings.

---

## Versioning and compatibility {#versioning}

`sbepp` uses [SemVer](https://semver.org/) that is defined in terms of *public
API*, its users and its changes. `sbepp` defines public API as:
- `sbepp::sbeppc` schema compiler
- public parts of `sbepp::sbepp` supporting library
- public parts of generated headers

The most important consequence from this definition is that **whether a change
is compatible/incompatible depends only on how it affects user's code, not the
generated one.** In practice it means that headers generated by `sbepp::sbeppc`
version `1.2.3` might not be compatible with `sbepp::sbepp` version `1.3.0` and
even with `1.2.4`. Such incompatibility requires schema recompilation but not
the user's code change (only MAJOR update will require that).

To summarize, the advised strategy is to use the same version of `sbepp::sbeppc`
and `sbepp::sbepp` to generate and use headers. Types from the `detail`
namespace should never be mentioned/used explicitly, they can be affected even
by the PATCH version change.