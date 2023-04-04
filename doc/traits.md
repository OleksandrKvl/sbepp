# Traits {#traits}

`sbepp` provides a way to access nearly all information provided by XML schema
and other useful properties via traits mechanism.

---

## Tags {#tags}

To access any trait you need a schema entity's *tag*. *Tag* is basically a path
to an entity. The tag for a schema itself (for `sbepp::schema_traits`) is
`<schema_name>::schema`. Tags for messages have form
`<schema_name>::schema::messages::<msg_name>` and tags for types
`<schema_name>::schema::types::<type_name>`. Here's an example:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- schema_name::schema -->
<sbe:messageSchema package="schema_name">
    <types>
        <!-- schema_name::schema::types::uint32_req -->
        <type name="uint32_req" primitiveType="uint32"/>

        <!-- schema_name::schema::types::numbers -->
        <enum name="numbers" encodingType="uint8">
            <!-- schema_name::schema::types::numbers::One -->
            <validValue name="One">1</validValue>
            <!-- schema_name::schema::types::numbers::Two -->
            <validValue name="Two">2</validValue>
        </enum>

        <!-- schema_name::schema::types::options -->
        <set name="options" encodingType="uint8">
            <!-- schema_name::schema::types::options::A -->
            <choice name="A">0</choice>
            <!-- schema_name::schema::types::options::B -->
            <choice name="B">2</choice>
        </set>

        <!-- schema_name::schema::types::groupSizeEncoding -->
        <composite name="groupSizeEncoding">
            <!-- schema_name::schema::types::groupSizeEncoding::blockLength -->
            <type name="blockLength" primitiveType="uint16"/>
            <!-- schema_name::schema::types::groupSizeEncoding::numInGroup -->
            <type name="numInGroup" primitiveType="uint16"/>
        </composite>
    </types>

    <!-- schema_name::schema::messages::msg -->
    <sbe:message name="msg" id="1">
        <!-- schema_name::schema::messages::msg::field -->
        <field name="field" id="1" type="uint32"/>

        <!-- schema_name::schema::messages::msg::group -->
        <group name="group" id="2">
            <!-- schema_name::schema::messages::msg::group::field -->
            <field name="field" id="1" type="uint32"/>
        </group>

        <!-- schema_name::schema::messages::msg::data -->
        <data name="data" id="3" type="varDataEncoding"/>
    </sbe:message>
</sbe:messageSchema>
```

For built-in types like `sbepp::char_t`, the type itself works as a tag, e.g.

```cpp
// get maxValue of a built-in required `char` type
auto max_char = sbepp::type_traits<sbepp::char_t>::max_value();
```

---

## Using traits

Similar to `std::numeric_limits`, `sbepp` traits are accessed like
`trait_name<Tag>::value()`:

```cpp
auto schema_version = sbepp::schema_traits<schema_name::schema>::version();
auto null_value = sbepp::type_traits<schema_name::schema::types::optional>::null_value();
```

For the list of available traits see @ref traits-list.