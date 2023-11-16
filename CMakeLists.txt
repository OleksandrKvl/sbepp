cmake_minimum_required(VERSION 3.11)

project(sbepp
    VERSION 1.1.0
    LANGUAGES CXX
)

include(CTest)
include(CMakeDependentOption)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

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
    find_package(fmt QUIET)
    if (NOT fmt_FOUND)
        FetchContent_Declare(fmt
          GIT_REPOSITORY https://github.com/fmtlib/fmt.git
          GIT_TAG 10.1.1
        )
        FetchContent_MakeAvailable(fmt)
    endif ()

    find_package(pugixml QUIET)
    if (NOT pugixml_FOUND)
        FetchContent_Declare(
          pugixml
          GIT_REPOSITORY https://github.com/zeux/pugixml.git
          GIT_TAG v1.14
        )
        FetchContent_MakeAvailable(pugixml)
    endif()

    add_subdirectory(sbeppc)
endif()

if(SBEPP_BUILD_BENCHMARK)
    find_package(benchmark QUIET)
    if (NOT GTest_FOUND)
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
            cmake_policy(SET CMP0135 NEW)
        endif()

        FetchContent_Declare(
          benchmark
          URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.tar.gz
        )

        set(INSTALL_BENCHMARK OFF)
        set(BENCHMARK_USE_BUNDLED_GTEST OFF)
        FetchContent_MakeAvailable(benchmark)
    endif ()

    add_subdirectory(benchmark)
endif()

if(SBEPP_BUILD_TESTS)
    enable_testing()
    message(STATUS "Tests are enabled")

    find_package(GTest QUIET)
    if (NOT GTest_FOUND)
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
            cmake_policy(SET CMP0135 NEW)
        endif()

        FetchContent_Declare(
          googletest
          URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
        )

        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF)
        FetchContent_MakeAvailable(googletest)
    endif ()

    add_subdirectory(test)
endif()

if(SBEPP_BUILD_DOCS)
    add_subdirectory(doc)
endif()
