# Overview {#overview}

**sbepp** is a zero-overhead C++ implementation of [Simple Binary Encoding (SBE)](https://www.fixtrading.org/standards/sbe/).
It consists of two parts:
- `sbeppc`, schema compiler which generates header-only C++ code
- `sbepp`, header-only supporting library

## How it works

1. Compile SBE schema using [sbeppc](#sbeppc) to get C++ headers.
2. [Integrate](#integration) them into your project.
3. Link to `sbepp::sbepp` library.
4. [Use it](#representation)!

`sbepp` has two main goals:
- provide fast, direct access to SBE data
- provide convenient representation of SBE entities like types, messages, etc.

As a result, almost all types in `sbepp` are reference semantics wrappers over
memory buffer. They
never manage underlying memory in any way, only treat the data according to SBE
standard. The idea was to provide a thing similar to plain `struct` with
getters/setters and leave rest to a client. The only thing which is handled
implicitly is a schema extension mechanism, everything else must be done by
a client. For a boiler-plate things like header filling `sbepp` provides helper
utilities. Basically, all it does under the hood is offset calculation and
memory read/write.

It's worth mentioning that at the moment `sbepp` cannot generate schema
representation at run-time like other similar software.

## License

Distributed under the MIT license.