// SPDX-License-Identifier: MIT
// Copyright (c) 2025, Oleksandr Koval

#include <test_schema/test_schema.hpp>

// This example is based on C++20
#if SBEPP_HAS_CONCEPTS

#    include <gtest/gtest.h>

#    include <array>
#    include <stdexcept>
#    include <cstdint>
#    include <string_view>

namespace
{
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

TEST(GetFieldByNameExample, CanGetNumericFieldByName)
{
    std::array<std::uint8_t, 128> buf{};
    auto msg =
        sbepp::make_view<test_schema::messages::msg4>(buf.data(), buf.size());
    sbepp::fill_message_header(msg);
    const auto valid_value1 = 1;
    const auto valid_value2 = 2;
    msg.number1(valid_value1);
    msg.number2(valid_value2);

    EXPECT_EQ(get_by_name(msg, "number1"), valid_value1);
    EXPECT_EQ(get_by_name(msg, "number2"), valid_value2);
}

TEST(GetFieldByNameExample, CanGetEnumFieldByName)
{
    std::array<std::uint8_t, 128> buf{};
    auto msg =
        sbepp::make_view<test_schema::messages::msg6>(buf.data(), buf.size());
    sbepp::fill_message_header(msg);
    const auto valid_value1 = 1;
    const auto valid_value2 = 2;
    msg.enumeration1(test_schema::types::numbers_enum{valid_value1});
    msg.enumeration2(test_schema::types::numbers_enum{valid_value2});

    EXPECT_EQ(get_by_name(msg, "enumeration1"), valid_value1);
    EXPECT_EQ(get_by_name(msg, "enumeration2"), valid_value2);
}

TEST(GetFieldByNameExample, CanGetSetFieldByName)
{
    std::array<std::uint8_t, 128> buf{};
    auto msg =
        sbepp::make_view<test_schema::messages::msg7>(buf.data(), buf.size());
    sbepp::fill_message_header(msg);
    const auto valid_value1 = 1;
    const auto valid_value2 = 2;
    msg.set1(test_schema::types::options_set{valid_value1});
    msg.set2(test_schema::types::options_set{valid_value2});

    EXPECT_EQ(get_by_name(msg, "set1"), valid_value1);
    EXPECT_EQ(get_by_name(msg, "set2"), valid_value2);
}

TEST(GetFieldByNameExample, ThrowsOnUnsupportedFieldType)
{
    std::array<std::uint8_t, 128> buf{};
    auto msg =
        sbepp::make_view<test_schema::messages::msg8>(buf.data(), buf.size());
    sbepp::fill_message_header(msg);

    EXPECT_THROW(get_by_name(msg, "composite1"), std::runtime_error);
}

TEST(GetFieldByNameExample, ThrowsOnWrongFieldName)
{
    std::array<std::uint8_t, 128> buf{};
    auto msg =
        sbepp::make_view<test_schema::messages::msg8>(buf.data(), buf.size());
    sbepp::fill_message_header(msg);

    EXPECT_THROW(get_by_name(msg, "wrong_name"), std::runtime_error);
}
} // namespace
#endif