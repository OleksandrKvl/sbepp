# Examples {#examples}
[TOC]

This pages shows some simple examples, most of things they demostrate are also
shown on other pages.

Here's the schema:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<sbe:messageSchema package="market" id="1" version="0" byteOrder="littleEndian">
    <types>
        <composite name="messageHeader">
            <type name="blockLength" primitiveType="uint16"/>
            <type name="templateId" primitiveType="uint16"/>
            <type name="schemaId" primitiveType="uint16"/>
            <type name="version" primitiveType="uint16"/>
        </composite>

        <composite name="groupSizeEncoding">
            <type name="blockLength" primitiveType="uint16"/>
            <type name="numInGroup" primitiveType="uint16"/>
        </composite>

        <composite name="varDataEncoding">
            <type name="length" primitiveType="uint32"/>
            <type name="varData" primitiveType="uint8" length="0"/>
        </composite>

        <type name="uint32_opt" primitiveType="uint32" presence="optional"/>

        <enum name="numbers" encodingType="uint8">
            <validValue name="One">1</validValue>
            <validValue name="Two">2</validValue>
        </enum>

        <set name="options" encodingType="uint8">
            <choice name="A">0</choice>
            <choice name="B">2</choice>
        </set>
    </types>

    <sbe:message name="msg" id="1">
        <field name="field" id="1" type="uint32_opt"/>
        <field name="number" id="2" type="numbers"/>
        <field name="option" id="3" type="options"/>

        <group name="group" id="4">
            <field name="field" id="1" type="uint32"/>
        </group>

        <data name="data" id="5" type="varDataEncoding"/>
    </sbe:message>
</sbe:messageSchema>
```

---

## Encoding a message using normal accessors

```cpp
// note: it's up to client to provide sufficient buffer
std::array<char, 1024> buf{};

auto m = sbepp::make_view<market::messages::msg>(buf.data(), buf.size());
sbepp::fill_message_header(m);

// note: order doesn't matter, it's mixed for demo purpose
m.option(market::types::options{}.B(true));
m.field(3);
m.number(market::types::numbers::Two);

auto d = m.data();
d.assign_string("hi!");

auto g = m.group();
sbepp::fill_group_header(g, 0);
g.resize(2);
for(const auto entry : g)
{
    entry.field(1);
}

// note: size_bytes is O(1) in this case because group is flat
const auto msg_size = sbepp::size_bytes(m);
send(sbepp::addressof(m), msg_size);
```

---

## Encoding a message using cursor-based accessors

```cpp
// note: it's up to client to provide sufficient buffer
std::array<char, 1024> buf{};

auto m = sbepp::make_view<market::messages::msg>(buf.data(), buf.size());
auto c = sbepp::init_cursor(m);
sbepp::fill_message_header(m);

// note: order must be preserved for cursor-based accessors
m.field(1, c);
m.number(market::types::numbers::Two, c);
m.option(market::types::options{}.B(true), c);

auto g = m.group(c);
sbepp::fill_group_header(g, 2);
for(const auto entry : g.cursor_range(c))
{
    entry.field(1, c);
}

// note: don't move cursor yet
auto d = m.data(sbepp::cursor_ops::dont_move(c));
d.assign_string("hi!");
// ok, now just skip it
m.data(sbepp::cursor_ops::skip(c));

// size_bytes here is a no-op because cursor is already at the end
const auto msg_size = sbepp::size_bytes(m, c);
send(sbepp::addressof(m), msg_size);
```

---

## Decoding a message using normal accessors

```cpp
std::array<char, 1024> buf{};
auto header = sbepp::make_const_view<
    sbepp::schema_traits<market::schema>::header_type>(
        buf.data(), buf.size());
if(*header.templateId()
    != sbepp::message_traits<market::schema::messages::msg>::id())
{
    std::cerr << "unknown message id: " << *header.templateId() << '\n'; 
    return;
}

auto m = sbepp::make_const_view<market::messages::msg>(buf.data(), buf.size());

// let's pretend we got this buffer from an untrusted network and want to be
// sure that the message is fully contained within given buffer.
auto checked_size = sbepp::size_bytes_checked(m, buf.size());
if(!checked_size.valid)
{
    std::cerr << "bad message\n";
    return;
}

// note: order doesn't matter, it's mixed for demo purpose
auto d = m.data();
std::cout.write(d.data(), d.size());

const auto field = m.field();
if(field.has_value())
{
    if(field.in_range())
    {
        std::cout << *field << '\n';
    }
    else
    {
        std::cout << "field value is out of range \n";
    }
}
else
{
    std::cout << "field is null\n";
}

std::cout << *m.option() << '\n';
std::cout << sbepp::to_underlying(m.number()) << '\n';

auto g = m.group();
std::cout << "group size: " << g.size() << '\n';
for(const auto entry : g)
{
    std::cout << *entry.field() << '\n';
}
```

---

## Decoding a message using cursor-based accessors

```cpp
std::array<char, 1024> buf{};
auto m = sbepp::make_const_view<market::messages::msg>(buf.data(), buf.size());
// there's nothing wrong in creation of a "wrong" message view
if(*sbepp::get_header(m).templateId()
    != sbepp::message_traits<market::schema::messages::msg>::id())
{
    std::cerr << "not a `market::messages::msg`" << '\n'; 
    return;
}

auto c = sbepp::init_cursor(m);

// note: order must be preserved for cursor-based accessors
const auto field = m.field(c);
if(field.has_value())
{
    if(field.in_range())
    {
        std::cout << *field << '\n';
    }
    else
    {
        std::cout << "field value is out of range \n";
    }
}
else
{
    std::cout << "field is null\n";
}

std::cout << sbepp::to_underlying(m.number(c)) << '\n';
std::cout << *m.option(c) << '\n';

auto g = m.group(c);
std::cout << "group size: " << g.size() << '\n';
for(const auto entry : g.cursor_range(c))
{
    std::cout << *entry.field(c) << '\n';
}

auto d = m.data(c);
std::cout.write(d.data(), d.size());
```

---

## Decoding message header

There are couple of ways to decode message header from an incoming data.

1. Using `sbepp::schema_traits::header_type` to get header type:

    ```cpp
    // `make_view` works as well
    auto header = sbepp::make_const_view<
        sbepp::schema_traits<market::schema>::header_type>(
            buf.data(), buf.size());
    ```

2. Using hardcoded header type(`messageHeader` here):

    ```cpp
    auto header = sbepp::make_const_view<market::types::messageHeader>(
        buf.data(), buf.size());
    ```

3. And finally, when one knows for sure what schema they're working with, it's
legal to create any message view from that schema and use `sbepp::get_header()`
to get the header. This works because header is the same for all messages within
the schema. Of course access anything beyond the header still requires message
type check.

    ```cpp
    // works even if `buf` represents a different message from `market` schema
    auto m = sbepp::make_const_view<market::messages::msg>(
        buf.data(), buf.size());
    auto header = sbepp::get_header(m);
    ```

---

## Estimating buffer size to encode a message {#example-size-bytes}

It is possible to estimate how much memory is required to represent encoded
message using `sbepp::message_traits::size_bytes()`. To do this, one needs to
know message structure details, the number of entries for each group and the
total size of `<data>` element(s) payload. Here's how it can be done at
compile-time:

```cpp
// specify max parameters
constexpr auto max_group_entries = 5;
constexpr auto max_data_size = 256;

constexpr auto max_msg_size = sbepp::message_traits<
    market::schema::messages:msg>::size_bytes(
        max_group_entries, max_data_size);

std::array<char, max_msg_size> buf{};

auto m = sbepp::make_view<market::messages::msg>(buf.data(), buf.size());
// encode message as usual but be sure not to exceed `max_group_entries` and
// `max_data_size`
```

or at run-time:

```cpp
void make_msg(
    const std::vector<std::uint32_t>& group_entries,
    const std::vector<std::uint8_t>& data_payload)
{
    // calculate buffer size required to store `group_entries` in `msg::group`
    // and `data_payload` in `msg::data`
    const auto msg_size = sbepp::message_traits<
        market::schema::messages:msg>::size_bytes(
            group_entries.size(), max_data_size.size());

    std::vector<char> buf;
    buf.resize(msg_size);

    auto m = sbepp::make_view<market::messages::msg>(buf.data(), buf.size());
    // encode message as usual...
}
```

---

## Stringification {#stringification-example}

Here's an example of how to build stringification visitor using
[fmt](https://github.com/fmtlib/fmt) and [Visit API](#visit-api) (you can find
the full example in `stringification.test.cpp`):

```cpp
class to_string_visitor
{
public:
    template<typename T, typename Cursor, typename Tag>
    void on_message(T m, Cursor& c, Tag)
    {
        append_line("message: {}", sbepp::message_traits<Tag>::name());
        sbepp::visit(sbepp::get_header(m), *this);
        append_line("content: ");
        indentation++;
        sbepp::visit_children(m, c, *this);
        indentation--;
    }

    template<typename T, typename Cursor, typename Tag>
    bool on_group(T g, Cursor& c, Tag)
    {
        append_line("{}:", sbepp::group_traits<Tag>::name());
        indentation++;
        sbepp::visit_children(g, c, *this);
        indentation--;

        return {};
    }

    template<typename T, typename Cursor>
    bool on_entry(T entry, Cursor& c)
    {
        append_line("entry:");
        indentation++;
        sbepp::visit_children(entry, c, *this);
        indentation--;

        return {};
    }

    template<typename T, typename Tag>
    bool on_data(T d, Tag)
    {
        on_array(d, sbepp::data_traits<Tag>::name());
        return {};
    }

    template<typename T, typename Tag>
    bool on_field(T f, Tag)
    {
        on_encoding(f, sbepp::field_traits<Tag>::name());
        return {};
    }

    template<typename T, typename Tag>
    bool on_type(T t, Tag)
    {
        on_encoding(t, sbepp::type_traits<Tag>::name());
        return {};
    }

    template<typename T, typename Tag>
    bool on_enum(T e, Tag)
    {
        on_encoding(e, sbepp::enum_traits<Tag>::name());
        return {};
    }

    template<typename T, typename Tag>
    bool on_set(T s, Tag)
    {
        on_encoding(s, sbepp::set_traits<Tag>::name());
        return {};
    }

    template<typename T, typename Tag>
    bool on_composite(T c, Tag)
    {
        on_encoding(c, sbepp::composite_traits<Tag>::name());
        return {};
    }

    template<typename Tag>
    void on_enum_value(auto /*e*/, Tag)
    {
        append("{}\n", sbepp::enum_value_traits<Tag>::name());
    }

    void on_enum_value(auto e, sbepp::unknown_enum_value_tag)
    {
        append("unknown({})\n", sbepp::to_underlying(e));
    }

    template<typename Tag>
    void on_set_choice(const bool value, Tag)
    {
        if(value)
        {
            if(!is_first_choice)
            {
                append(", ");
            }
            is_first_choice = false;
            append("{}", sbepp::set_choice_traits<Tag>::name());
        }
    }

    const std::string& str() const
    {
        return res;
    }

private:
    std::string res;
    std::size_t indentation{};
    bool is_first_choice{};

    void indent()
    {
        fmt::format_to(std::back_inserter(res), "{:{}}", "", indentation * 4);
    }

    template<typename... Args>
    void append(fmt::format_string<Args...> fmt, Args&&... args)
    {
        fmt::format_to(
            std::back_inserter(res), fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void append_line(fmt::format_string<Args...> fmt, Args&&... args)
    {
        indent();
        append(fmt, std::forward<Args>(args)...);
        res.push_back('\n');
    }

    void on_encoding(sbepp::required_type auto t, const char* name)
    {
        append_line("{}: {}", name, *t);
    }

    void on_encoding(sbepp::optional_type auto t, const char* name)
    {
        if(t)
        {
            append_line("{}: {}", name, *t);
        }
        else
        {
            append_line("{}: null", name);
        }
    }

    void on_encoding(sbepp::array_type auto a, const char* name)
    {
        on_array(a, name);
    }

    template<typename T>
    void on_array(T a, const char* name)
    {
        if constexpr(std::is_same_v<typename T::value_type, char>)
        {
            // output char arrays as C-strings. Keep in mind that they are not
            // required to be null-terminated so pass size explicitly
            append_line("{}: {:.{}}", name, a.data(), a.size());
        }
        else
        {
            // use standard range-formatter
            append_line("{}: {}", name, a);
        }
    }

    void on_encoding(sbepp::enumeration auto e, const char* name)
    {
        indent();
        append("{}: ", name);
        sbepp::visit(e, *this);
    }

    void on_encoding(sbepp::set auto s, const char* name)
    {
        indent();
        append("{}: (", name);
        is_first_choice = true;
        sbepp::visit(s, *this);
        append(")\n");
    }

    void on_encoding(sbepp::composite auto c, const char* name)
    {
        append_line("{}:", name);
        indentation++;
        sbepp::visit_children(c, *this);
        indentation--;
    }
};

// usage:
auto res = sbepp::visit<to_string_visitor>(message);
fmt::print("{}", res.str());
```

Now, for a message like:

```xml
<enum name="numbers_enum" encodingType="uint8">
    <validValue name="One">1</validValue>
    <validValue name="Two">2</validValue>
</enum>

<set name="options_set" encodingType="uint8">
    <choice name="A">0</choice>
    <choice name="B">2</choice>
</set>

<sbe:message name="msg28" id="28">
    <field name="required" id="1" type="uint32"/>
    <field name="optional1" id="2" type="uint32_opt"/>
    <field name="optional2" id="3" type="uint32_opt"/>
    <field name="number" id="4" type="numbers_enum"/>
    <field name="option" id="5" type="options_set"/>
    <field name="string" id="6" type="str128"/>
    <field name="array" id="7" type="arr8"/>

    <group name="group" id="8">
        <field name="number" id="1" type="uint32"/>
    </group>
    
    <data name="varData" id="9" type="varDataEncoding"/>
    <data name="varStr" id="10" type="varStrEncoding"/>
</sbe:message>
```

we can get the following output:

```
message: msg28
messageHeader:
    blockLength: 150
    templateId: 28
    schemaId: 1
    version: 0
content: 
    required: 1
    optional1: 2
    optional2: null
    number: One
    option: (A, B)
    string: hi
    array: [1, 0, 2, 0, 0, 0, 0, 0]
    group:
        entry:
            number: 1
        entry:
            number: 2
    varData: [1, 2]
    varStr: ab
```

---

## Automatic handling of all schema messages

Using `sbepp::schema_traits::message_tags` we can make a helper that takes
incoming message buffer, automatically detects what schema message it represents
and passes to the callback a corresponding message view object (see the full
example in `handle_schema_message.test.cpp`):

```cpp
template<typename SchemaTag, typename Byte, typename... MessageTags, typename F>
void handle_message_impl(
    const Byte* data,
    const std::size_t size,
    F&& cb,
    sbepp::type_list<MessageTags...>)
{
    const auto msg_id =
        sbepp::make_const_view<
            sbepp::schema_traits<SchemaTag>::template header_type>(data, size)
            .templateId()
            .value();

    const auto try_create_message = [msg_id, data, size, &cb]<typename Tag>(Tag)
    {
        if(msg_id == sbepp::message_traits<Tag>::id())
        {
            cb(sbepp::make_view<
                sbepp::message_traits<Tag>::template value_type>(data, size));
            return true;
        }
        return false;
    };

    const auto is_known_message = (try_create_message(MessageTags{}) || ...);
    if(!is_known_message)
    {
        // log error somehow
    }
}

// invokes `cb` with message view corresponding to the given buffer
template<typename SchemaTag, typename Byte, typename F>
void handle_schema_message(const Byte* data, const std::size_t size, F&& cb)
{
    using message_tags = typename sbepp::schema_traits<SchemaTag>::message_tags;
    handle_message_impl<SchemaTag>(
        data, size, std::forward<F>(cb), message_tags{});
}

// usage:
handle_schema_message<test_schema::schema>(
    buf.data(), buf.size(), overloaded{
        [](test_schema::messages::msg4<const byte_type>)
        {
            // handle `msg4`
        },
        [](auto)
        {
            // not interested in other messages
        }
    }
);
```

With this helper, users can avoid writing separate if-else/switch-case statement
per each schema message manually and only handle messages they are interested
in.

---

## Tag-based accessors {#tag-based-accessors-examples}
### Access field by name

Sometimes it's useful to access fields by their names. Using
`sbepp::message_traits::field_tags` and `sbepp::field_traits::name` it's
possible to generate by-name accessors at compile-time (see the full example in
`get_by_name.test.cpp`):

```cpp
template<sbepp::message Message, typename... FieldTags>
std::uint64_t get_by_name_impl(
    Message msg,
    const std::string_view field_name,
    sbepp::type_list<FieldTags...>)
{
    std::uint64_t res{};

    auto try_get_value = [&res]<typename Tag>(auto msg, auto name, Tag)
    {
        if(sbepp::field_traits<Tag>::name() == name)
        {
            using value_type_tag = sbepp::field_traits<Tag>::value_type_tag;
            if constexpr(
                sbepp::type_tag<value_type_tag>
                || sbepp::set_tag<value_type_tag>)
            {
                res = static_cast<std::uint64_t>(*sbepp::get_by_tag<Tag>(msg));
            }
            else if constexpr(sbepp::enum_tag<value_type_tag>)
            {
                res = static_cast<std::uint64_t>(
                    sbepp::to_underlying(sbepp::get_by_tag<Tag>(msg)));
            }
            else
            {
                throw std::runtime_error{"Unsupported field type"};
            }
            return true;
        }
        return false;
    };

    const auto is_found = (try_get_value(msg, field_name, FieldTags{}) || ...);
    if(!is_found)
    {
        throw std::runtime_error{"Wrong field name"};
    }

    return res;
}

// gets message field underlying value by name
template<sbepp::message Message>
std::uint64_t get_by_name(Message msg, const std::string_view field_name)
{
    return get_by_name_impl(
        msg,
        field_name,
        typename sbepp::message_traits<
            sbepp::traits_tag_t<Message>>::field_tags{});
}

// usage:
auto value = get_by_name(msg, "fieldName");
```

### Set all optional fields to null

When creating a new SBE message, underlying memory buffer is often initialized
with zeros. This is OK for `required` fields which by definition must be set
explicitly. However, `optional` fields are different and zeros might not
represent their null values (in fact, none of the SBE built-in types except
`char` have `0` as their null value). To avoid setting each of them manually, we
can iterate over message field tags, check their `presence` and set `optional`
ones to null (see the full example in `nullify_optional_fields.test.cpp`):

```cpp
template<typename... FieldTags>
void nullify_optional_fields_impl(auto view, sbepp::type_list<FieldTags...>)
{
    auto set_nullopt = []<typename Tag>(auto view, Tag)
    {
        if constexpr(
            sbepp::field_traits<Tag>::presence()
            == sbepp::field_presence::optional)
        {
            sbepp::set_by_tag<Tag>(view, sbepp::nullopt);
        }
    };

    (set_nullopt(view, FieldTags{}), ...);
}

// sets optional message fields to null
template<sbepp::message Message>
void nullify_optional_fields(Message msg)
{
    nullify_optional_fields_impl(
        msg,
        typename sbepp::message_traits<
            sbepp::traits_tag_t<Message>>::field_tags{});
}

// usage:
nullify_optional_fields(msg);
```