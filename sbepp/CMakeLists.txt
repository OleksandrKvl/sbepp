set(target "sbepp_sbepp")
add_library(${target} INTERFACE)
add_library(sbepp::sbepp ALIAS ${target})
set_target_properties(${target} PROPERTIES
    OUTPUT_NAME sbepp
    EXPORT_NAME sbepp
)

target_include_directories(${target}
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS ${target}
    EXPORT sbepp
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
