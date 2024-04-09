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
d.push_back('h');
d.push_back('i');
d.push_back('!');

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
d.push_back('h');
d.push_back('i');
d.push_back('!');
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
auto checked_size = sbepp::size_bytes_checked(m);
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