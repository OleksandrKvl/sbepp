function(add_naming_test_schema schema schema_list_var)
    sbeppc_compile_schema(
        TARGET_NAME compiled_naming_test_schemas
        SCHEMA_FILE "${CMAKE_CURRENT_LIST_DIR}/${schema}.xml"
        SCHEMA_NAME "${schema}"
    )

    list(APPEND ${schema_list_var} "${schema}")
    set(${schema_list_var} "${${schema_list_var}}" PARENT_SCOPE)
endfunction()

function(create_naming_test_target target_name schemas)
    add_executable(${target_name})

    set(includes)

    foreach(schema IN LISTS schemas)
        string(APPEND includes "#include <${schema}/${schema}.hpp>\n")
    endforeach()

    configure_file("main.cpp.in" "main.cpp" @ONLY)

    target_sources(${target_name}
        PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/main.cpp"
    )

    target_link_libraries(${target_name}
        PRIVATE
        compiled_naming_test_schemas
    )
endfunction()

# ------------------------------------------------------------------------------

set(naming_test_schemas)

add_naming_test_schema("type_member_name_clash" "naming_test_schemas")
add_naming_test_schema("public_private_type_name_clash" "naming_test_schemas")
add_naming_test_schema("mangled_type_name_clash" "naming_test_schemas")
add_naming_test_schema("types_type" "naming_test_schemas")

add_naming_test_schema("message_member_name_clash" "naming_test_schemas")
add_naming_test_schema("mangled_message_name_clash" "naming_test_schemas")
add_naming_test_schema("messages_message" "naming_test_schemas")
add_naming_test_schema("mangled_message_header_name" "naming_test_schemas")

set(target_name "naming_test")

create_naming_test_target("${target_name}" "${naming_test_schemas}")

add_test(
    NAME "${target_name}"
    COMMAND "${target_name}"
)