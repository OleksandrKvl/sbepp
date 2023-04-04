# sbepp

**sbepp** is a zero-overhead C++ implementation of
[Simple Binary Encoding (SBE)](https://www.fixtrading.org/standards/sbe/).
It consists of two parts:
- `sbeppc`, schema compiler which generates header-only C++ code
- `sbepp`, header-only supporting library

*This project was created in Ukraine during the invasion of Russian terrorist
forces. Please consider donating to the [UNITED24](https://u24.gov.ua/) platform
to help us withstand.*

## Features

- fast, generates the same assembly as a hand-written code
- generated code needs only C++11 and has no dependencies beyond `sbepp` itself
- random access API to access fields in any order
- cursor-based API for efficient work with complex messages in a forward-only
    way
- lightweight, never allocates, most objects store only a single pointer
- convenient, STL-like interface
- supports `constexpr` encoding/decoding in C++20
- never changes schema names, no `get_FieldName()`-like functions
- provides all XML schema information via traits

## Examples

Decoding example:

```cpp
#include <schema_name/messages/msg1.hpp>

auto m = sbepp::make_view<schema_name::messages:msg1>(dataPtr, dataSize);
// read top-level fields
std::cout << *m.required();

if(m.optional())
{
    std::cout << *m.field2();
}

if(m.bitset().A())
{
    std::cout << "bitset.A";
}

// read composite field
std::cout << *m.composite().field();

// read group
for(auto entry : m.group())
{
    std::cout << sbepp::to_underlying(entry.enum_field());
}

// read data
auto d = m.data();
std::cout.write(d.data(), d.size());
```

Encoding example:

```cpp
#include <schema_name/messages/msg1.hpp>

std::array<char, 1024> buf{};
auto m = sbepp::make_view<schema_name::messages:msg1>(buf.data(), buf.size());
sbepp::fill_message_header(m);
// fill top-level fields
m.required(1);
m.optional(2);
m.bitset(schema_name::types::set{}.A(true));

// fill composite field
m.composite().field(1);

// fill group
auto g = m.group();
const auto group_size = 3;
sbepp::fill_group_header(g, group_size);
for(auto entry : g)
{
    g.enum_field(schema_name::types::my_enum::A);
}

// fill data
auto d = m.data();
d.resize(2);
d[0] = 'h';
d[1] = 'i';

const auto msg_size = sbepp::size_bytes(m); // get final message size
send(buf.data(), mgs_size);
```

## Documentation

See full documentation [here](https://oleksandrkvl.github.io/sbepp/).

## Feedback

Feel free to create an issue, send me an [email](oleksandr.koval.dev@gmail.com),
or contact me on [Cpplang](https://cppalliance.org/slack/) slack channel if you
have questions or ideas related to this project.

## License

Distributed under the [MIT license](LICENSE.md).