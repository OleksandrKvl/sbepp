// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include "sbepp/sbepp.hpp"
#ifdef USE_TOP_FILE
#    include <test_schema/test_schema.hpp>
#else
#    include <test_schema/messages/msg3.hpp>
#endif

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <iterator>
#include <array>
#include <type_traits>

namespace
{
using byte_type = std::uint8_t;
using group_tag = test_schema::schema::messages::msg3::nested_group;
using group_t = sbepp::group_traits<group_tag>::value_type<byte_type>;
using iterator_t = group_t::iterator;

constexpr auto g_block_length = sbepp::group_traits<group_tag>::block_length();
constexpr auto g_header_size = sbepp::composite_traits<
    sbepp::group_traits<group_tag>::dimension_type_tag>::size_bytes();
constexpr auto g_subgroup_header_size =
    sbepp::composite_traits<sbepp::group_traits<
        group_tag::flat_group>::dimension_type_tag>::size_bytes();
constexpr auto g_data_length_size =
    sizeof(sbepp::data_traits<group_tag::data>::length_type);
constexpr auto g_empty_entry_size =
    g_block_length + g_subgroup_header_size + g_data_length_size;

STATIC_ASSERT_V(std::is_nothrow_default_constructible<iterator_t>);
STATIC_ASSERT_V(std::is_trivially_copy_constructible<iterator_t>);
STATIC_ASSERT_V(std::is_trivially_copy_assignable<iterator_t>);
STATIC_ASSERT_V(std::is_trivially_move_constructible<iterator_t>);
STATIC_ASSERT_V(std::is_trivially_move_assignable<iterator_t>);
STATIC_ASSERT_V(std::is_trivially_destructible<iterator_t>);

#if __cpp_lib_concepts >= 202002L
STATIC_ASSERT(std::forward_iterator<iterator_t>);
#endif

// member types test
STATIC_ASSERT_V(
    std::is_same<iterator_t::iterator_category, std::forward_iterator_tag>);
STATIC_ASSERT_V(std::is_same<iterator_t::value_type, group_t::value_type>);
STATIC_ASSERT_V(std::is_same<iterator_t::reference, iterator_t::value_type>);
STATIC_ASSERT_V(
    std::is_same<iterator_t::difference_type, group_t::difference_type>);
STATIC_ASSERT_V(std::is_same<
                iterator_t::pointer,
                sbepp::detail::arrow_proxy<iterator_t::value_type>>);

class ForwardIteratorTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        sbepp::fill_group_header(g, 3);
        std::size_t i{};
        for(auto e : g)
        {
            e.number(i);
            sbepp::fill_group_header(e.flat_group(), 0);
            i++;
        }
    }

    // reserve memory only for 3 entries
    std::array<std::uint8_t, g_header_size + 3 * g_empty_entry_size> buf{};
    group_t g{buf.data(), buf.size()};
};

using ForwardIteratorDeathTest = ForwardIteratorTest;

TEST_F(ForwardIteratorTest, DereferenceReturnsEntry)
{
    auto it = g.begin();
    for(std::size_t i = 0; i != g.size(); ++i, ++it)
    {
        ASSERT_EQ((*it).number(), i);
    }

    STATIC_ASSERT(noexcept(*it));
    STATIC_ASSERT_V(std::is_same<decltype(*it), iterator_t::reference>);
}

TEST_F(ForwardIteratorTest, CanAccessEntryMembersThroughArrow)
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

TEST_F(ForwardIteratorTest, PreIncMovesToNextEntry)
{
    auto it = g.begin();
    auto e1 = *it;

    ++it;

    auto e2 = *it;
    ASSERT_EQ(sbepp::addressof(e2) - sbepp::addressof(e1), g_empty_entry_size);
    ASSERT_EQ(e1.number(), 0);
    ASSERT_EQ(e2.number(), 1);
    STATIC_ASSERT(noexcept(++it));
}

TEST_F(ForwardIteratorDeathTest, PreIncTerminatesIfMovedOutOfRange)
{
    auto it = g.end();

    ASSERT_DEATH({ ++it; }, ".*");
}

TEST_F(ForwardIteratorTest, PostIncMovesToNextEntry)
{
    auto it = g.begin();

    auto prev = it++;

    auto e1 = *prev;
    auto e2 = *it;
    ASSERT_EQ(sbepp::addressof(e2) - sbepp::addressof(e1), g_empty_entry_size);
    ASSERT_EQ(e1.number(), 0);
    ASSERT_EQ(e2.number(), 1);
    STATIC_ASSERT(noexcept(it++));
}

TEST_F(ForwardIteratorDeathTest, PostIncTerminatesIfMovedOutOfRange)
{
    auto it = g.end();

    ASSERT_DEATH({ it++; }, ".*");
}

TEST_F(ForwardIteratorTest, ProvidesEqualityOperations)
{
    auto first = g.begin();
    auto second = ++g.begin();

    ASSERT_TRUE(first == first);
    ASSERT_TRUE(second == second);
    ASSERT_FALSE(first == second);
    STATIC_ASSERT(noexcept(first == first));

    ASSERT_TRUE(first != second);
    ASSERT_FALSE(first != first);
    STATIC_ASSERT(noexcept(first != second));
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
    (it1 == it2);
    (it1 != it2);

    return buf;
}

constexpr auto buf = constexpr_test();
#endif
} // namespace
