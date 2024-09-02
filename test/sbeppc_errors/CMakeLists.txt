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
    )

    set_tests_properties(
        "${test_name}"
        PROPERTIES PASS_REGULAR_EXPRESSION "${arg_ERROR}"
    )
endfunction()

# CLI error
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
# schema_parser errors
sbeppc_error_test(
    NAME "wrong_schema_file"
    ARGS "wrong_file_name.xml"
    ERROR "can't open file"
)
# C++ codegen errors
