include(sbeppc_error_test)

sbeppc_error_test(
    NAME "wrong_ref_type"
    ERROR "encoding `some_type` doesn't exist"
)

sbeppc_error_test(
    NAME "cyclic_refs"
    ERROR "cyclic reference detected while processing encoding `composite`"
)

sbeppc_error_test(
    NAME "invalid_value_ref"
    ERROR "`wrong-ref` is not a valid valueRef"
)

sbeppc_error_test(
    NAME "no_value_ref_encoding"
    ERROR "encoding `numbers_enum` doesn't exist"
)

sbeppc_error_test(
    NAME "wrong_value_ref_encoding"
    ERROR "encoding `numbers_enum` is not an enum"
)

sbeppc_error_test(
    NAME "no_value_ref_enumerator"
    ERROR "enum `numbers_enum` doesn't have `One` enumerator"
)