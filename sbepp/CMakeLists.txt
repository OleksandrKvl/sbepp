set(target "sbepp_sbepp")
add_library(${target} INTERFACE)
add_library(sbepp::sbepp ALIAS ${target})
set_target_properties(${target} PROPERTIES
    EXPORT_NAME sbepp
)

target_include_directories(${target}
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if (SBEPP_INSTALL)
    install(TARGETS ${target}
        EXPORT sbepp
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        DIRECTORY src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT sbepp
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sbepp
        NAMESPACE sbepp::
    )

    write_basic_package_version_file(
        "sbeppConfigVersion.cmake"
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES ${CMAKE_SOURCE_DIR}/cmake/sbeppConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sbepp
        COMPONENT sbepp
    )

    install(
        FILES "${PROJECT_BINARY_DIR}/sbepp/sbeppConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sbepp
        COMPONENT sbepp
    )
endif ()
