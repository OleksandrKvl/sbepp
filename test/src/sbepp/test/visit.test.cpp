// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#ifdef USE_TOP_FILE
#    include <test_schema/test_schema.hpp>
#else
#    include <test_schema/messages/msg26.hpp>
#    include <test_schema/messages/msg17.hpp>
#    include <test_schema/types/refs_composite.hpp>
#    include <test_schema/types/composite_b.hpp>
#endif

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <type_traits>

namespace
{
using byte_type = std::uint8_t;
using message_t = test_schema::messages::msg26<byte_type>;
using group_tag = test_schema::schema::messages::msg26::group;
using group_t = sbepp::group_traits<group_tag>::value_type<byte_type>;

template<bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template<typename GroupTag>
class entry_wrapper
    : public sbepp::group_traits<GroupTag>::template entry_type<byte_type>
{
public:
    using base_t =
        typename sbepp::group_traits<GroupTag>::template entry_type<byte_type>;

    constexpr entry_wrapper(byte_type* ptr, const std::size_t size)
        : base_t{ptr, size, sbepp::group_traits<GroupTag>::block_length()}
    {
    }
};

class FieldsContainer : public ::testing::Test
{
public:
    std::array<byte_type, 512> buf{};
};

struct valid_values
{
    sbepp::uint32_t uint32_value;
    test_schema::types::uint32_req uint32_req_value;
    test_schema::types::numbers_enum enum_value;
    test_schema::types::options_set set_value;
    byte_type* composite_ptr;
    byte_type* msg_ptr{};
    byte_type* array_ptr{};
    byte_type* group_ptr{};
    byte_type* entry_ptr{};
    byte_type* data_ptr{};
};

class single_level_tester
{
public:
    explicit single_level_tester(
        valid_values values, const std::vector<std::string>& valid_names)
        : values{values}, valid_names{valid_names}
    {
    }

    template<typename T, typename Cursor, typename Tag>
    void on_message(T m, Cursor& c, Tag)
    {
        names.emplace_back(sbepp::message_traits<Tag>::name());
        if(test_values(sbepp::addressof(m), values.msg_ptr))
        {
            sbepp::visit_children(m, c, *this);
        }
    }

    template<typename T, typename Cursor, typename Tag>
    bool on_group(T g, Cursor&, Tag)
    {
        names.emplace_back(sbepp::group_traits<Tag>::name());
        return !test_values(sbepp::addressof(g), values.group_ptr);
    }

    template<typename T, typename Cursor>
    bool on_entry(T e, Cursor& c)
    {
        if(test_values(sbepp::addressof(e), values.entry_ptr))
        {
            sbepp::visit_children(e, c, *this);
        }
        return false;
    }

    template<typename T, typename Tag>
    bool on_data(T d, Tag)
    {
        names.emplace_back(sbepp::data_traits<Tag>::name());
        return !test_values(sbepp::addressof(d), values.data_ptr);
    }

    template<typename T, typename Tag>
    bool on_field(T f, Tag)
    {
        return on_encoding(f, sbepp::field_traits<Tag>::name());
    }

    template<typename T, typename Tag>
    bool on_composite(T c, Tag)
    {
        return on_encoding(c, sbepp::composite_traits<Tag>::name());
    }

    template<typename T, typename Tag>
    bool on_type(T t, Tag)
    {
        return on_encoding(t, sbepp::type_traits<Tag>::name());
    }

    template<typename T, typename Tag>
    bool on_enum(T e, Tag)
    {
        return on_encoding(e, sbepp::enum_traits<Tag>::name());
    }

    template<typename T, typename Tag>
    bool on_set(T s, Tag)
    {
        return on_encoding(s, sbepp::set_traits<Tag>::name());
    }

    bool is_valid() const
    {
        return valid && (valid_names == names);
    }

private:
    valid_values values;
    std::vector<std::string> valid_names;
    std::vector<std::string> names;
    bool valid{true};

    template<typename T>
    bool test_values(T lhs, T rhs)
    {
        if(lhs != rhs)
        {
            valid = false;
        }
        return valid;
    }

    template<typename T>
    enable_if_t<sbepp::is_composite<T>::value, bool>
        on_encoding(T c, const char* name)
    {
        names.emplace_back(name);
        return !test_values(sbepp::addressof(c), values.composite_ptr);
    }

    template<typename T>
    enable_if_t<sbepp::is_non_array_type<T>::value, bool>
        on_encoding(T t, const char* name)
    {
        names.emplace_back(name);
        const auto value = *t;
        const auto builtin_value = *values.uint32_value;
        const auto number_value = *values.uint32_req_value;
        if(std::strcmp(name, "builtin") == 0)
        {
            return !test_values(value, builtin_value);
        }
        else if(std::strcmp(name, "number") == 0)
        {
            return !test_values(value, number_value);
        }
        else
        {
            valid = false;
        }

        return {};
    }

    template<typename T>
    enable_if_t<sbepp::is_enum<T>::value, bool>
        on_encoding(T e, const char* name)
    {
        names.emplace_back(name);
        return !test_values(e, values.enum_value);
    }

    template<typename T>
    enable_if_t<sbepp::is_set<T>::value, bool>
        on_encoding(T s, const char* name)
    {
        names.emplace_back(name);
        return !test_values(s, values.set_value);
    }

    template<typename T>
    enable_if_t<sbepp::is_array_type<T>::value, bool>
        on_encoding(T a, const char* name)
    {
        names.emplace_back(name);
        return !test_values(sbepp::addressof(a), values.array_ptr);
    }
};

using FieldsVisitorTest = FieldsContainer;

TEST_F(FieldsVisitorTest, MessageVisitorGetsCorrectValues)
{
    message_t m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    sbepp::fill_group_header(m.group(), 0);
    valid_values values;
    values.uint32_value = 1;
    values.uint32_req_value = 3;
    values.enum_value = test_schema::types::numbers_enum::Two;
    values.set_value.A(true).B(true);
    values.msg_ptr = sbepp::addressof(m);
    values.array_ptr = sbepp::addressof(m.array());
    values.composite_ptr = sbepp::addressof(m.composite());
    values.group_ptr = sbepp::addressof(m.group());
    values.data_ptr = sbepp::addressof(m.data());
    m.builtin(values.uint32_value);
    m.number(values.uint32_req_value);
    m.enumeration(values.enum_value);
    m.set(values.set_value);

    std::vector<std::string> valid_names{
        "msg26",
        "builtin",
        "number",
        "enumeration",
        "set",
        "array",
        "composite",
        "group",
        "data"};
    single_level_tester tester1{values, valid_names};

    sbepp::visit(m, tester1);

    ASSERT_TRUE(tester1.is_valid());
}

// can it be simplified by filling a single message including single entry?
// at least filling can be put into function.
TEST_F(FieldsVisitorTest, EntryVisitorGetsCorrectValues)
{
    entry_wrapper<group_tag> e{buf.data(), buf.size()};
    sbepp::fill_group_header(e.group(), 0);
    valid_values values;
    values.uint32_value = 1;
    values.uint32_req_value = 3;
    values.enum_value = test_schema::types::numbers_enum::Two;
    values.set_value.A(true).B(true);
    values.entry_ptr = sbepp::addressof(e);
    values.array_ptr = sbepp::addressof(e.array());
    values.composite_ptr = sbepp::addressof(e.composite());
    values.group_ptr = sbepp::addressof(e.group());
    values.data_ptr = sbepp::addressof(e.data());
    e.builtin(values.uint32_value);
    e.number(values.uint32_req_value);
    e.enumeration(values.enum_value);
    e.set(values.set_value);

    std::vector<std::string> valid_names{
        "builtin",
        "number",
        "enumeration",
        "set",
        "array",
        "composite",
        "group",
        "data"};
    single_level_tester tester1{values, valid_names};

    sbepp::visit(e, tester1);

    ASSERT_TRUE(tester1.is_valid());
}

using CompositeTags = ::testing::Types<
    test_schema::schema::types::refs_composite,
    test_schema::schema::types::composite_b>;

template<typename Tag>
class VisitCompositeTest : public ::testing::Test
{
public:
    using composite_tag = Tag;
    std::array<byte_type, 512> buf;
    typename sbepp::composite_traits<Tag>::template value_type<byte_type> c{
        buf.data(), buf.size()};
};

TYPED_TEST_SUITE(VisitCompositeTest, CompositeTags);

struct valid_values2
{
    std::uint32_t uint32_value;
    std::underlying_type<test_schema::types::numbers_enum>::type enum_value;
    sbepp::set_traits<test_schema::schema::types::options_set>::encoding_type
        set_value;
    byte_type* root_composite_ptr{};
    byte_type* child_composite_ptr{};
    byte_type* array_ptr{};
};

class composite_tester
{
public:
    explicit composite_tester(
        valid_values2 values, const std::vector<std::string>& valid_names)
        : values{values}, valid_names{valid_names}
    {
    }

    template<typename T, typename Tag>
    bool on_composite(T c, Tag)
    {
        names.emplace_back(sbepp::composite_traits<Tag>::name());
        if(is_first_composite)
        {
            is_first_composite = false;
            if(test_values(sbepp::addressof(c), values.root_composite_ptr))
            {
                sbepp::visit_children(c, *this);
                return false;
            }
            return true;
        }

        return !test_values(sbepp::addressof(c), values.child_composite_ptr);
    }

    template<typename T, typename Tag>
    enable_if_t<sbepp::is_non_array_type<T>::value, bool> on_type(T t, Tag)
    {
        names.emplace_back(sbepp::type_traits<Tag>::name());
        return !test_values(*t, values.uint32_value);
    }

    template<typename T, typename Tag>
    enable_if_t<sbepp::is_array_type<T>::value, bool> on_type(T a, Tag)
    {
        names.emplace_back(sbepp::type_traits<Tag>::name());
        return !test_values(sbepp::addressof(a), values.array_ptr);
    }

    template<typename T, typename Tag>
    bool on_enum(T e, Tag)
    {
        names.emplace_back(sbepp::enum_traits<Tag>::name());
        return !test_values(sbepp::to_underlying(e), values.enum_value);
    }

    template<typename T, typename Tag>
    bool on_set(T s, Tag)
    {
        names.emplace_back(sbepp::set_traits<Tag>::name());
        return !test_values(*s, values.set_value);
    }

    bool is_valid() const
    {
        return valid && (valid_names == names);
    }

private:
    valid_values2 values;
    std::vector<std::string> valid_names;
    std::vector<std::string> names;
    bool valid{true};
    bool is_first_composite{true};

    template<typename T>
    bool test_values(T lhs, T rhs)
    {
        if(lhs != rhs)
        {
            valid = false;
        }
        return valid;
    }
};

TYPED_TEST(VisitCompositeTest, CompositeVisitorGetsCorrectValues)
{
    auto& c = this->c;
    c.number(2);
    c.set(c.set().A(true).B(true));
    c.enumeration(decltype(c.enumeration())::Two);

    valid_values2 values{};
    values.uint32_value = *c.number();
    values.enum_value = sbepp::to_underlying(c.enumeration());
    values.set_value = *c.set();
    values.root_composite_ptr = sbepp::addressof(c);
    values.child_composite_ptr = sbepp::addressof(c.composite());
    values.array_ptr = sbepp::addressof(c.array());

    composite_tester visitor{
        values,
        {sbepp::composite_traits<typename TestFixture::composite_tag>::name(),
         "number",
         "array",
         "enumeration",
         "set",
         "composite"}};
    sbepp::visit(c, visitor);

    ASSERT_TRUE(visitor.is_valid());
}

class group_visitor
{
public:
    explicit group_visitor(const std::vector<void*>& entries) : entries{entries}
    {
    }

    template<typename T, typename Cursor>
    bool on_entry(T e, Cursor& c)
    {
        if(sbepp::addressof(e) != entries[entry_index])
        {
            valid = false;
            return true;
        }
        entry_index++;

        // required to move the cursor through the fields
        sbepp::visit_children(e, c, *this);
        return false;
    }

    template<typename T, typename Tag>
    bool on_field(T, Tag)
    {
        return {};
    }

    bool is_valid() const
    {
        return valid && (entry_index == entries.size());
    }

private:
    std::vector<void*> entries;
    std::size_t entry_index{};
    bool valid{true};
};

TEST(VisitGroupTest, OnEntryVisitorIsCalledCorrectNumberOfTimes)
{
    std::array<byte_type, 512> buf{};
    using group_t = sbepp::group_traits<
        test_schema::schema::messages::msg17::group>::value_type<byte_type>;
    group_t g{buf.data(), buf.size()};
    static constexpr auto num_of_entries = 3;
    sbepp::fill_group_header(g, num_of_entries);
    std::vector<void*> entries;
    entries.reserve(num_of_entries);
    for(const auto entry : g)
    {
        entries.push_back(sbepp::addressof(entry));
    }
    group_visitor visitor{entries};

    sbepp::visit_children(g, visitor);

    ASSERT_TRUE(visitor.is_valid());
}
} // namespace
