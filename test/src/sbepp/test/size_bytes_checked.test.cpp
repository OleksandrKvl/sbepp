// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <test_schema/messages/msg27.hpp>

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <array>

namespace
{
using byte_type = std::uint8_t;
using message_t = test_schema::messages::msg27<byte_type>;

class SizeBytesCheckedTest : public ::testing::Test
{
public:
    SizeBytesCheckedTest()
    {
        sbepp::fill_message_header(msg);
        sbepp::fill_group_header(msg.group(), num_in_group);
        msg.data().resize(data_size);
    }

    std::array<byte_type, 512> buf{};
    message_t msg{buf.data(), buf.size()};
    static constexpr auto num_in_group = 3;
    static constexpr auto data_size = 10;
};

TEST_F(SizeBytesCheckedTest, ReturnsValidSizeForMessage)
{
    const auto res = sbepp::size_bytes_checked(msg, buf.size());

    ASSERT_TRUE(res.valid);
    ASSERT_EQ(res.size, sbepp::size_bytes(msg));
    STATIC_ASSERT(noexcept(sbepp::size_bytes_checked(msg, buf.size())));
}

TEST_F(SizeBytesCheckedTest, ReturnsValidSizeForGroup)
{
    auto g = msg.group();
    auto remaining_size =
        buf.size() - (sbepp::addressof(g) - sbepp::addressof(msg));
    const auto res = sbepp::size_bytes_checked(g, remaining_size);

    ASSERT_TRUE(res.valid);
    ASSERT_EQ(res.size, sbepp::size_bytes(g));
}

TEST_F(SizeBytesCheckedTest, FailsIfViewIsEmpty)
{
    message_t m;

    ASSERT_FALSE(sbepp::size_bytes_checked(m, 1).valid);
}

TEST_F(SizeBytesCheckedTest, FailsIfViewIsSizeIsZero)
{
    ASSERT_FALSE(sbepp::size_bytes_checked(msg, 0).valid);
}

TEST_F(SizeBytesCheckedTest, FailsIfNoSpaceForMessageHeader)
{
    const auto not_enough_size = sbepp::size_bytes(sbepp::get_header(msg)) - 1;
    ASSERT_FALSE(sbepp::size_bytes_checked(msg, not_enough_size).valid);
}

TEST_F(SizeBytesCheckedTest, FailsIfNoSpaceForGroupHeader)
{
    auto g = msg.group();
    const auto not_enough_size = sbepp::size_bytes(sbepp::get_header(g)) - 1;
    ASSERT_FALSE(sbepp::size_bytes_checked(g, not_enough_size).valid);
}

TEST_F(SizeBytesCheckedTest, FailsIfNoSpaceForMessageBlockLength)
{
    const auto not_enough_size = *sbepp::get_header(msg).blockLength() - 1;
    ASSERT_FALSE(sbepp::size_bytes_checked(msg, not_enough_size).valid);
}

TEST_F(SizeBytesCheckedTest, FailsIfNoSpaceForEntryBlockLength)
{
    auto g = msg.group();
    const auto header = sbepp::get_header(g);
    auto not_enough_size =
        sbepp::size_bytes(header) + *header.blockLength() - 1;
    ASSERT_FALSE(sbepp::size_bytes_checked(g, not_enough_size).valid);
}

TEST_F(SizeBytesCheckedTest, FailsIfNoSpaceForData)
{
    auto d = msg.data();
    const auto not_enough_size =
        sbepp::addressof(d) - sbepp::addressof(msg) + sbepp::size_bytes(d) - 1;
    ASSERT_FALSE(sbepp::size_bytes_checked(msg, not_enough_size).valid);
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test()
{
    std::array<byte_type, 512> buf{};
    message_t msg{buf.data(), buf.size()};
    sbepp::fill_message_header(msg);
    sbepp::fill_group_header(msg.group(), 3);
    msg.data().resize(2);

    return sbepp::size_bytes_checked(msg, buf.size());
}

constexpr auto buf = constexpr_test();
#endif
} // namespace