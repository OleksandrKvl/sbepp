include(sbeppcHelpers)

find_package(benchmark REQUIRED)

sbeppc_compile_schema(
    SCHEMA_FILE "${CMAKE_CURRENT_LIST_DIR}/benchmark_schema.xml"
    TARGET_NAME compiled_benchmark_schemas
)

set(target "benchmark")
set(src_dir "src/sbepp/benchmark")

add_executable(${target})

target_sources(${target}
    PRIVATE
    ${src_dir}/sbepp_reader.cpp
    ${src_dir}/sbepp_cursor_reader.cpp
    ${src_dir}/raw_reader.cpp
    ${src_dir}/real_logic_reader.cpp
)

target_include_directories(${target}
    PRIVATE
    "src"
)

target_link_libraries(${target}
    PRIVATE
    compiled_benchmark_schemas
    benchmark::benchmark
    benchmark::benchmark_main
)