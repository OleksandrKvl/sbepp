# Stringification primitives {#stringification}

Instead of providing fully-fledged stringification mechanism out of the box,
`sbepp` provides only low-level primitives which in pair with
[Visit API](#visit-api) can be used to build `to_string()`-like routine that
uses a specific formatting/output mechanisms.  
For most field types their raw value is enough to produce human-readable
representation. The only exceptions are enums, for which we need enumerator's
name, and sets, for which we need choice names. These two cases are covered by
`sbepp::enum_to_string()` and `sbepp::visit_set()`.

Here's an example of how to build stringification visitor using
[fmt](https://github.com/fmtlib/fmt) and [Visit API](#visit-api)(you can find
the full example in `stringification.test.cpp`):

```cpp
class to_string_visitor
{
public:
    template<typename T, typename Cursor, typename Tag>
    void on_message(T m, Cursor& c, Tag)
    {
        append("message: {}\n", sbepp::message_traits<Tag>::name());
        sbepp::visit(sbepp::get_header(m), *this);
        append("content: \n");
        indentation++;
        sbepp::visit_children(m, c, *this);
        indentation--;
    }

    template<typename T, typename Cursor, typename Tag>
    bool on_group(T g, Cursor& c, Tag)
    {
        append("{}:\n", sbepp::group_traits<Tag>::name());
        indentation++;
        sbepp::visit_children(g, c, *this);
        indentation--;

        return {};
    }

    template<typename T, typename Cursor>
    bool on_entry(T entry, Cursor& c)
    {
        append("entry:\n");
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

    const std::string& str() const
    {
        return res;
    }

private:
    std::string res;
    std::size_t indentation{};

    template<typename... Args>
    void append(fmt::format_string<Args...> fmt, Args&&... args)
    {
        fmt::format_to(std::back_inserter(res), "{:{}}", "", indentation * 4);
        fmt::format_to(
            std::back_inserter(res), fmt, std::forward<Args>(args)...);
    }

    void on_encoding(sbepp::required_type auto t, const char* name)
    {
        append("{}: {}\n", name, *t);
    }

    void on_encoding(sbepp::optional_type auto t, const char* name)
    {
        if(t)
        {
            append("{}: {}\n", name, *t);
        }
        else
        {
            append("{}: null\n", name);
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
            append("{}: {:.{}}\n", name, a.data(), a.size());
        }
        else
        {
            // use standard range-formatter
            append("{}: {}\n", name, a);
        }
    }

    void on_encoding(sbepp::enumeration auto e, const char* name)
    {
        const auto as_string = sbepp::enum_to_string(e);
        if(as_string)
        {
            append("{}: {}\n", name, as_string);
        }
        else
        {
            append("{}: unknown({})\n", name, sbepp::to_underlying(e));
        }
    }

    void on_encoding(sbepp::set auto s, const char* name)
    {
        std::size_t n{};
        std::array<const char*, 64> choices{};
        sbepp::visit_set(
            s,
            [&n, &choices](const auto value, const auto name)
            {
                if(value)
                {
                    choices[n] = name;
                    n++;
                }
            });

        append(
            "{}: ({})\n",
            name,
            fmt::join(choices.begin(), choices.begin() + n, ", "));
    }

    void on_encoding(sbepp::composite auto c, const char* name)
    {
        append("{}:\n", name);
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