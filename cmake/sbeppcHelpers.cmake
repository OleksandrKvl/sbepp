# Wrapper around `cmake_parse_arguments` that implements
# `${prefix}_KEYWORDS_MISSING_VALUES` in CMake <3.15
macro(sbepp_parse_arguments
        prefix options one_value_keywords multi_value_keywords
    )
    cmake_parse_arguments(
        "${prefix}"
        "${options}"
        "${one_value_keywords}"
        "${multi_value_keywords}"
        "${ARGN}"
    )
    if(CMAKE_VERSION VERSION_LESS "3.15")
        foreach(keyword IN LISTS one_value_keywords multi_value_keywords)
            if(NOT DEFINED ${prefix}_${keyword})
                list(APPEND ${prefix}_KEYWORDS_MISSING_VALUES ${keyword})
            endif()
        endforeach()
    endif()

    if(DEFINED ${prefix}_UNPARSED_ARGUMENTS)
        message(WARNING "Unparsed arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()

    if(DEFINED ${prefix}_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR
            "Keywords with missing values: ${arg_KEYWORDS_MISSING_VALUES}"
        )
    endif()
endmacro()

# A helper function to integrate `sbeppc` SBE schema compilation into CMake
# project. Creates a custom target with given name that compiles SBE schema.

# A helper function that creates a custom target that compiles SBE schema using
# `sbeppc`.
# 
# Arguments:
# 
#   SCHEMA_FILE: the path to schema XML file
#   TARGET_NAME: the name of generated custom target. A single target can be
#       used to compile multiple schemas.
#   OUTPUT_DIR: the directory in which `sbeppc` will generate headers
#   SCHEMA_NAME: optional, overrides schema name from XML
#   SBEPPC_PATH: optional, the path to `sbeppc`, uses `sbepp::sbeppc` by default
# 
# Example:
# 
# sbeppc_make_schema_target(
#     SCHEMA_FILE "schema.xml"
#     TARGET_NAME compiled_schema
#     OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
# )
# add_executable(exe)
# add_dependencies(exe compiled_schema)
# target_include_directories(exe SYSTEM PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
# target_link_libraries(exe PRIVATE sbepp::sbepp)
# 
function(sbeppc_make_schema_target)
    set(one_value_args
        SCHEMA_FILE TARGET_NAME OUTPUT_DIR SCHEMA_NAME SBEPPC_PATH
    )
    sbepp_parse_arguments(arg "" "${one_value_args}" "" "${ARGN}")

    if(NOT DEFINED arg_SCHEMA_FILE)
        message(FATAL_ERROR "Missing SCHEMA_FILE argument")
    endif()

    if(NOT DEFINED arg_OUTPUT_DIR)
        message(FATAL_ERROR "Missing OUTPUT_DIR argument")
    endif()

    if(NOT DEFINED arg_TARGET_NAME)
        message(FATAL_ERROR "Missing TARGET_NAME argument")
    endif()

    get_filename_component(schema_real_path "${arg_SCHEMA_FILE}" REALPATH)
    string(SHA256 anchor_file_name "${schema_real_path}")
    # `cmake_path` requires CMake >=3.20 so concatenate by hand
    set(anchor_file_path "${arg_OUTPUT_DIR}/${anchor_file_name}.anchor")

    if(DEFINED arg_SBEPPC_PATH)
        list(APPEND sbeppc_command "${arg_SBEPPC_PATH}")
    else()
        list(APPEND sbeppc_command "$<TARGET_FILE:sbepp::sbeppc>")
    endif()

    if(DEFINED arg_SCHEMA_NAME)
        list(APPEND sbeppc_command "--schema-name" "${arg_SCHEMA_NAME}")
    endif()

    list(APPEND sbeppc_command
        "--output-dir" "${arg_OUTPUT_DIR}"
        "${arg_SCHEMA_FILE}"
    )

    list(APPEND sbeppc_command_deps "${arg_SCHEMA_FILE}")
    # depend explicitly only on `SBEPPC_PATH` if given, otherwise the usage of
    # `TARGET_FILE` creates dependency on `sbepp::sbeppc` target implicitly
    if(DEFINED arg_SBEPPC_PATH)
        list(APPEND sbeppc_command_deps "${arg_SBEPPC_PATH}")
    endif()

    add_custom_command(
        VERBATIM
        COMMAND
            ${sbeppc_command}
        DEPENDS
            ${sbeppc_command_deps}
        COMMENT
            "Compiling SBE schema: ${arg_SCHEMA_FILE}"

        # We don't know any output file name because it depends on the content
        # of the schema so we generate a fake anchor file to depend on it in
        # `add_custom_target`
        COMMAND
            "${CMAKE_COMMAND}" -E touch "${anchor_file_path}"
        OUTPUT
            "${anchor_file_path}"
    )

    if(NOT TARGET "${arg_TARGET_NAME}")
        add_custom_target(
            "${arg_TARGET_NAME}"
            # To support multiple schemas with the same `TARGET_NAME` we depend
            # on an extendable list of anchor files stored in a custom property
            # `SBEPPC_ANCHORS`
            DEPENDS $<TARGET_PROPERTY:${arg_TARGET_NAME},SBEPPC_ANCHORS>
        )
    endif()

    set_property(TARGET "${arg_TARGET_NAME}"
        APPEND PROPERTY SBEPPC_ANCHORS "${anchor_file_path}"
    )
endfunction()

# A helper that creates `INTERFACE` library that compiles SBE schema using
# `sbeppc`.
# 
# Arguments:
# 
#   SCHEMA_FILE: the path to schema XML file
#   TARGET_NAME: the name of the generated `INTERFACE` library. A single target
#       can be used to compile multiple schemas.
#   OUTPUT_DIR: optional, the directory in which `sbeppc` will generate headers.
#       Defaults to `${CMAKE_BINARY_DIR}/sbeppc_generated`.
#   SCHEMA_NAME: optional, overrides schema name from XML
#   SBEPPC_PATH: optional, the path to `sbeppc`, uses `sbepp::sbeppc` by default
# 
# Example:
# 
# sbeppc_compile_schema(
#     TARGET_NAME compiled_schema
#     SCHEMA_FILE "schema.xml"
# )
# add_executable(exe)
# target_link_libraries(exe PRIVATE compiled_schema)
# 
function(sbeppc_compile_schema)
    set(one_value_args
        SCHEMA_FILE TARGET_NAME OUTPUT_DIR SCHEMA_NAME SBEPPC_PATH
    )
    sbepp_parse_arguments(arg "" "${one_value_args}" "" "${ARGN}")

    if(NOT DEFINED arg_OUTPUT_DIR)
        set(arg_OUTPUT_DIR "${CMAKE_BINARY_DIR}/sbeppc_generated")
    endif()

    set(custom_target_name ${arg_TARGET_NAME}_sbeppc_custom_target)

    set(make_schema_target_args
        SCHEMA_FILE "${arg_SCHEMA_FILE}"
        OUTPUT_DIR "${arg_OUTPUT_DIR}"
        TARGET_NAME "${custom_target_name}"
    )

    if(DEFINED arg_SCHEMA_NAME)
        list(APPEND make_schema_target_args SCHEMA_NAME ${arg_SCHEMA_NAME})
    endif()

    if(DEFINED arg_SBEPPC_PATH)
        list(APPEND make_schema_target_args SBEPPC_PATH "${arg_SBEPPC_PATH}")
    endif()

    sbeppc_make_schema_target(${make_schema_target_args})

    if(NOT TARGET ${arg_TARGET_NAME})
        add_library(${arg_TARGET_NAME} INTERFACE)
        target_link_libraries(${arg_TARGET_NAME}
            INTERFACE
            sbepp::sbepp
        )
    endif()

    target_include_directories(${arg_TARGET_NAME}
        SYSTEM INTERFACE "${arg_OUTPUT_DIR}"
    )

    add_dependencies(${arg_TARGET_NAME} ${custom_target_name})
endfunction()
