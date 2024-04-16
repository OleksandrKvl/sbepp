# Visit API {#visit-api}

Visit API provides a way to access various SBE entities and iterate over their
inner parts. It can be used for [stringification](#stringification-example) or
conversion to another data format. `sbepp` uses this mechanism to implement
`sbepp::size_bytes_checked()` to gradually iterate over message parts and check
that it fits into a buffer.

The API is based on two functions, `sbepp::visit()` and
`sbepp::visit_children()`. They have different behavior but both have the same
set of overloads:
- `Visitor&& f(ThingToVisit, Visitor&&)`
- `Visitor&& f(ThingToVisit, Cursor&, Visitor&&)`

To be efficient, Visit API always uses [cursors](#cursor-accessors) under the
hood, the first overload creates it implicitly, the second relies on the
provided one. `Visitor` is an object with member functions in form of
`void/bool on_<entity type>(EntityValue, [Cursor&,] EntityTag)`, where
`<entity type>` corresponds to schema entity being visited, e.g. `on_message`,
`on_group`, etc. When return type is `bool`, returned value works as a
stop-flag, i.e., setting it to `true` stops further visiting. As described
above, Visit API relies on cursor accessors, additional `Cursor&` parameter
allows to pass the cursor to a nested `sbepp::visit()`/`sbepp::visit_children()`
call. `EntityTag` represents a [tag](#tags) of an entity.

Note that visitor is not required to have all possible `on_<entity type>()`
functions but only those that correspond to the structure of the object being
visited. For example, composite visitor doesn't need `on_group()` function. If
it has only type members, having `on_type()` is enough to visit it.

In general, visiting has two parts:
- visiting the parent object itself using `sbepp::visit()`
- visiting its children using `sbepp::visit_children()`

However, this order is not required, each call can be used independently from
the other. For example, if one wants to visit composite members but not the
composite object itself, there's nothing wrong with calling only
`sbepp::visit_children()` without `sbepp::visit()`.

---

## Visiting parent views

Message/group/entry/composite views are called "parent" in this context because
they are the only ones that can contain nested/children members. For them,
`sbepp::visit()` visits only the provided view itself. Here's a complete visitor
interface for such a call:

```cpp
class parent_visitor
{
public:
    template<typename T, typename Cursor, typename Tag>
    void on_message(T message, Cursor& c, Tag);

    template<typename T, typename Cursor, typename Tag>
    bool on_group(T group, Cursor& c, Tag);

    template<typename T, typename Cursor, typename Tag>
    bool on_entry(T entry, Cursor& c, Tag);

    template<typename T, typename Tag>
    bool on_composite(T composite, Tag);
};
```

For message, group and entry visitors, `Cursor&` is provided to support
efficient children visiting. For composites it's not needed because they support
efficient member access without cursors.

---

## Visiting children

`sbepp::visit_children()` visits children of a parent view in the following way:
- for message and group entry, visits their *direct* field/group/data members in
    schema order
- for group, visits all its entries
- for composite, visits its type/enum/set/composite *direct* members in schema
    order

Here's a corresponding visitor interface:

```cpp
class children_visitor
{
public:
    template<typename T, typename Cursor, typename Tag>
    bool on_group(T group, Cursor& c, Tag);

    template<typename T, typename Cursor, typename Tag>
    bool on_entry(T entry, Cursor& c, Tag);

    template<typename T, typename Tag>
    bool on_composite(T composite, Tag);

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
};
```

Note that a field can be represented by type/enum/set/composite so
traits/concepts are often needed to distinguish them for further processing
(e.g. `sbepp::is_type`, `sbepp::composite`).

---

## Visiting enums and sets

When `sbepp::visit()` is applied to a set, it visits all its known choices and
passes their values/tags to a visitor via the following function:

```cpp
class set_visitor{
    template<typename Tag>
    void on_set_choice(bool choice_value, Tag);
};
```

For enums, `sbepp::visit()` invokes visitor with enum value and its tag. When
value is not one of the ones defined in schema, it provides
`sbepp::unknown_enum_value_tag` as a tag. The visitor interface is:

```cpp
class enum_visitor{
    template<typename T, typename Tag>
    void on_enum_value(T enumeration, Tag);
};
```

See [stringification example](#stringification-example) for an example of how
all of the above can be used together to implement `to_string()`-like
functionality.