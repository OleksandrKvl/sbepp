function(sbeppc_error_test)
    set(options "")
    set(one_value_keywords "NAME" "ERROR")
    set(multi_value_keywords "ARGS")

    cmake_parse_arguments(
        arg
        "${options}"
        "${one_value_keywords}"
        "${multi_value_keywords}"
        "${ARGN}"
    )

    set(test_name "sbeppc_error_${arg_NAME}")

    add_test(
        NAME "${test_name}"
        COMMAND sbepp::sbeppc ${arg_ARGS}
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
    )

    set_tests_properties(
        "${test_name}"
        PROPERTIES PASS_REGULAR_EXPRESSION "${arg_ERROR}"
    )
endfunction()

# CLI errors
sbeppc_error_test(
    NAME "no_schema_name"
    ARGS "--schema-name"
    ERROR "no value for option `--schema-name`"
)
sbeppc_error_test(
    NAME "no_output_dir"
    ARGS "--output-dir"
    ERROR "no value for option `--output-dir`"
)
sbeppc_error_test(
    NAME "no_inject_include"
    ARGS "--inject-include"
    ERROR "no value for option `--inject-include`"
)
sbeppc_error_test(
    NAME "unknown_cli_arg"
    ARGS "--option"
    ERROR "unknown argument: `--option`"
)
sbeppc_error_test(
    NAME "no_filename"
    ARGS "--schema-name" "name"
    ERROR "missing filename"
)
sbeppc_error_test(
    NAME "too_many_args"
    ARGS "schema.xml" "extra_arg"
    ERROR "too many arguments"
)

# `schema_parser` errors
sbeppc_error_test(
    NAME "wrong_schema_file"
    ARGS "wrong_file_name.xml"
    ERROR "can't open file"
)

sbeppc_error_test(
    NAME "no_primitive_type"
    ARGS "no_primitive_type.xml"
    ERROR "required attribute `primitiveType` doesn't exist"
)

sbeppc_error_test(
    NAME "no_href"
    ARGS "no_href.xml"
    ERROR "required attribute `href` doesn't exist"
)

sbeppc_error_test(
    NAME "empty_href"
    ARGS "empty_href.xml"
    ERROR "`href` attribute is empty"
)

sbeppc_error_test(
    NAME "no_enum_encoding_type"
    ARGS "no_enum_encoding_type.xml"
    ERROR "required attribute `encodingType` doesn't exist"
)

sbeppc_error_test(
    NAME "empty_enum_encoding_type"
    ARGS "empty_enum_encoding_type.xml"
    ERROR "`encodingType` attribute is empty"
)

sbeppc_error_test(
    NAME "no_set_encoding_type"
    ARGS "no_set_encoding_type.xml"
    ERROR "required attribute `encodingType` doesn't exist"
)

sbeppc_error_test(
    NAME "empty_set_encoding_type"
    ARGS "empty_set_encoding_type.xml"
    ERROR "`encodingType` attribute is empty"
)

sbeppc_error_test(
    NAME "no_ref_type"
    ARGS "no_ref_type.xml"
    ERROR "required attribute `type` doesn't exist"
)

sbeppc_error_test(
    NAME "empty_ref_type"
    ARGS "empty_ref_type.xml"
    ERROR "`type` attribute is empty"
)

sbeppc_error_test(
    NAME "no_field_type"
    ARGS "no_field_type.xml"
    ERROR "required attribute `type` doesn't exist"
)

sbeppc_error_test(
    NAME "empty_field_type"
    ARGS "empty_field_type.xml"
    ERROR "`type` attribute is empty"
)

sbeppc_error_test(
    NAME "no_data_type"
    ARGS "no_data_type.xml"
    ERROR "required attribute `type` doesn't exist"
)

sbeppc_error_test(
    NAME "empty_data_type"
    ARGS "empty_data_type.xml"
    ERROR "`type` attribute is empty"
)

# get_required_name
# 3 scenarios: no name, empty name, non SBE name
function(required_name_test sbe_node)
    sbeppc_error_test(
        NAME "no_${sbe_node}_name"
        ARGS "no_${sbe_node}_name.xml"
        ERROR "required attribute `name` doesn't exist"
    )

    sbeppc_error_test(
        NAME "empty_${sbe_node}_name"
        ARGS "empty_${sbe_node}_name.xml"
        ERROR "`name` attribute is empty"
    )

    sbeppc_error_test(
        NAME "non_sbe_${sbe_node}_name"
        ARGS "non_sbe_${sbe_node}_name.xml"
        ERROR "is not a valid SBE name"
    )
endfunction()

required_name_test("type")
required_name_test("valid_value")
required_name_test("enum")

# get_required_numeric_attribute
# C++ codegen errors
