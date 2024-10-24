// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#ifdef USE_SINGLE_FILE
#    include <test_schema.hpp>
#    include <test_schema2.hpp>
#elif defined(USE_TOP_FILE)
#    include <test_schema/test_schema.hpp>
#    include <test_schema2/test_schema2.hpp>
#else
#    include <test_schema/messages/msg3.hpp>
#    include <test_schema2/messages/msg1.hpp>
#endif

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <iterator>
#include <array>

namespace
{
using byte_type = std::uint8_t;
using group_tag = test_schema::schema::messages::msg3::nested_group::flat_group;
using group_t = sbepp::group_traits<group_tag>::value_type<byte_type>;
using const_group_t =
    sbepp::group_traits<group_tag>::value_type<const byte_type>;

#if SBEPP_SIZE_CHECKS_ENABLED
STATIC_ASSERT(sizeof(group_t) == sizeof(void*) * 2);
#else
STATIC_ASSERT(sizeof(group_t) == sizeof(void*));
#endif

STATIC_ASSERT_V(std::is_nothrow_default_constructible<group_t>);
STATIC_ASSERT_V(std::is_trivially_copy_constructible<group_t>);
STATIC_ASSERT_V(std::is_trivially_copy_assignable<group_t>);
STATIC_ASSERT_V(std::is_trivially_move_constructible<group_t>);
STATIC_ASSERT_V(std::is_trivially_move_assignable<group_t>);
STATIC_ASSERT_V(std::is_trivially_destructible<group_t>);

// member types test
STATIC_ASSERT_V(std::is_same<
                group_t::value_type,
                sbepp::group_traits<group_tag>::entry_type<byte_type>>);
STATIC_ASSERT_V(std::is_same<group_t::reference, group_t::value_type>);
STATIC_ASSERT_V(std::is_same<
                group_t::sbe_size_type,
                sbepp::type_traits<sbepp::group_traits<
                    group_tag>::dimension_type_tag::numInGroup>::value_type>);
STATIC_ASSERT_V(
    std::is_same<group_t::size_type, group_t::sbe_size_type::value_type>);
STATIC_ASSERT_V(std::is_same<
                group_t::difference_type,
                typename std::make_signed<group_t::size_type>::type>);
STATIC_ASSERT_V(std::is_same<
                group_t::iterator::iterator_category,
                std::random_access_iterator_tag>);

STATIC_ASSERT(sbepp::is_group<group_t>::value);
STATIC_ASSERT(sbepp::is_flat_group<group_t>::value);

#if SBEPP_HAS_INLINE_VARS
STATIC_ASSERT(sbepp::is_group_v<group_t>);
STATIC_ASSERT(sbepp::is_flat_group_v<group_t>);
#endif

#if SBEPP_HAS_CONCEPTS
STATIC_ASSERT(sbepp::group<group_t>);
STATIC_ASSERT(sbepp::flat_group<group_t>);
#endif

class FlatGroupTest : public ::testing::Test
{
public:
    std::array<std::uint8_t, 64> buf{};
    group_t g{buf.data(), buf.size()};
};

using FlatGroupDeathTest = FlatGroupTest;

TEST_F(FlatGroupTest, DefaultConstructedToNullptr)
{
    group_t g;

    ASSERT_EQ(sbepp::addressof(g), nullptr);
}

TEST_F(FlatGroupTest, CanBeConstructedFromBeginEndPointers)
{
    group_t g{buf.data(), buf.data() + buf.size()};

    ASSERT_EQ(sbepp::addressof(g), buf.data());
    STATIC_ASSERT_V(std::is_nothrow_constructible<
                    group_t,
                    decltype(buf.data()),
                    decltype(buf.data())>);
}

TEST_F(FlatGroupTest, CanBeConstructedFromPointerAndSize)
{
    ASSERT_EQ(sbepp::addressof(g), buf.data());
    STATIC_ASSERT_V(std::is_nothrow_constructible<
                    group_t,
                    decltype(buf.data()),
                    decltype(buf.size())>);
}

TEST_F(FlatGroupTest, CopyAndMoveCopyPointer)
{
    auto g2{g};

    ASSERT_EQ(sbepp::addressof(g2), sbepp::addressof(g));

    group_t g3;
    g2 = g3;

    ASSERT_EQ(sbepp::addressof(g2), sbepp::addressof(g3));

    auto g4{std::move(g)};

    ASSERT_EQ(sbepp::addressof(g4), sbepp::addressof(g));

    g = std::move(g3);

    ASSERT_EQ(sbepp::addressof(g), sbepp::addressof(g3));
}

TEST_F(FlatGroupTest, CanBeConstructedFromLessConstType)
{
    STATIC_ASSERT_V(!std::is_nothrow_constructible<group_t, const_group_t>);
    STATIC_ASSERT_V(std::is_nothrow_constructible<const_group_t, group_t>);

    const_group_t g2{g};

    ASSERT_EQ(sbepp::addressof(g2), sbepp::addressof(g));

    g2 = group_t{};

    ASSERT_EQ(sbepp::addressof(g2), nullptr);
}

TEST_F(FlatGroupTest, GetHeaderReturnsGroupEncodingType)
{
    auto h = sbepp::get_header(g);
    (void)h.blockLength();
    (void)h.numInGroup();

    STATIC_ASSERT_V(std::is_same<
                    decltype(h),
                    sbepp::group_traits<group_tag>::dimension_type<byte_type>>);

    const_group_t g2 = g;
    auto h2 = sbepp::get_header(g2);
    (void)h2.blockLength();
    (void)h2.numInGroup();

    STATIC_ASSERT_V(
        std::is_same<
            decltype(h2),
            sbepp::group_traits<group_tag>::dimension_type<const byte_type>>);
    ASSERT_EQ(sbepp::addressof(h), sbepp::addressof(h2));
    ASSERT_EQ(sbepp::addressof(h), sbepp::addressof(g));
    STATIC_ASSERT(noexcept(sbepp::get_header(g)));
    STATIC_ASSERT(noexcept(sbepp::get_header(g2)));
}

TEST_F(FlatGroupTest, SizeBytesReturnsSizeOfGroupData)
{
    sbepp::fill_group_header(g, 0);
    static constexpr auto header_size = sbepp::composite_traits<
        sbepp::group_traits<group_tag>::dimension_type_tag>::size_bytes();
    auto size = sbepp::size_bytes(g);

    ASSERT_EQ(size, header_size + 0);

    static constexpr auto entries_count = 2;
    g.resize(entries_count);
    size = sbepp::size_bytes(g);

    ASSERT_EQ(
        size,
        header_size
            + entries_count * sbepp::group_traits<group_tag>::block_length());
}

TEST_F(FlatGroupTest, SbeSizeReturnsNumInGroup)
{
    static constexpr auto new_size = 3;
    sbepp::fill_group_header(g, new_size);

    auto sbe_size = g.sbe_size();

    ASSERT_EQ(sbe_size, sbepp::get_header(g).numInGroup());
    ASSERT_EQ(sbe_size, new_size);
    STATIC_ASSERT(noexcept(g.sbe_size()));
}

TEST_F(FlatGroupTest, SizeReturnsSbeSizeValue)
{
    static constexpr auto new_size = 3;
    sbepp::fill_group_header(g, new_size);

    auto size = g.size();

    ASSERT_EQ(size, g.sbe_size().value());
    ASSERT_EQ(size, new_size);
    STATIC_ASSERT(noexcept(g.size()));
}

TEST_F(FlatGroupTest, ResizeUpdatesDimensionNumInGroup)
{
    static constexpr auto new_size = 3;

    g.resize(new_size);

    ASSERT_EQ(g.size(), new_size);
    STATIC_ASSERT(noexcept(g.resize(new_size)));
}

TEST_F(FlatGroupTest, EmptyReflectsSize)
{
    ASSERT_TRUE(g.empty());
    ASSERT_EQ(g.size(), 0);

    g.resize(1);

    ASSERT_FALSE(g.empty());
    ASSERT_NE(g.size(), 0);
    STATIC_ASSERT(noexcept(g.empty()));
}

TEST_F(FlatGroupTest, MaxSizeReturnsMaxValueOfSbeSizeType)
{
    ASSERT_EQ(group_t::max_size(), g.sbe_size().max_value());
}

TEST_F(FlatGroupTest, SubscriptReturnsEntry)
{
    sbepp::fill_group_header(g, 1);

    auto e = g[0];

    ASSERT_EQ(sbepp::addressof(e), sbepp::addressof(*g.begin()));
    STATIC_ASSERT_V(std::is_same<decltype(e), group_t::reference>);
    STATIC_ASSERT(noexcept(g[0]));
}

TEST_F(FlatGroupDeathTest, SubscriptTerminatesIfOutOfRange)
{
    ASSERT_EQ(g.size(), 0);

    ASSERT_DEATH({ g[0]; }, ".*");
}

TEST_F(FlatGroupTest, FrontReturnsFirstEntry)
{
    sbepp::fill_group_header(g, 2);
    g[0].number(1);
    g[1].number(2);

    auto front = g.front();

    ASSERT_EQ(front.number(), g[0].number());
    ASSERT_EQ(sbepp::addressof(front), sbepp::addressof(g[0]));
    STATIC_ASSERT(noexcept(g.front()));
}

TEST_F(FlatGroupDeathTest, FrontTerminatesIfEmpty)
{
    ASSERT_EQ(g.size(), 0);

    ASSERT_DEATH({ g.front(); }, ".*");
}

TEST_F(FlatGroupTest, BackReturnsLastEntry)
{
    sbepp::fill_group_header(g, 2);
    g[0].number(1);
    g[1].number(2);

    auto back = g.back();

    ASSERT_EQ(back.number(), g[1].number());
    ASSERT_EQ(sbepp::addressof(back), sbepp::addressof(g[1]));
    STATIC_ASSERT(noexcept(g.back()));
}

TEST_F(FlatGroupDeathTest, BackTerminatesIfEmpty)
{
    ASSERT_EQ(g.size(), 0);

    ASSERT_DEATH({ g.back(); }, ".*");
}

TEST_F(FlatGroupTest, ClearSetsSizeToZero)
{
    g.resize(1);

    g.clear();

    ASSERT_EQ(g.size(), 0);
    STATIC_ASSERT(noexcept(g.clear()));
}

TEST_F(FlatGroupTest, BeginEndRepresentsEntriesWithinCurrentSize)
{
    ASSERT_EQ(g.size(), 0);
    ASSERT_EQ(g.begin(), g.end());

    sbepp::fill_group_header(g, 3);

    ASSERT_EQ(std::distance(g.begin(), g.end()), g.size());

    STATIC_ASSERT(noexcept(g.begin()));
    STATIC_ASSERT(noexcept(g.end()));
}

TEST_F(FlatGroupTest, FillGroupHeaderSetsBlockLengthAndNumInGroup)
{
    static constexpr auto num_in_group = 3;
    auto header = sbepp::fill_group_header(g, num_in_group);

    ASSERT_EQ(
        header.blockLength(), sbepp::group_traits<group_tag>::block_length());
    ASSERT_EQ(header.numInGroup(), num_in_group);
    STATIC_ASSERT(noexcept(sbepp::fill_group_header(g, num_in_group)));
}

struct fill_group_header
{
    template<typename T>
    auto operator()(T obj) -> decltype(sbepp::fill_group_header(obj))
    {
    }
};

TEST_F(FlatGroupTest, FillGroupHeaderNeedsNonConstByteType)
{
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<fill_group_header, const_group_t>::value);
}

TEST_F(
    FlatGroupTest, FillGroupHeaderSetsNumGroupsAndNumVarDataFieldsIfTheyExist)
{
    using group_tag = test_schema2::schema::messages::msg1::group;
    using group_t = sbepp::group_traits<group_tag>::value_type<byte_type>;
    static constexpr auto num_in_group = 3;
    static constexpr auto num_groups = 1;
    static constexpr auto num_var_data_fields = 1;
    group_t g{buf.data(), buf.size()};
    auto header = sbepp::fill_group_header(g, num_in_group);

    ASSERT_EQ(
        header.blockLength(), sbepp::group_traits<group_tag>::block_length());
    ASSERT_EQ(header.numInGroup(), num_in_group);
    ASSERT_EQ(header.numGroups(), num_groups);
    ASSERT_EQ(header.numVarDataFields(), num_var_data_fields);
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test()
{
    std::array<byte_type, 512> buf{};
    group_t g1;
    g1 = group_t{buf.data(), buf.size()};
    group_t g2{std::move(g1)};
    auto g3{g2};
    g2 = g1;
    g1 = std::move(g2);

    sbepp::fill_group_header(g3, 1);
    sbepp::get_header(g3);

    g3.sbe_size();
    g3.size();
    g3.resize(2);
    (void)g3.empty();
    g3.max_size();
    g3.begin();
    g3.end();
    g3[0];
    g3.front();
    g3.back();
    g3.clear();

    sbepp::addressof(g3);
    sbepp::size_bytes(g3);

    return buf;
}

constexpr auto buf = constexpr_test();
#endif
} // namespace
