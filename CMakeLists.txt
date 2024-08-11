cmake_minimum_required(VERSION 3.11)

project(sbepp
    VERSION 1.4.1
    LANGUAGES CXX
)

include(CTest)
include(CMakeDependentOption)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(utils)

sbepp_in_source_build_guard()
sbepp_set_language_standard(CXX 17)
sbepp_set_default_build_type(Release)

option(SBEPP_DEV_MODE "Developer mode" OFF)

cmake_dependent_option(SBEPP_BUILD_TESTS
    "Build sbepp tests" OFF
    "SBEPP_DEV_MODE;BUILD_TESTING;CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR"
    OFF
)

cmake_dependent_option(SBEPP_BUILD_BENCHMARK
    "Build sbepp benchmark" OFF
    "SBEPP_DEV_MODE;CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF
)

option(SBEPP_BUILD_SBEPPC "Build sbeppc schema compiler" ON)
option(SBEPP_BUILD_DOCS "Build documentation" OFF)

add_subdirectory(sbepp)

if(SBEPP_BUILD_SBEPPC)
    add_subdirectory(sbeppc)
endif()

if(SBEPP_BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()

if(SBEPP_BUILD_TESTS)
    add_subdirectory(test)
endif()

if(SBEPP_BUILD_DOCS)
    add_subdirectory(doc)
endif()

install(EXPORT sbepp
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sbepp
    NAMESPACE sbepp::
)

write_basic_package_version_file(
    "sbeppConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

install(
    FILES cmake/sbeppConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sbepp
    COMPONENT sbepp
)

install(
    FILES "${PROJECT_BINARY_DIR}/sbeppConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sbepp
    COMPONENT sbepp
)

install(
    FILES cmake/sbeppcHelpers.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sbepp
)
