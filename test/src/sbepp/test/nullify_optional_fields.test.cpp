// SPDX-License-Identifier: MIT
// Copyright (c) 2025, Oleksandr Koval

#include <test_schema/test_schema.hpp>

// This example is based on C++20
#if SBEPP_HAS_CONCEPTS

#    include <gtest/gtest.h>

#    include <array>

namespace
{
template<typename... FieldTags>
void nullify_optional_fields_impl(auto view, sbepp::type_list<FieldTags...>)
{
    auto set_nullopt = []<typename Tag>(auto view, Tag)
    {
        if constexpr(
            sbepp::field_traits<Tag>::presence()
            == sbepp::field_presence::optional)
        {
            sbepp::set_by_tag<Tag>(view, sbepp::nullopt);
        }
    };

    (set_nullopt(view, FieldTags{}), ...);
}

// sets optional message fields to null
template<sbepp::message Message>
void nullify_optional_fields(Message msg)
{
    nullify_optional_fields_impl(
        msg,
        typename sbepp::message_traits<
            sbepp::traits_tag_t<Message>>::field_tags{});
}

TEST(NullifyOptionalFieldsTest, SetsOptionalFieldsToNull)
{
    std::array<std::uint8_t, 128> buf{};
    auto msg =
        sbepp::make_view<test_schema::messages::msg28>(buf.data(), buf.size());
    sbepp::fill_message_header(msg);
    const auto required_value = 1;
    const auto optional_value = 2;
    msg.required(required_value);
    msg.optional1(optional_value);
    msg.optional2(optional_value);

    nullify_optional_fields(msg);

    EXPECT_FALSE(msg.optional1().has_value());
    EXPECT_FALSE(msg.optional2().has_value());
    EXPECT_EQ(msg.required(), required_value);
}
} // namespace
#endif