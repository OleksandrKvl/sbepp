# Visit API {#visit-api}

`sbepp` provides a way to visit a message/group/entry/composite and their
children. This can be used for stringification or conversion to another format.
`sbepp` uses this mechanism to implement `sbepp::size_bytes_checked()`. It's based
on two functions `sbepp::visit()` and `sbepp::visit_children()`. They both
have the same signature: `Visitor&& visit(View, Visitor&& = {})`. Here's the
full visitor interface:

```cpp
class my_visitor
{
public:
    template<typename T, typename Cursor, typename Tag>
    void on_message(T message, Cursor& c, Tag);

    template<typename T, typename Cursor, typename Tag>
    bool on_group(T group, Cursor& c, Tag);

    template<typename T, typename Cursor, typename Tag>
    bool on_entry(T entry, Cursor& c, Tag);

    template<typename T, typename Tag>
    bool on_data(T data, Tag);

    template<typename T, typename Tag>
    bool on_field(T field, Tag);

    template<typename T, typename Tag>
    bool on_type(T type, Tag);

    template<typename T, typename Tag>
    bool on_enum(T enumeration, Tag);

    template<typename T, typename Tag>
    bool on_set(T set, Tag);

    template<typename T, typename Tag>
    bool on_composite(T composite, Tag);
};
```

Most functions take value and its [tag](#tags), the `bool` return value
works as a stop flag, i.e., should be set to `true` to stop visitation. Each
function is called for the corresponding schema node so not all of them are
required, only those for nodes in the object being visited. Note that field can
be type/enum/set/composite, one needs to use corresponding traits/concepts (e.g.
`sbepp::is_type`, `sbepp::composite`) to get the actual field kind.

The difference between `sbepp::visit()` and `sbepp::visit_children()` is that
the former visits only the root object itself, while the latter visits only its
children. To visit children recursively, additional `Cursor` parameter should be
passed to `sbepp::visit_children` because under the hood visitation always uses
cursors. *Note that when visiting message/group header from
`on_message`/`on_group`, cursor should not be passed because headers are
always skipped by cursors.*
For example, here's how to visit message members:

```cpp
class my_visitor
{
public:
    template<typename T, typename Cursor, typename Tag>
    void on_message(T message, Cursor& c, Tag)
    {
        sbepp::visit_children(message, *this, c);
    }
    
    // ...
};

auto m = sbepp::make_view<msg_type>(buf.data(), buf.size());
sbepp::visit<my_visitor>(m);
```

Without that `sbepp::visit_children`, only `on_message` will be called.  
Note that visitation can also be started from any group/entry/composite, not
only from message.

See [Stringification primitives](#stringification) section for an example how
to build your own `to_string()`-like function using Visit API.