list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

add_subdirectory(cli_errors)
add_subdirectory(schema_parser_errors)
add_subdirectory(sbe_checker_errors)
# C++ codegen errors
