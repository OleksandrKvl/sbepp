find_package(GTest REQUIRED)
find_package(fmt REQUIRED)
include(GoogleTest)
include(sbeppcHelpers)

function(get_available_cpp_versions result)
    set(supported_versions 11 14 17 20 23)
    foreach(version IN LISTS supported_versions)
        if("cxx_std_${version}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
            list(APPEND res ${version})
        endif()
    endforeach()

    set(${result} ${res} PARENT_SCOPE)
endfunction()

set(test_schemas
    test_schema
    test_schema2
    big_endian_schema
    traits_test_schema
    traits_test_schema2
)

foreach(schema IN LISTS test_schemas)
    sbeppc_compile_schema(
        SCHEMA_FILE "${CMAKE_CURRENT_LIST_DIR}/schemas/${schema}.xml"
        # actually, only `traits_test_schema2` needs explicit name
        SCHEMA_NAME "${schema}"
        TARGET_NAME compiled_test_schemas
    )
endforeach()

get_available_cpp_versions(cpp_versions)

set(src_dir "src/sbepp/test")

function(add_test_binary cpp_version)
    set(test_name tests_cpp_${cpp_version})
    add_executable(${test_name})
    sbepp_set_target_language_standard(${test_name} CXX ${cpp_version})
    target_sources(${test_name}
        PRIVATE
        ${src_dir}/assert_handler.cpp
        ${src_dir}/required.test.cpp
        ${src_dir}/optional.test.cpp
        ${src_dir}/enum.test.cpp
        ${src_dir}/set.test.cpp
        ${src_dir}/constants.test.cpp
        ${src_dir}/composite.test.cpp
        ${src_dir}/static_array_ref.test.cpp
        ${src_dir}/dynamic_array_ref.test.cpp
        ${src_dir}/message.test.cpp
        ${src_dir}/entry.test.cpp
        ${src_dir}/field_accessors.test.cpp
        ${src_dir}/utilities.test.cpp
        ${src_dir}/flat_group.test.cpp
        ${src_dir}/nested_group.test.cpp
        ${src_dir}/random_access_iterator.test.cpp
        ${src_dir}/forward_iterator.test.cpp
        ${src_dir}/input_iterator.test.cpp
        ${src_dir}/cursor.test.cpp
        ${src_dir}/memory_access.test.cpp
        ${src_dir}/traits.test.cpp
        ${src_dir}/visit.test.cpp
        ${src_dir}/size_bytes_checked.test.cpp
        ${src_dir}/borrowed_range.test.cpp
        ${src_dir}/stringification.test.cpp
        ${src_dir}/float_fields.test.cpp
        ${src_dir}/stdbyte_adl.test.cpp
        ${src_dir}/built_in_types.test.cpp
        ${src_dir}/top_header_includes.test.cpp
        ${src_dir}/handle_schema_message.test.cpp
        ${src_dir}/get_by_name.test.cpp
        ${src_dir}/nullify_optional_fields.test.cpp
        ${src_dir}/tag_type.test.cpp
        ${src_dir}/access_by_tag.test.cpp
    )

    target_include_directories(${test_name}
        PRIVATE
        "src"
    )
        
    target_link_libraries(${test_name}
        PRIVATE
        compiled_test_schemas
        GTest::gtest_main
        fmt::fmt
    )

    target_compile_definitions(${test_name} PRIVATE
        SBEPP_ENABLE_ASSERTS_WITH_HANDLER
    )

    sbepp_set_strict_compiler_options(${test_name})

    if(SBEPP_SEPARATE_TESTS)
        gtest_discover_tests(${test_name} TEST_PREFIX "${test_name}.")
    else()
        add_test(NAME "${test_name}" COMMAND "${test_name}")
    endif()
endfunction()

foreach(version IN LISTS cpp_versions)
    add_test_binary(${version})
endforeach()

add_subdirectory(sbeppc_errors)
add_subdirectory(naming_test)