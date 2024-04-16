# Stringification primitives {#stringification}

@deprecated Functionality described on this page is deprecated in favor of more
generic `sbepp::visit`. You can find up-to-date stringification example
[here](#stringification-example).

Instead of providing fully-fledged stringification mechanism out of the box,
`sbepp` provides only low-level primitives which in pair with
[Visit API](#visit-api) can be used to build `to_string()`-like routine that
uses a specific formatting/output mechanisms.  
For most field types their raw value is enough to produce human-readable
representation. The only exceptions are enums, for which we need enumerator's
name, and sets, for which we need choice names. These two cases are covered by
`sbepp::enum_to_string()` and `sbepp::visit_set()`.