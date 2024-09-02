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

# 2 scenarios: no attribute, empty attribute
function(non_empty_attribute_test name attribute)
    sbeppc_error_test(
        NAME "no_${name}"
        ARGS "no_${name}.xml"
        ERROR "required attribute `${attribute}` doesn't exist"
    )

    sbeppc_error_test(
        NAME "empty_${name}"
        ARGS "empty_${name}.xml"
        ERROR "`${attribute}` attribute is empty"
    )
endfunction()

non_empty_attribute_test("href" "href")
non_empty_attribute_test("enum_encoding_type" "encodingType")
non_empty_attribute_test("set_encoding_type" "encodingType")
non_empty_attribute_test("ref_type" "type")
non_empty_attribute_test("field_type" "type")
non_empty_attribute_test("data_type" "type")

# 3 scenarios: no name, empty name, non-SBE name
function(required_name_test sbe_node)
    non_empty_attribute_test("${sbe_node}_name" "name")

    sbeppc_error_test(
        NAME "non_sbe_${sbe_node}_name"
        ARGS "non_sbe_${sbe_node}_name.xml"
        ERROR "is not a valid SBE name"
    )
endfunction()

required_name_test("type")
required_name_test("valid_value")
required_name_test("enum")
required_name_test("choice")
required_name_test("set")
required_name_test("ref")
required_name_test("composite")
required_name_test("field")
required_name_test("group")
required_name_test("data")
required_name_test("message")

# 4 scenarios: no id, empty id, out-of-range id, text id
function(numeric_attribute_test name attribute)
    non_empty_attribute_test("${name}" "${attribute}")

    sbeppc_error_test(
        NAME "out_of_range_${name}"
        ARGS "out_of_range_${name}.xml"
        ERROR "cannot convert `${attribute}` value \\(-1\\) to its underlying numeric type"
    )

    sbeppc_error_test(
        NAME "text_${name}"
        ARGS "text_${name}.xml"
        ERROR "cannot convert `${attribute}` value \\(text\\) to its underlying numeric type"
    )
endfunction()

function(id_test name)
    numeric_attribute_test("${name}_id" "id")
endfunction()


id_test("message")
id_test("field")
id_test("group")
id_test("data")
id_test("schema")

numeric_attribute_test("schema_version" "version")

# C++ codegen errors
