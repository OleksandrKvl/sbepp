// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <sbepp/sbepp.hpp>
#include <sbepp/test/utils.hpp>

#include "gtest/gtest.h"
#include <gtest/gtest.h>

#include <iterator>
#include <type_traits>

namespace
{
template<typename Byte, typename Value, std::size_t N>
using static_array_ref = sbepp::detail::static_array_ref<Byte, Value, N>;

constexpr auto g_array_size = 128;
using value_type = char;
using byte_type = std::uint8_t;
using array_t = static_array_ref<byte_type, value_type, g_array_size>;
using const_array_t =
    static_array_ref<const byte_type, value_type, g_array_size>;

// member types test
// element_type is value_type with copied cv-qualifiers from byte_type
STATIC_ASSERT_V(std::is_same<const_array_t::element_type, const value_type>);
STATIC_ASSERT_V(std::is_same<const_array_t::value_type, value_type>);
STATIC_ASSERT_V(std::is_same<const_array_t::size_type, std::size_t>);
STATIC_ASSERT_V(std::is_same<const_array_t::difference_type, std::ptrdiff_t>);
STATIC_ASSERT_V(
    std::is_same<const_array_t::pointer, const const_array_t::element_type*>);
STATIC_ASSERT_V(
    std::is_same<const_array_t::reference, const const_array_t::element_type&>);
STATIC_ASSERT_V(std::is_same<const_array_t::iterator, const_array_t::pointer>);
STATIC_ASSERT_V(std::is_same<
                const_array_t::reverse_iterator,
                std::reverse_iterator<const_array_t::pointer>>);
STATIC_ASSERT(sbepp::is_array_type<array_t>::value);
STATIC_ASSERT(sbepp::is_type<array_t>::value);

#if SBEPP_HAS_INLINE_VARS
STATIC_ASSERT(sbepp::is_array_type_v<array_t>);
STATIC_ASSERT(sbepp::is_type_v<array_t>);
#endif

#if SBEPP_HAS_CONCEPTS
STATIC_ASSERT(sbepp::array_type<array_t>);
STATIC_ASSERT(sbepp::type<array_t>);
#endif

#if SBEPP_SIZE_CHECKS_ENABLED
STATIC_ASSERT(sizeof(array_t) == sizeof(void*) * 2);
#else
STATIC_ASSERT(sizeof(array_t) == sizeof(void*));
#endif

STATIC_ASSERT_V(std::is_nothrow_default_constructible<array_t>);
STATIC_ASSERT_V(std::is_trivially_copy_constructible<array_t>);
STATIC_ASSERT_V(std::is_trivially_copy_assignable<array_t>);
STATIC_ASSERT_V(std::is_trivially_move_constructible<array_t>);
STATIC_ASSERT_V(std::is_trivially_move_assignable<array_t>);
STATIC_ASSERT_V(std::is_trivially_destructible<array_t>);

class StaticArrayRefTest : public ::testing::Test
{
public:
    std::array<std::uint8_t, g_array_size> buf{};
    array_t a{buf.data(), buf.size()};
};

TEST_F(StaticArrayRefTest, DefaultConstructedToNullptr)
{
    array_t a;

    ASSERT_EQ(sbepp::addressof(a), nullptr);
}

TEST_F(StaticArrayRefTest, CanBeConstructedFromBeginEndPointers)
{
    ASSERT_EQ(sbepp::addressof(a), buf.data());
    STATIC_ASSERT_V(std::is_nothrow_constructible<
                    array_t,
                    decltype(std::begin(buf)),
                    decltype(std::end(buf))>);
}

TEST_F(StaticArrayRefTest, CanBeConstructedFromPointerAndSize)
{
    ASSERT_EQ(sbepp::addressof(a), buf.data());
    STATIC_ASSERT_V(std::is_nothrow_constructible<
                    array_t,
                    decltype(buf.data()),
                    decltype(buf.size())>);
}

TEST_F(StaticArrayRefTest, CopyAndMoveCopyPointer)
{
    auto a2{a};

    ASSERT_EQ(sbepp::addressof(a2), sbepp::addressof(a));

    array_t a3;
    a2 = a3;

    ASSERT_EQ(sbepp::addressof(a2), sbepp::addressof(a3));

    auto a4{std::move(a)};

    ASSERT_EQ(sbepp::addressof(a4), sbepp::addressof(a));

    a = std::move(a3);

    ASSERT_EQ(sbepp::addressof(a), sbepp::addressof(a3));
}

TEST_F(StaticArrayRefTest, CanBeConstructedFromLessConstType)
{
    STATIC_ASSERT_V(!std::is_nothrow_constructible<array_t, const_array_t>);
    STATIC_ASSERT_V(std::is_nothrow_constructible<const_array_t, array_t>);

    const_array_t a2{a};

    ASSERT_EQ(sbepp::addressof(a2), sbepp::addressof(a));

    a2 = array_t{};

    ASSERT_EQ(sbepp::addressof(a2), nullptr);
}

using StaticArrayRefDeathTest = StaticArrayRefTest;

TEST_F(StaticArrayRefDeathTest, TerminatesIfAccessedWithoutEnoughStorage)
{
    static constexpr auto not_enough_size = array_t::size() - 1;
    // provided pointer/size is not checked in any way until the actual data
    // access
    array_t a{buf.data(), not_enough_size};

    ASSERT_DEATH({ a[0]; }, ".*");
    ASSERT_DEATH({ a.front(); }, ".*");
    ASSERT_DEATH({ a.back(); }, ".*");
    ASSERT_DEATH({ a.data(); }, ".*");
    ASSERT_DEATH({ a.begin(); }, ".*");
    ASSERT_DEATH({ a.end(); }, ".*");
    ASSERT_DEATH({ a.rbegin(); }, ".*");
    ASSERT_DEATH({ a.rend(); }, ".*");
}

TEST_F(StaticArrayRefTest, SubscriptReturnsReference)
{
    static constexpr auto x = 'x';

    a[0] = x;

    ASSERT_EQ(a[0], x);
    STATIC_ASSERT_V(std::is_lvalue_reference<decltype(a[0])>);
    STATIC_ASSERT(noexcept(a[0]));
}

TEST_F(StaticArrayRefDeathTest, SubscriptTerminatesIfAccessedOutOfRange)
{
    static constexpr auto x = 'x';

    a[0] = x;

    ASSERT_DEATH({ a[a.size()]; }, ".*");
}

TEST_F(StaticArrayRefTest, FrontReturnsReferenceToFirstElement)
{
    static constexpr auto x = 'x';
    static constexpr auto y = 'y';

    buf.front() = x;

    ASSERT_EQ(a.front(), x);
    STATIC_ASSERT_V(std::is_lvalue_reference<decltype(a.front())>);
    STATIC_ASSERT(noexcept(a.front()));

    a.front() = y;

    ASSERT_EQ(a.front(), y);
}

TEST_F(StaticArrayRefTest, BackReturnsReferenceToLastElement)
{
    static constexpr auto x = 'x';
    static constexpr auto y = 'y';

    buf.back() = x;

    ASSERT_EQ(a.back(), x);
    STATIC_ASSERT_V(std::is_lvalue_reference<decltype(a.back())>);
    STATIC_ASSERT(noexcept(a.back()));

    a.back() = y;

    ASSERT_EQ(a.back(), y);
}

TEST_F(StaticArrayRefTest, DataReturnsPointerToFirstElement)
{
    ASSERT_EQ(a.data(), static_cast<void*>(buf.data()));
    STATIC_ASSERT_V(std::is_pointer<decltype(a.data())>);
    STATIC_ASSERT(noexcept(a.data()));
}

TEST_F(StaticArrayRefTest, SizeReturnsArraySize)
{
    STATIC_ASSERT(array_t::size() == g_array_size);
    STATIC_ASSERT(noexcept(array_t::size()));
}

TEST_F(StaticArrayRefTest, EmptyReflectsSize)
{
    STATIC_ASSERT(array_t::size() != 0);
    STATIC_ASSERT(array_t::empty() == false);
    STATIC_ASSERT(noexcept(array_t::empty()));

    using empty_array_t = static_array_ref<char, char, 0>;

    STATIC_ASSERT(empty_array_t::size() == 0);
    STATIC_ASSERT(empty_array_t::empty() == true);
}

TEST_F(StaticArrayRefTest, MaxSizeReturnsSize)
{
    STATIC_ASSERT(array_t::max_size() == array_t::size());
    STATIC_ASSERT(noexcept(array_t::max_size()));
}

TEST_F(StaticArrayRefTest, BeginEndRepresentBuffer)
{
    ASSERT_EQ(static_cast<void*>(a.begin()), buf.data());
    ASSERT_EQ(static_cast<void*>(a.end()), buf.data() + buf.size());
    ASSERT_EQ(std::distance(a.begin(), a.end()), buf.size());
    STATIC_ASSERT(noexcept(a.begin()));
    STATIC_ASSERT(noexcept(a.end()));
}

TEST_F(StaticArrayRefTest, ReversedBeginEndRepresentReversedBuffer)
{
    ASSERT_EQ(static_cast<void*>(a.rend().operator->()), buf.data() - 1);
    ASSERT_EQ(
        static_cast<void*>(a.rbegin().operator->()),
        buf.data() + buf.size() - 1);
    ASSERT_EQ(std::distance(a.rbegin(), a.rend()), buf.size());
    STATIC_ASSERT(noexcept(a.rbegin()));
    STATIC_ASSERT(noexcept(a.rend()));
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test()
{
    std::array<byte_type, 512> buf{};
    array_t a1;
    a1 = array_t{buf.data(), buf.size()};
    array_t a2;
    a2 = std::move(a1);
    auto a3{a2};
    auto a4{std::move(a3)};
    auto a5 = a4.raw();

    sbepp::size_bytes(a5);
    sbepp::addressof(a5);
    a5[0] = 'x';
    a5.front() = 'x';
    a5.back() = 'y';
    a5.data();
    (void)a5.empty();
    a5.size();
    a5.max_size();
    a5.begin();
    a5.end();
    a5.rbegin();
    a5.rend();

    return buf;
}

constexpr auto buf = constexpr_test();
#endif
} // namespace
