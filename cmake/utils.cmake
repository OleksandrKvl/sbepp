include_guard(GLOBAL)

# sets required standard without extensions
macro(sbepp_set_language_standard language standard)
    set(CMAKE_${language}_STANDARD ${standard})
    set(CMAKE_${language}_STANDARD_REQUIRED YES)
    set(CMAKE_${language}_EXTENSIONS NO)
endmacro()

# sets required standard without extensions for a target
macro(sbepp_set_target_language_standard target language standard)
    set_target_properties(${target} PROPERTIES ${language}_STANDARD ${standard})
    set_target_properties(${target} PROPERTIES ${language}_STANDARD_REQUIRED
        YES
    )
    set_target_properties(${target} PROPERTIES ${language}_EXTENSIONS NO)
endmacro()

# disallows build in source directory
function(sbepp_in_source_build_guard)
    if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
        message(FATAL_ERROR "In-source builds are not allowed.")
    endif()
endfunction()

# sets build type that is used in absence of CMAKE_BUILD_TYPE
# has no effect on multi-configuration generators
macro(sbepp_set_default_build_type type)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE ${type})
    endif()
endmacro()

function(sbepp_set_strict_warning_options target)
    if(SBEPP_DEV_MODE)
        if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
            target_compile_options(
                    ${target} PRIVATE -Wall -Wextra -Wpedantic -Werror
            )
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            target_compile_options(${target} PRIVATE /W3 /WX)
        endif()
    endif()
endfunction()
