# Integration into your project {#integration}

After schema compilation, the only thing you need to using it is to include
generated files into your project and link to `sbepp::sbepp` CMake target.

As an example, here are two common approaches:

1. Compiling schema on-the-fly using your build system. Here's how it can be
    achieved in CMake:

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

    This will automatically compile the schema when its XML or `sbeppc` changes.

2. Using pre-generated schema headers. Useful when you don't want to install
    `sbeppc` on each dev's machine or you don't have C++17 (required for
    `sbeppc`) there. In this case one should be careful to keep once-generated
    headers compatible with `sbepp` if it's installed automatically via some
    dependency management mechanism.  
    \note `sbepp` uses [SemVer](https://semver.org/). This means that generated
    headers are compatible with the same *major* version of `sbepp`.

    Check out [SBEPP_BUILD_SBEPPC](#installation) CMake option to disable
    `sbeppc` building for such cases.

Also, since it's a third-party code, I recommend to include headers path as
`SYSTEM` to hide potential warnings from generated code.

---

### CMake helpers

`sbepp 1.2.0` introduced a couple of helpers to automate steps described above.

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
for the client:

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
a custom command and a custom target to compile the schema leaving rest of work
up to client. It also creates a special empty anchor file with a name like
`<HASH>.anchor` for each schema. It's required for CMake to establish dependency
between custom command and custom target.

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
