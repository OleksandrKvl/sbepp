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

sbeppc_error_test(
    NAME "no_enum_encoding"
    ERROR "encoding `wrong_type` doesn't exist"
)

sbeppc_error_test(
    NAME "non_type_enum_encoding"
    ERROR "encoding `composite` is not a type"
)

sbeppc_error_test(
    NAME "no_set_encoding"
    ERROR "encoding `wrong_type` doesn't exist"
)

sbeppc_error_test(
    NAME "non_type_set_encoding"
    ERROR "encoding `composite` is not a type"
)

sbeppc_error_test(
    NAME "non_unsigned_set_encoding1"
    ERROR "underlying type must be unsigned"
)

sbeppc_error_test(
    NAME "non_unsigned_set_encoding2"
    ERROR "underlying type must be unsigned"
)

sbeppc_error_test(
    NAME "out_of_range_choice_index1"
    ERROR "choice index `8` is out of valid range \\(\\[0, 7\\]\\)"
)

sbeppc_error_test(
    NAME "out_of_range_choice_index2"
    ERROR "choice index `8` is out of valid range \\(\\[0, 7\\]\\)"
)