find_package(benchmark REQUIRED)

set(schema_name "benchmark_schema")
set(schema_file "${CMAKE_CURRENT_LIST_DIR}/${schema_name}.xml")
set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${schema_name}/${schema_name}.hpp")

add_custom_command(
    OUTPUT ${output_file}
    COMMAND $<TARGET_FILE:sbepp::sbeppc> "${schema_file}"
    DEPENDS sbepp::sbeppc "${schema_file}"
)

add_custom_target(compile_benchmark_schema DEPENDS ${output_file})

set(target "benchmark")
set(src_dir "src/sbepp/benchmark")

add_executable(${target})
add_dependencies(${target} compile_benchmark_schema)

target_sources(${target}
    PRIVATE
    ${src_dir}/sbepp_reader.cpp
    ${src_dir}/sbepp_cursor_reader.cpp
    ${src_dir}/raw_reader.cpp
    ${src_dir}/real_logic_reader.cpp
)

target_include_directories(${target}
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    "src"
)

target_link_libraries(${target}
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    sbepp::sbepp
)