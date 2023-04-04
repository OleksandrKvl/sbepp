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
        COMMAND $<TARGET_FILE:sbepp::sbeppc>
            "${CMAKE_CURRENT_LIST_DIR}/schemas/${schema}.xml"
        DEPENDS sbepp::sbeppc "${CMAKE_CURRENT_LIST_DIR}/schemas/${schema}.xml"
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
    target_link_libraries(${test_name}
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