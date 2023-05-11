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
auto header = sbepp::make_const_view<>(buf.data(), buf.size());
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