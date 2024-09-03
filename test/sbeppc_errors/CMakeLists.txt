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

    # for most test cases, test case and its schema file have the same name
    if(NOT arg_ARGS)
        set(arg_ARGS "${arg_NAME}.xml")
    endif()

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
    NAME "wrong_file_name"
    ERROR "can't open file"
)

sbeppc_error_test(
    NAME "no_primitive_type"
    ERROR "required attribute `primitiveType` doesn't exist"
)

# TODO: add helper that uses the same test and schema name

# 2 scenarios: no attribute, empty attribute
function(non_empty_attribute_test name attribute)
    sbeppc_error_test(
        NAME "no_${name}"
        ERROR "required attribute `${attribute}` doesn't exist"
    )

    sbeppc_error_test(
        NAME "empty_${name}"
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

function(numeric_conversion_test name attribute)
    sbeppc_error_test(
        NAME "out_of_range_${name}"
        ERROR "cannot convert `${attribute}` value \\(-1\\) to its underlying numeric type"
    )

    sbeppc_error_test(
        NAME "text_${name}"
        ERROR "cannot convert `${attribute}` value \\(text\\) to its underlying numeric type"
    )
endfunction()

# 4 scenarios: no id, empty id, out-of-range id, text id
function(required_numeric_attribute_test name attribute)
    non_empty_attribute_test("${name}" "${attribute}")
    numeric_conversion_test("${name}" "${attribute}")
endfunction()

function(id_test name)
    required_numeric_attribute_test("${name}_id" "id")
endfunction()


id_test("message")
id_test("field")
id_test("group")
id_test("data")
id_test("schema")

required_numeric_attribute_test("schema_version" "version")

sbeppc_error_test(
    NAME "duplicate_encoding_name"
    ERROR "encoding `type_name` already exists"
)

sbeppc_error_test(
    NAME "duplicate_message_name"
    ERROR "message with name `msg_name` already exists"
)

sbeppc_error_test(
    NAME "duplicate_message_id"
    ERROR "message with id `1` already exists"
)

sbeppc_error_test(
    NAME "wrong_type_presence"
    ERROR "wrong presence token `wrong`"
)

sbeppc_error_test(
    NAME "wrong_field_presence"
    ERROR "wrong presence token `wrong`"
)

sbeppc_error_test(
    NAME "wrong_primitive_type"
    ERROR "primitiveType `uint33` is not a valid primitive type"
)

sbeppc_error_test(
    NAME "no_constant_value"
    ERROR "either `valueRef` or value must be provided for constant `constant`"
)

sbeppc_error_test(
    NAME "no_valid_value_content"
    ERROR "required node content is empty"
)

sbeppc_error_test(
    NAME "no_choice_content"
    ERROR "required node content is empty"
)

sbeppc_error_test(
    NAME "duplicate_valid_value_name"
    ERROR "duplicate validValue name: `A`"
)

sbeppc_error_test(
    NAME "duplicate_choice_name"
    ERROR "duplicate choice name: `A`"
)

sbeppc_error_test(
    NAME "out_of_range_choice_index"
    ERROR "node's value `-1` doesn't represent choice_index_t"
)

sbeppc_error_test(
    NAME "text_choice_index"
    ERROR "node's value `text` doesn't represent choice_index_t"
)

sbeppc_error_test(
    NAME "duplicate_composite_element"
    ERROR "duplicate composite element: `A`"
)

# get_optional_numeric_attribute, use `numeric_conversion_test`
function(offset_test name)
    numeric_conversion_test("${name}_offset" "offset")
endfunction()

offset_test("type")
offset_test("enum")
offset_test("set")
offset_test("ref")
offset_test("composite")
offset_test("field")

function(version_test name)
    numeric_conversion_test("${name}_depr_version" "deprecated")
    numeric_conversion_test("${name}_since_version" "sinceVersion")
endfunction()

version_test("type")
version_test("valid_value")
version_test("enum")
version_test("choice")
version_test("set")
version_test("ref")
version_test("composite")
version_test("field")
version_test("group")
version_test("data")
version_test("message")

numeric_conversion_test("type_length" "length")
numeric_conversion_test("message_block_length" "blockLength")
numeric_conversion_test("group_block_length" "blockLength")

# TODO: continue from 19th occurence of "throw"

# C++ codegen errors
