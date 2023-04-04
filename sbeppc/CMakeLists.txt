find_package(Git REQUIRED)
find_package(fmt 9.1.0 REQUIRED)
find_package(pugixml 1.12.1 REQUIRED)

set(target "sbepp_sbeppc")
add_executable(${target})
add_executable(sbepp::sbeppc ALIAS ${target})
set_target_properties(${target} PROPERTIES
    OUTPUT_NAME sbeppc
    EXPORT_NAME sbeppc
)

set(src_dir src/sbepp/sbeppc)

execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    RESULT_VARIABLE result
    OUTPUT_VARIABLE ${PROJECT_NAME}_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(result)
    message(FATAL_ERROR "Failed to get git hash: ${result}")
endif()

configure_file(${src_dir}/build_info.cpp.in build_info.cpp @ONLY)

target_sources(${target}
    PRIVATE
    ${src_dir}/main.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/build_info.cpp
)

target_include_directories(${target} PRIVATE src)

target_link_libraries(${target}
    PRIVATE
    sbepp::sbepp
    fmt::fmt
    pugixml::pugixml
)

sbepp_set_strict_warning_options(${target})

install(TARGETS ${target}
    EXPORT sbepp
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)