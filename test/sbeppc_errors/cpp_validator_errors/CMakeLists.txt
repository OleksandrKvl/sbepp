include(sbeppc_error_test)

function(bad_cpp_name_test kind)
    sbeppc_error_test(
        NAME "non_cpp_${kind}_name"
        ERROR "`constexpr` is not a valid C\\+\\+ name"
    )
endfunction()

bad_cpp_name_test("type")
bad_cpp_name_test("enum")
bad_cpp_name_test("valid_value")
bad_cpp_name_test("set")
bad_cpp_name_test("choice")
bad_cpp_name_test("composite")
bad_cpp_name_test("ref")
bad_cpp_name_test("field")
bad_cpp_name_test("group")
bad_cpp_name_test("data")
bad_cpp_name_test("message")

set(bad_schema_name_error "schema name `constexpr` is not a valid C\\+\\+ namespace. Change `messageSchema.package` attribute or provide a custom name using `--schema-name` sbeppc option")

sbeppc_error_test(
    NAME "non_cpp_schema_name"
    ERROR "${bad_schema_name_error}"
)

sbeppc_error_test(
    NAME "non_cpp_schema_name2"
    ERROR "${bad_schema_name_error}"
    ARGS "--schema-name" "constexpr" "non_cpp_schema_name.xml"
)

sbeppc_error_test(
    NAME "non_cpp_schema_name3"
    ERROR "schema name `non-SBE name` is not a valid C\\+\\+ namespace. Change `messageSchema.package` attribute or provide a custom name using `--schema-name` sbeppc option"
    ARGS "--schema-name" "non-SBE name" "non_cpp_schema_name.xml"
)

sbeppc_error_test(
    NAME "non_cpp_schema_name4"
    ERROR "schema name `std` is not a valid C\\+\\+ namespace. Change `messageSchema.package` attribute or provide a custom name using `--schema-name` sbeppc option"
    ARGS "--schema-name" "std" "non_cpp_schema_name.xml"
)