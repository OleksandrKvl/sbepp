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

sbeppc_error_test(
    NAME "array_enum_encoding"
    ERROR "encoding type `arr` must have length equal to 1, got `8`"
)

sbeppc_error_test(
    NAME "array_set_encoding"
    ERROR "encoding type `arr` must have length equal to 1, got `8`"
)

sbeppc_error_test(
    NAME "wrong_value_ref_constant_length"
    ERROR "non-char constant length must be equal to 1, got `2`"
)

sbeppc_error_test(
    NAME "wrong_numeric_constant_length"
    ERROR "non-char constant length must be equal to 1, got `2`"
)

sbeppc_error_test(
    NAME "no_message_header"
    ERROR "message header encoding `messageHeader` doesn't exist"
)

sbeppc_error_test(
    NAME "non_composite_message_header"
    ERROR "message header encoding `messageHeader` is not a composite"
)

function(level_header_element_test level header_name element)
    sbeppc_error_test(
        NAME "no_${level}_header_${element}"
        ERROR "${level} header `${header_name}` doesn't have required `${element}` element"
    )

    sbeppc_error_test(
        NAME "wrong_${level}_header_${element}_kind"
        ERROR "${level} header element `${element}` must be a type or a ref"
    )

    sbeppc_error_test(
        NAME "non_type_${level}_header_${element}_ref"
        ERROR "${level} header element `${element}` must refer to a type"
    )

    sbeppc_error_test(
        NAME "array_${level}_header_${element}"
        ERROR "${level} header element `${element}` must be a non-array type"
    )

    sbeppc_error_test(
        NAME "array_${level}_header_${element}_ref"
        ERROR "${level} header element `${element}` must be a non-array type"
    )

    sbeppc_error_test(
        NAME "constant_${level}_header_${element}"
        ERROR "${level} header element `${element}` cannot be a constant"
    )

    sbeppc_error_test(
        NAME "constant_${level}_header_${element}_ref"
        ERROR "${level} header element `${element}` cannot be a constant"
    )
endfunction()

function(message_header_element_test element)
    level_header_element_test("message" "messageHeader" "${element}")
endfunction()

message_header_element_test("schemaId")
message_header_element_test("templateId")
message_header_element_test("version")
message_header_element_test("blockLength")

sbeppc_error_test(
    NAME "bad_field_type"
    ERROR "field type `some_type` doesn't exist"
)

sbeppc_error_test(
    NAME "bad_group_field_type"
    ERROR "field type `some_type` doesn't exist"
)

sbeppc_error_test(
    NAME "no_group_header"
    ERROR "group header encoding `groupSizeEncoding` doesn't exist"
)

sbeppc_error_test(
    NAME "non_composite_group_header"
    ERROR "group header encoding `groupSizeEncoding` is not a composite"
)

function(group_header_element_test element)
    level_header_element_test("group" "groupSizeEncoding" "${element}")
endfunction()

group_header_element_test("blockLength")
group_header_element_test("numInGroup")

sbeppc_error_test(
    NAME "bad_message_field_offset"
    ERROR "custom offset \\(5\\) is less than minimum possible \\(6\\)"
)

sbeppc_error_test(
    NAME "bad_group_field_offset"
    ERROR "custom offset \\(5\\) is less than minimum possible \\(6\\)"
)

sbeppc_error_test(
    NAME "bad_message_blockLength"
    ERROR "custom `blockLength` \\(5\\) is less than minimum possible \\(6\\)"
)

sbeppc_error_test(
    NAME "bad_group_blockLength"
    ERROR "custom `blockLength` \\(5\\) is less than minimum possible \\(6\\)"
)

sbeppc_error_test(
    NAME "non_single_byte_array"
    ERROR "arrays must have a single-byte type"
)

sbeppc_error_test(
    NAME "no_data_header"
    ERROR "data header encoding `varDataEncoding` doesn't exist"
)

sbeppc_error_test(
    NAME "non_composite_data_header"
    ERROR "data header encoding `varDataEncoding` is not a composite"
)

level_header_element_test("data" "varDataEncoding" "length")

sbeppc_error_test(
    NAME "no_data_header_varData"
    ERROR "data header `varDataEncoding` doesn't have required `varData` element"
)

sbeppc_error_test(
    NAME "wrong_data_header_varData_kind"
    ERROR "data header element `varData` must be a type or a ref"
)

sbeppc_error_test(
    NAME "non_type_data_header_varData_ref"
    ERROR "data header element `varData` must refer to a type"
)

sbeppc_error_test(
    NAME "non_zero_length_data_header_varData"
    ERROR "data header element `varData` must have length equal to 0"
)

sbeppc_error_test(
    NAME "non_zero_length_data_header_varData_ref"
    ERROR "data header element `varData` must have length equal to 0"
)

sbeppc_error_test(
    NAME "no_field_valueRef"
    ERROR "field constant must have `valueRef`"
)

sbeppc_error_test(
    NAME "invalid_field_valueRef1"
    ERROR "`bad_ref` is not a valid `valueRef`"
)

sbeppc_error_test(
    NAME "invalid_field_valueRef2"
    ERROR "`abc.` is not a valid `valueRef`"
)

sbeppc_error_test(
    NAME "invalid_field_valueRef3"
    ERROR "`.abc` is not a valid `valueRef`"
)

sbeppc_error_test(
    NAME "no_field_valueRef_encoding"
    ERROR "encoding `abc` doesn't exist"
)

sbeppc_error_test(
    NAME "non_enum_field_valueRef_encoding"
    ERROR "encoding `messageHeader` is not an enum"
)

sbeppc_error_test(
    NAME "no_field_valueRef_valid_value"
    ERROR "enum `options` doesn't have valid value `def`"
)

sbeppc_error_test(
    NAME "out_of_range_field_valueRef_valid_value"
    ERROR "valueRef `options.a` \\(256\\) cannot be represented by type `uint8`"
)

sbeppc_error_test(
    NAME "composite_field_constant"
    ERROR "composite field can't be a constant"
)

sbeppc_error_test(
    NAME "no_enum_field_valueRef"
    ERROR "field constant must have `valueRef`"
)

sbeppc_error_test(
    NAME "invalid_enum_field_valueRef1"
    ERROR "`bad_ref` is not a valid `valueRef`"
)

sbeppc_error_test(
    NAME "invalid_enum_field_valueRef2"
    ERROR "`abc.` is not a valid `valueRef`"
)

sbeppc_error_test(
    NAME "invalid_enum_field_valueRef3"
    ERROR "`.abc` is not a valid `valueRef`"
)

sbeppc_error_test(
    NAME "no_enum_field_valueRef_encoding"
    ERROR "encoding `abc` doesn't exist"
)

sbeppc_error_test(
    NAME "non_enum_enum_field_valueRef_encoding"
    ERROR "encoding `messageHeader` is not an enum"
)

sbeppc_error_test(
    NAME "no_enum_field_valueRef_valid_value"
    ERROR "enum `options` doesn't have valid value `def`"
)

sbeppc_error_test(
    NAME "wrong_enum_field_valueRef_enum"
    ERROR "enum constant type `some_enum` should match field type `options`"
)

function(non_sbe_name_test sbe_node)
    sbeppc_error_test(
        NAME "non_sbe_${sbe_node}_name"
        ERROR "`non-SBE name` is not a valid SBE name"
    )
endfunction()

non_sbe_name_test("type")
non_sbe_name_test("valid_value")
non_sbe_name_test("enum")
non_sbe_name_test("choice")
non_sbe_name_test("set")
non_sbe_name_test("ref")
non_sbe_name_test("composite")
non_sbe_name_test("field")
non_sbe_name_test("group")
non_sbe_name_test("data")
non_sbe_name_test("message")

sbeppc_error_test(
    NAME "wrong_primitive_type"
    ERROR "primitiveType `uint33` is not a valid primitive type"
)

sbeppc_error_test(
    NAME "no_constant_value"
    ERROR "either `valueRef` or value must be provided for constant `constant`"
)