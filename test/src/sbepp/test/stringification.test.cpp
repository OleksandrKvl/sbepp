// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#ifdef USE_TOP_FILE
#    include <test_schema/test_schema.hpp>
#else
#    include <test_schema/messages/msg28.hpp>
#endif

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <type_traits>
#include <array>
#include <string>

#if SBEPP_HAS_CONCEPTS
namespace
{
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

TEST(StringificationTest, Test1)
{
    namespace types = test_schema::types;
    std::array<char, 1024> buf{};
    auto m =
        sbepp::make_view<test_schema::messages::msg28>(buf.data(), buf.size());
    sbepp::fill_message_header(m);

    m.required(1);
    m.optional1(2);
    m.optional2({});
    m.number(types::numbers_enum::One);
    m.option(types::options_set{}.A(true).B(true));
    m.string()[0] = 'h';
    m.string()[1] = 'i';
    m.array()[0] = 1;
    m.array()[2] = 2;

    auto g = m.group();
    sbepp::fill_group_header(g, 2);
    g[0].number(1);
    g[1].number(2);

    auto var_data = m.varData();
    var_data.push_back(1);
    var_data.push_back(2);

    auto var_str = m.varStr();
    var_str.push_back('a');
    var_str.push_back('b');

    auto res = sbepp::visit<to_string_visitor>(m);

    fmt::print("{}", res.str());

    const auto expected_string =
        // clang-format off
R"(message: msg28
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
)";
    // clang-format on

    ASSERT_EQ(res.str(), expected_string);
}
} // namespace
#endif
