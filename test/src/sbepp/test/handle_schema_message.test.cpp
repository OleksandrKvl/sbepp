// SPDX-License-Identifier: MIT
// Copyright (c) 2025, Oleksandr Koval

#include <test_schema/test_schema.hpp>

// This example is based on C++20
#if SBEPP_HAS_CONCEPTS

#    include <gtest/gtest.h>

#    include <cstdint>
#    include <cstddef>
#    include <array>

namespace
{
using byte_type = std::uint8_t;

template<typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template<typename SchemaTag, typename Byte, typename... MessageTags, typename F>
void handle_message_impl(
    const Byte* data,
    const std::size_t size,
    F&& cb,
    sbepp::type_list<MessageTags...>)
{
    const auto msg_id =
        sbepp::make_const_view<
            sbepp::schema_traits<SchemaTag>::template header_type>(data, size)
            .templateId()
            .value();

    const auto try_create_message = [msg_id, data, size, &cb]<typename Tag>(Tag)
    {
        if(msg_id == sbepp::message_traits<Tag>::id())
        {
            cb(sbepp::make_view<
                sbepp::message_traits<Tag>::template value_type>(data, size));
            return true;
        }
        return false;
    };

    const auto is_known_message = (try_create_message(MessageTags{}) || ...);
    if(!is_known_message)
    {
        // log error somehow
    }
}

// invokes `cb` with message view corresponding to the given buffer
template<typename SchemaTag, typename Byte, typename F>
void handle_schema_message(const Byte* data, const std::size_t size, F&& cb)
{
    using message_tags = typename sbepp::schema_traits<SchemaTag>::message_tags;
    handle_message_impl<SchemaTag>(
        data, size, std::forward<F>(cb), message_tags{});
}

TEST(HandleSchemaMessageExample, CanAutomaticallyHandleSchemaMessages)
{
    std::array<byte_type, 128> buf{};
    // for simplicity, we don't create full messages but only change
    // `templateId` value in the header
    auto header = sbepp::make_view<
        sbepp::schema_traits<test_schema::schema>::header_type>(
        buf.data(), buf.size());

    header.templateId(
        sbepp::message_traits<test_schema::schema::messages::msg4>::id());

    bool msg4_is_detected{};
    bool other_is_detected{};

    const auto message_handler = overloaded{
        [&msg4_is_detected](test_schema::messages::msg4<const byte_type>)
        {
            msg4_is_detected = true;
        },
        [&other_is_detected](auto)
        {
            // not interested in other messages
            other_is_detected = true;
        }};

    handle_schema_message<test_schema::schema>(
        buf.data(), buf.size(), message_handler);

    EXPECT_TRUE(msg4_is_detected);
    EXPECT_FALSE(other_is_detected);

    msg4_is_detected = false;
    header.templateId(
        sbepp::message_traits<test_schema::schema::messages::msg5>::id());

    handle_schema_message<test_schema::schema>(
        buf.data(), buf.size(), message_handler);

    EXPECT_TRUE(other_is_detected);
    EXPECT_FALSE(msg4_is_detected);
}
} // namespace
#endif