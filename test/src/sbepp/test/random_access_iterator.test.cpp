// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <test_schema/messages/msg3.hpp>

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <iterator>
#include <array>
#include <type_traits>

namespace
{
using byte_type = std::uint8_t;
using group_tag = test_schema::schema::messages::msg3::nested_group::flat_group;
using group_t = sbepp::group_traits<group_tag>::value_type<byte_type>;
using iterator_t = group_t::iterator;

constexpr auto g_block_length = sbepp::group_traits<group_tag>::block_length();
constexpr auto g_header_size = sbepp::composite_traits<
    sbepp::group_traits<group_tag>::dimension_type_tag>::size_bytes();

STATIC_ASSERT_V(std::is_nothrow_default_constructible<iterator_t>);
STATIC_ASSERT_V(std::is_trivially_copy_constructible<iterator_t>);
STATIC_ASSERT_V(std::is_trivially_copy_assignable<iterator_t>);
STATIC_ASSERT_V(std::is_trivially_move_constructible<iterator_t>);
STATIC_ASSERT_V(std::is_trivially_move_assignable<iterator_t>);
STATIC_ASSERT_V(std::is_trivially_destructible<iterator_t>);

#if __cpp_lib_concepts >= 202002L
STATIC_ASSERT(std::random_access_iterator<iterator_t>);
#endif

// member types test
STATIC_ASSERT_V(std::is_same<
                iterator_t::iterator_category,
                std::random_access_iterator_tag>);
STATIC_ASSERT_V(std::is_same<iterator_t::value_type, group_t::value_type>);
STATIC_ASSERT_V(std::is_same<iterator_t::reference, iterator_t::value_type>);
STATIC_ASSERT_V(
    std::is_same<iterator_t::difference_type, group_t::difference_type>);
STATIC_ASSERT_V(std::is_same<
                iterator_t::pointer,
                sbepp::detail::arrow_proxy<iterator_t::value_type>>);

class RandomAccessIteratorTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        sbepp::fill_group_header(g, 3);
        for(std::size_t i = 0; i != g.size(); i++)
        {
            g[i].number(i);
        }
    }

    // reserve memory only for 3 entries
    std::array<std::uint8_t, g_header_size + g_block_length * 3> buf{};
    group_t g{buf.data(), buf.size()};
};

using RandomAccessIteratorDeathTest = RandomAccessIteratorTest;

TEST_F(RandomAccessIteratorTest, DereferenceReturnsEntry)
{
    auto it = g.begin();
    for(std::size_t i = 0; i != g.size(); ++i, ++it)
    {
        ASSERT_EQ((*it).number(), i);
    }

    STATIC_ASSERT(noexcept(*it));
    STATIC_ASSERT_V(std::is_same<decltype(*it), iterator_t::reference>);
}

TEST_F(RandomAccessIteratorTest, CanAccessEntryMembersThroughArrow)
{
    auto it = g.begin();
    for(std::size_t i = 0; i != g.size(); ++i, ++it)
    {
        ASSERT_EQ(it->number(), i);
    }

    STATIC_ASSERT(noexcept(it.operator->()));
    STATIC_ASSERT_V(
        std::is_same<decltype(it.operator->()), iterator_t::pointer>);
}

TEST_F(RandomAccessIteratorTest, PreIncMovesUpByBlockLength)
{
    auto it = g.begin();
    auto e1 = *it;

    ++it;

    auto e2 = *it;
    ASSERT_EQ(sbepp::addressof(e2) - sbepp::addressof(e1), g_block_length);
    STATIC_ASSERT(noexcept(++it));
}

TEST_F(RandomAccessIteratorDeathTest, PreIncTerminatesIfMovedOutOfRange)
{
    auto it = g.end();

    ASSERT_DEATH({ ++it; }, ".*");
}

TEST_F(RandomAccessIteratorTest, PostIncMovesUpByBlockLength)
{
    auto it = g.begin();

    auto prev = it++;

    auto e1 = *prev;
    auto e2 = *it;
    ASSERT_EQ(sbepp::addressof(e2) - sbepp::addressof(e1), g_block_length);
    STATIC_ASSERT(noexcept(it++));
}

TEST_F(RandomAccessIteratorDeathTest, PostIncTerminatesIfMovedOutOfRange)
{
    auto it = g.end();

    ASSERT_DEATH({ it++; }, ".*");
}

TEST_F(RandomAccessIteratorTest, PreDecMovesDownByBlockLength)
{
    auto it = g.begin() + 1;
    auto e2 = *it;

    --it;

    auto e1 = *it;
    ASSERT_EQ(sbepp::addressof(e2) - sbepp::addressof(e1), g_block_length);
    STATIC_ASSERT(noexcept(--it));
}

TEST_F(RandomAccessIteratorTest, PostDecMovesDownByBlockLength)
{
    auto it = g.begin() + 1;

    auto e2 = *it--;

    auto e1 = *it;
    ASSERT_EQ(sbepp::addressof(e2) - sbepp::addressof(e1), g_block_length);
    STATIC_ASSERT(noexcept(it--));
}

TEST_F(RandomAccessIteratorTest, PlusEqualsMovesUpOrDown)
{
    auto it = g.begin();

    it += 1;

    ASSERT_EQ((*it).number(), 1);

    it = g.end();

    it += -1;

    ASSERT_EQ((*it).number(), 2);
    STATIC_ASSERT(noexcept(it += 1));
}

TEST_F(RandomAccessIteratorTest, PlusReturnsMovedUpOrDown)
{
    ASSERT_EQ((*(g.begin() + 1)).number(), 1);
    ASSERT_EQ((*(g.end() + (-1))).number(), 2);
    STATIC_ASSERT(noexcept(g.begin() + 1));
}

TEST_F(RandomAccessIteratorTest, Plus2ReturnsMovedUpOrDown)
{
    ASSERT_EQ((*(1 + g.begin())).number(), 1);
    ASSERT_EQ((*(-1 + g.end())).number(), 2);
    STATIC_ASSERT(noexcept(1 + g.begin()));
}

TEST_F(RandomAccessIteratorTest, MinusEqualsMovesUpOrDown)
{
    auto it = g.end();

    it -= 1;

    ASSERT_EQ((*it).number(), 2);

    it = g.begin();

    it -= -1;

    ASSERT_EQ((*it).number(), 1);
    STATIC_ASSERT(noexcept(it -= 1));
}

TEST_F(RandomAccessIteratorTest, MinusReturnsMovedUpOrDown)
{
    ASSERT_EQ((*(g.end() - 1)).number(), 2);
    ASSERT_EQ((*(g.begin() - (-1))).number(), 1);
    STATIC_ASSERT(noexcept(g.end() - 1));
}

TEST_F(RandomAccessIteratorTest, Minus2ReturnsDistance)
{
    ASSERT_EQ(g.end() - g.begin(), g.size());
    STATIC_ASSERT(noexcept(g.end() - g.begin()));
}

TEST_F(RandomAccessIteratorTest, SubscriptReturnsEntry)
{
    ASSERT_EQ(g.begin()[0].number(), 0);
    ASSERT_EQ(g.begin()[1].number(), 1);
    ASSERT_EQ(g.end()[-1].number(), 2);
    STATIC_ASSERT(noexcept(g.begin()[0]));
}

TEST_F(RandomAccessIteratorTest, ProvidesComparisonOperations)
{
    auto first = g.begin();
    auto last = g.end();

    ASSERT_TRUE(first == first);
    ASSERT_FALSE(first == last);
    STATIC_ASSERT(noexcept(first == last));

    ASSERT_TRUE(first != last);
    ASSERT_FALSE(first != first);
    STATIC_ASSERT(noexcept(first != last));

    ASSERT_TRUE(first < last);
    ASSERT_FALSE(last < first);
    STATIC_ASSERT(noexcept(first < last));

    ASSERT_TRUE(first <= last);
    ASSERT_TRUE(first <= first);
    ASSERT_FALSE(last <= first);
    STATIC_ASSERT(noexcept(first <= last));

    ASSERT_TRUE(last > first);
    ASSERT_FALSE(first > last);
    STATIC_ASSERT(noexcept(last > first));

    ASSERT_TRUE(last >= first);
    ASSERT_TRUE(last >= last);
    ASSERT_FALSE(first >= last);
    STATIC_ASSERT(noexcept(last >= first));
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test()
{
    std::array<byte_type, 512> buf{};
    group_t g{buf.data(), buf.size()};
    sbepp::fill_group_header(g, 3);
    using it_t = group_t::iterator;
    it_t it1;
    auto it2 = g.begin();
    it1 = it2;

    *it2;
    it2.operator->();
    it2++;
    ++it2;
    it2--;
    --it2;
    it2 += 3;
    it1 = it2 + 1;
    it1 = 1 + it2;
    it2 -= 1;
    it1 = it2 - 1;
    auto distance = it2 - it2;
    (void)distance;
    it2[0];

    (it1 == it2);
    (it1 != it2);
    (it1 < it2);
    (it1 <= it2);
    (it1 >= it2);
    (it1 > it2);

    return buf;
}

constexpr auto buf = constexpr_test();
#endif
} // namespace
