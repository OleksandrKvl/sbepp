// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

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
using group_tag = test_schema::schema::messages::msg3::nested_group::flat_group;
using group_t = sbepp::group_traits<group_tag>::value_type<byte_type>;
using iterator_t = group_t::cursor_iterator<byte_type>;

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
STATIC_ASSERT(std::input_iterator<iterator_t>);
#endif

// member types test
STATIC_ASSERT_V(
    std::is_same<iterator_t::iterator_category, std::input_iterator_tag>);
STATIC_ASSERT_V(std::is_same<iterator_t::value_type, group_t::value_type>);
STATIC_ASSERT_V(std::is_same<iterator_t::reference, iterator_t::value_type>);
STATIC_ASSERT_V(
    std::is_same<iterator_t::difference_type, group_t::difference_type>);
STATIC_ASSERT_V(std::is_same<
                iterator_t::pointer,
                sbepp::detail::arrow_proxy<iterator_t::value_type>>);

class InputIteratorTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        sbepp::fill_group_header(g, 3);

        // `cursor_range` expects that cursor already points to the first field
        c = sbepp::init_cursor(g);

        for(std::size_t i = 0; i != g.size(); i++)
        {
            g[i].number(i);
        }
    }

    // reserve memory only for 3 entries
    std::array<std::uint8_t, g_header_size + g_block_length * 3> buf{};
    group_t g{buf.data(), buf.size()};
    sbepp::cursor<byte_type> c;
};

TEST_F(InputIteratorTest, DereferenceReturnsEntry)
{
    auto range = g.cursor_range(c);
    std::size_t i{};

    for(auto e : range)
    {
        ASSERT_EQ(e.number(c), i);
        i++;
    }

    STATIC_ASSERT(noexcept(*range.begin()));
    STATIC_ASSERT_V(
        std::is_same<decltype(*range.begin()), iterator_t::reference>);
}

TEST_F(InputIteratorTest, CanAccessEntryMembersThroughArrow)
{
    auto range = g.cursor_range(c);
    auto begin = std::begin(range);
    auto end = std::end(range);
    std::size_t i{};

    for(; begin != end; ++begin)
    {
        ASSERT_EQ(begin->number(c), i);
        i++;
    }

    STATIC_ASSERT(noexcept(begin.operator->()));
    STATIC_ASSERT_V(
        std::is_same<decltype(begin.operator->()), iterator_t::pointer>);
}

TEST_F(InputIteratorTest, PreIncMovesToNextEntry)
{
    auto range = g.cursor_range(c);
    auto it = range.begin();
    auto e1 = *it;
    e1.number(sbepp::cursor_ops::skip(c));

    ++it;

    auto e2 = *it;
    ASSERT_EQ(sbepp::addressof(e2) - sbepp::addressof(e1), g_block_length);
    ASSERT_EQ(e1.number(), 0);
    ASSERT_EQ(e2.number(), 1);
    STATIC_ASSERT(noexcept(++it));
}

TEST_F(InputIteratorTest, PostIncMovesToNextEntry)
{
    auto range = g.cursor_range(c);
    auto it = range.begin();

    auto e1 = *it++;
    (*it).number(sbepp::cursor_ops::skip(c));

    auto e2 = *it;
    ASSERT_EQ(sbepp::addressof(e2) - sbepp::addressof(e1), g_block_length);
    ASSERT_EQ(e1.number(), 0);
    ASSERT_EQ(e2.number(), 1);
    STATIC_ASSERT(noexcept(it++));
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test()
{
    std::array<byte_type, 512> buf{};
    group_t g{buf.data(), buf.size()};
    sbepp::fill_group_header(g, 3);
    auto c = sbepp::init_cursor(g);

    iterator_t it1;
    auto it2 = g.cursor_range(c).begin();
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
