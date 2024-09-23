include(sbeppc_error_test)

sbeppc_error_test(
    NAME "wrong_ref_type"
    ERROR "encoding `some_type` doesn't exist"
)

sbeppc_error_test(
    NAME "cyclic_refs"
    ERROR "cyclic reference detected while processing encoding `composite`"
)