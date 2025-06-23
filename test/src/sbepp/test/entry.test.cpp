// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <test_schema/messages/msg2.hpp>

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <type_traits>

namespace
{
const auto g_block_length = sbepp::group_traits<
    test_schema::schema::messages::msg2::group>::block_length();

using byte_type = std::uint8_t;
using entry_t = sbepp::group_traits<
    test_schema::schema::messages::msg2::group>::entry_type<byte_type>;
using const_entry_t = sbepp::group_traits<
    test_schema::schema::messages::msg2::group>::entry_type<const byte_type>;

STATIC_ASSERT_V(std::is_nothrow_default_constructible<entry_t>);
STATIC_ASSERT_V(std::is_trivially_copy_constructible<entry_t>);
STATIC_ASSERT_V(std::is_trivially_copy_assignable<entry_t>);
STATIC_ASSERT_V(std::is_trivially_move_constructible<entry_t>);
STATIC_ASSERT_V(std::is_trivially_move_assignable<entry_t>);
STATIC_ASSERT_V(std::is_trivially_destructible<entry_t>);

STATIC_ASSERT(sbepp::is_group_entry<entry_t>::value);

#if SBEPP_HAS_INLINE_VARS
STATIC_ASSERT(sbepp::is_group_entry_v<entry_t>);
#endif

#if SBEPP_HAS_CONCEPTS
STATIC_ASSERT(sbepp::group_entry<entry_t>);
#endif

class EntryTest : public ::testing::Test
{
public:
    std::array<byte_type, 100> buf{};
};

TEST_F(EntryTest, DefaultConstructedToNullptr)
{
    entry_t e;

    ASSERT_EQ(sbepp::addressof(e), nullptr);
    ASSERT_EQ(e(sbepp::detail::get_block_length_tag{}), 0);
}

TEST_F(EntryTest, CanBeConstructedFromBeginEndPointersAndBlockLength)
{
    entry_t e{buf.data(), buf.data() + buf.size(), g_block_length};

    ASSERT_EQ(sbepp::addressof(e), buf.data());
    ASSERT_EQ(e(sbepp::detail::get_block_length_tag{}), g_block_length);
    STATIC_ASSERT_V(
        std::is_nothrow_constructible<
            entry_t,
            decltype(buf.data()),
            decltype(buf.data()),
            decltype(g_block_length)>);
}

TEST_F(EntryTest, CanBeConstructedFromPointerSizeAndBlockLength)
{
    entry_t e{buf.data(), buf.size(), g_block_length};

    ASSERT_EQ(sbepp::addressof(e), buf.data());
    ASSERT_EQ(e(sbepp::detail::get_block_length_tag{}), g_block_length);
    STATIC_ASSERT_V(
        std::is_nothrow_constructible<
            entry_t,
            decltype(buf.data()),
            decltype(buf.size()),
            decltype(g_block_length)>);
}

TEST_F(EntryTest, CopyAndMoveCopyPointer)
{
    entry_t e{buf.data(), buf.size(), g_block_length};
    auto e2{e};

    ASSERT_EQ(sbepp::addressof(e2), sbepp::addressof(e));
    ASSERT_EQ(
        e2(sbepp::detail::get_block_length_tag{}),
        e(sbepp::detail::get_block_length_tag{}));

    entry_t e3;
    e2 = e3;

    ASSERT_EQ(sbepp::addressof(e2), sbepp::addressof(e3));
    ASSERT_EQ(
        e2(sbepp::detail::get_block_length_tag{}),
        e3(sbepp::detail::get_block_length_tag{}));

    auto e4{std::move(e)};

    ASSERT_EQ(sbepp::addressof(e4), sbepp::addressof(e));
    ASSERT_EQ(
        e4(sbepp::detail::get_block_length_tag{}),
        e(sbepp::detail::get_block_length_tag{}));

    e = std::move(e3);

    ASSERT_EQ(sbepp::addressof(e), sbepp::addressof(e3));
    ASSERT_EQ(
        e(sbepp::detail::get_block_length_tag{}),
        e3(sbepp::detail::get_block_length_tag{}));
}

TEST_F(EntryTest, CanBeConstructedFromLessConstType)
{
    STATIC_ASSERT_V(!std::is_nothrow_constructible<entry_t, const_entry_t>);
    STATIC_ASSERT_V(std::is_nothrow_constructible<const_entry_t, entry_t>);

    entry_t e{buf.data(), buf.size(), g_block_length};
    const_entry_t e2{e};

    ASSERT_EQ(sbepp::addressof(e2), sbepp::addressof(e));
    ASSERT_EQ(
        e2(sbepp::detail::get_block_length_tag{}),
        e(sbepp::detail::get_block_length_tag{}));

    e2 = entry_t{};

    ASSERT_EQ(sbepp::addressof(e2), nullptr);
    ASSERT_EQ(e2(sbepp::detail::get_block_length_tag{}), 0);
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test()
{
    std::array<byte_type, 100> buf{};
    entry_t e1;
    e1 = entry_t{buf.data(), buf.data() + buf.size(), 0};
    e1 = entry_t{buf.data(), buf.size(), 0};
    auto e2 = e1;
    entry_t e3{std::move(e2)};
    e2 = std::move(e3);

    return buf;
}

constexpr auto buf = constexpr_test();
#endif
} // namespace
