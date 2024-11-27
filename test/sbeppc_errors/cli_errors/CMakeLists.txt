include(sbeppc_error_test)

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