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