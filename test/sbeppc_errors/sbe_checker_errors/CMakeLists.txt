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
    ERROR "enum `numbers_enum` doesn't have valid value `One`"
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

sbeppc_error_test(
    NAME "wrong_char_const_length"
    ERROR "constant length \\(3\\) is greater than `length` attribute \\(1\\)"
)

function(valid_value_test type value)
    sbeppc_error_test(
        NAME "wrong_valid_value_${type}"
        ERROR "value `${value}` cannot be represented by type `${type}`"
    )
endfunction()

valid_value_test("char" "not_char")
valid_value_test("int8" "128")
valid_value_test("uint8" "-1")
valid_value_test("int16" "32768")
valid_value_test("uint16" "-1")
valid_value_test("int32" "2147483648")
valid_value_test("uint32" "-1")
valid_value_test("int64" "9223372036854775808")
valid_value_test("uint64" "-1")

sbeppc_error_test(
    NAME "wrong_enum_encoding"
    ERROR "enum type should be `char` or integer, got `float`"
)

sbeppc_error_test(
    NAME "wrong_value_ref_value"
    ERROR "valueRef `numbers_enum.Two` \\(255\\) cannot be represented by type `int8`"
)

sbeppc_error_test(
    NAME "wrong_numeric_constant_value"
    ERROR "value `255` cannot be represented by type `int8`"
)

function(offset_test encoding)
    sbeppc_error_test(
        NAME "bad_${encoding}_offset"
        ERROR "custom offset \\(4\\) is less than minimum possible \\(8\\)"
    )
endfunction()

offset_test("type")
offset_test("enum")
offset_test("set")
offset_test("composite")
offset_test("ref")