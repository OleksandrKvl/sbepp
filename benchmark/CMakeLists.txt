
set(schema_name "benchmark_schema")

sbepp_generate(
    SCHEMA_NAME ${schema_name}
    SCHEMA_FILE ${CMAKE_CURRENT_LIST_DIR}/${schema_name}.xml
    OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
    TARGET_NAME compile_benchmark_schema
)

set(target "sbepp-benchmark")
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
