find_package(GTest REQUIRED)
find_package(fmt REQUIRED)
include(GoogleTest)

function(get_available_cpp_versions result)
    set(supported_versions 11 14 17 20 23)
    foreach(version IN LISTS supported_versions)
        if("cxx_std_${version}" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
            list(APPEND res ${version})
        endif()
    endforeach()

    set(${result} ${res} PARENT_SCOPE)
endfunction()

# compile test schemas
set(test_schemas
    test_schema
    test_schema2
    big_endian_schema
    traits_test_schema
    traits_test_schema2
)

foreach(schema IN LISTS test_schemas)
    # multi-file compilation
    set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${schema}/${schema}.hpp")
    list(APPEND schema_output_files ${output_file})

    add_custom_command(
        OUTPUT ${output_file}
        # actually, only `traits_test_schema2` needs explicit name
        COMMAND $<TARGET_FILE:sbepp::sbeppc>
            "--schema-name" "${schema}"
            "${CMAKE_CURRENT_LIST_DIR}/schemas/${schema}.xml"
        DEPENDS sbepp::sbeppc "${CMAKE_CURRENT_LIST_DIR}/schemas/${schema}.xml"
    )
endforeach()

add_custom_target(compile_test_schemas DEPENDS ${schema_output_files})

get_available_cpp_versions(cpp_versions)

set(src_dir "src/sbepp/test")

function(add_test_binary name_suffix cpp_version)
    set(test_name tests_cpp_${cpp_version}_${name_suffix})
    add_executable(${test_name})
    sbepp_set_target_language_standard(${test_name} CXX ${cpp_version})
    add_dependencies(${test_name} compile_test_schemas)
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
    )

    target_include_directories(${test_name}
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        "src"
    )
        
    target_link_libraries(${test_name}
        PRIVATE
        sbepp::sbepp
        GTest::gtest_main
        fmt::fmt
    )

    target_compile_definitions(${test_name} PRIVATE
        SBEPP_ENABLE_ASSERTS_WITH_HANDLER
        ${ARGN}
    )

    sbepp_set_strict_compiler_options(${test_name})

    gtest_discover_tests(${test_name} TEST_PREFIX "${test_name}.")
endfunction()

foreach(version IN LISTS cpp_versions)
    add_test_binary("mf" ${version})
    add_test_binary("tf" ${version} USE_TOP_FILE)
endforeach()
