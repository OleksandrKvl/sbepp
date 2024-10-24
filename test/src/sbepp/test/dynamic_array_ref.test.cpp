// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#ifdef USE_TOP_FILE
#    include <test_schema/test_schema.hpp>
#else
#    include <test_schema/types/uint32_req.hpp>
#endif

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <cstring>

namespace
{
template<typename Byte, typename Value, typename Length, sbepp::endian E>
using dynamic_array_ref =
    sbepp::detail::dynamic_array_ref<Byte, Value, Length, E>;

constexpr auto g_buf_size = 128;
using value_type = char;
using byte_type = std::uint8_t;
using sbe_length_type = test_schema::types::uint32_req;
using array_t = dynamic_array_ref<
    byte_type,
    value_type,
    sbe_length_type,
    sbepp::endian::native>;
using const_array_t = dynamic_array_ref<
    const byte_type,
    value_type,
    sbe_length_type,
    sbepp::endian::native>;

// member types test
// element_type is value_type with copied cv-qualifiers from byte_type
STATIC_ASSERT_V(std::is_same<const_array_t::element_type, const value_type>);
STATIC_ASSERT_V(std::is_same<const_array_t::value_type, value_type>);
STATIC_ASSERT_V(std::is_same<const_array_t::sbe_size_type, sbe_length_type>);
STATIC_ASSERT_V(
    std::is_same<const_array_t::size_type, sbe_length_type::value_type>);
STATIC_ASSERT_V(std::is_same<const_array_t::difference_type, std::ptrdiff_t>);
STATIC_ASSERT_V(
    std::is_same<const_array_t::reference, const const_array_t::element_type&>);
STATIC_ASSERT_V(
    std::is_same<const_array_t::pointer, const const_array_t::element_type*>);
STATIC_ASSERT_V(std::is_same<const_array_t::iterator, const_array_t::pointer>);
STATIC_ASSERT_V(std::is_same<
                const_array_t::reverse_iterator,
                std::reverse_iterator<const_array_t::pointer>>);

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

STATIC_ASSERT(sbepp::is_data<array_t>::value);

#if SBEPP_HAS_INLINE_VARS
STATIC_ASSERT(sbepp::is_data_v<array_t>);
#endif

#if SBEPP_HAS_CONCEPTS
STATIC_ASSERT(sbepp::data<array_t>);
#endif

class DynamicArrayRefTest : public ::testing::Test
{
public:
    std::array<std::uint8_t, g_buf_size> buf{};
    array_t a{buf.data(), buf.size()};
};

TEST_F(DynamicArrayRefTest, DefaultConstructedToNullptr)
{
    array_t a;

    ASSERT_EQ(sbepp::addressof(a), nullptr);
}

TEST_F(DynamicArrayRefTest, CanBeConstructedFromBeginEndPointers)
{
    array_t a{&*std::begin(buf), &*std::end(buf)};

    ASSERT_EQ(sbepp::addressof(a), buf.data());
    STATIC_ASSERT_V(std::is_nothrow_constructible<
                    array_t,
                    decltype(&*std::begin(buf)),
                    decltype(&*std::end(buf))>);
}

TEST_F(DynamicArrayRefTest, CanBeConstructedFromPointerAndSize)
{
    ASSERT_EQ(sbepp::addressof(a), buf.data());
    STATIC_ASSERT_V(std::is_nothrow_constructible<
                    array_t,
                    decltype(buf.data()),
                    decltype(buf.size())>);
}

TEST_F(DynamicArrayRefTest, CopyAndMoveCopyPointer)
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

TEST_F(DynamicArrayRefTest, CanBeConstructedFromLessConstType)
{
    STATIC_ASSERT_V(!std::is_nothrow_constructible<array_t, const_array_t>);
    STATIC_ASSERT_V(std::is_nothrow_constructible<const_array_t, array_t>);

    const_array_t a2{a};

    ASSERT_EQ(sbepp::addressof(a2), sbepp::addressof(a));

    a2 = array_t{};

    ASSERT_EQ(sbepp::addressof(a2), nullptr);
}

using DynamicArrayRefDeathTest = DynamicArrayRefTest;

TEST_F(DynamicArrayRefDeathTest, TerminatesIfAccessedWhileHoldsNullptr)
{
    array_t empty;

    ASSERT_DEATH({ empty.begin(); }, ".*");
    ASSERT_DEATH({ empty.end(); }, ".*");
    ASSERT_DEATH({ empty.rbegin(); }, ".*");
    ASSERT_DEATH({ empty.rend(); }, ".*");
    ASSERT_DEATH({ empty.front(); }, ".*");
    ASSERT_DEATH({ empty.back(); }, ".*");
    ASSERT_DEATH({ empty.data(); }, ".*");
    ASSERT_DEATH({ a[0]; }, ".*");
    ASSERT_DEATH({ empty.sbe_size(); }, ".*");
    ASSERT_DEATH({ empty.size(); }, ".*");
    ASSERT_DEATH(
        {
            auto e = empty.empty();
            (void)e;
        },
        ".*");
    ASSERT_DEATH({ empty.clear(); }, ".*");
    ASSERT_DEATH({ empty.resize(0); }, ".*");
    ASSERT_DEATH({ empty.resize(0, 0); }, ".*");
    ASSERT_DEATH({ empty.resize(0, sbepp::default_init); }, ".*");
    ASSERT_DEATH({ empty.push_back(0); }, ".*");
    ASSERT_DEATH({ empty.pop_back(); }, ".*");
    ASSERT_DEATH({ empty.assign(1, 'x'); }, ".*");
    ASSERT_DEATH({ empty.assign_string("abc"); }, ".*");
    ASSERT_DEATH({ empty.assign_range(std::string{}); }, ".*");
    // other functions require iterators
}

TEST_F(DynamicArrayRefDeathTest, TerminatesOnDataAccessIfBufferIsLessThanSize)
{
    // dynamic array always has length before data. In this particular case,
    // length has underlying type `std::uint32_t`
    // calculate max possible size for given buffer
    const auto max_size = buf.size() - sizeof(std::uint32_t);
    // NOLINTNEXTLINE: cast is intentional here
    auto length = reinterpret_cast<std::uint32_t*>(buf.data());
    // set length indirectly to a greater number
    *length = max_size + 1;

    // `size()` and `sbe_size()` requires only enough memory to read the length
    ASSERT_GT(a.size(), max_size);

    ASSERT_DEATH({ a.begin(); }, ".*");
    ASSERT_DEATH({ a.end(); }, ".*");
    ASSERT_DEATH({ a.rbegin(); }, ".*");
    ASSERT_DEATH({ a.rend(); }, ".*");
    ASSERT_DEATH({ a.front(); }, ".*");
    ASSERT_DEATH({ a.back(); }, ".*");
    ASSERT_DEATH({ a.data(); }, ".*");
    ASSERT_DEATH({ a[0]; }, ".*");
    ASSERT_DEATH({ a.push_back(0); }, ".*");
    // other functions require iterators
}

TEST_F(DynamicArrayRefTest, BeginEndRepresentsActualContent)
{
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(a.begin(), a.end());

    static constexpr auto new_size = 10;
    a.resize(new_size);

    ASSERT_EQ(
        static_cast<void*>(a.begin()), buf.data() + sizeof(sbe_length_type));
    ASSERT_EQ(a.end(), a.begin() + a.size());
    ASSERT_EQ(std::distance(a.begin(), a.end()), a.size());
    STATIC_ASSERT(noexcept(a.begin()));
    STATIC_ASSERT(noexcept(a.end()));
}

TEST_F(DynamicArrayRefDeathTest, BeginEndTerminatesIfHoldsNullptr)
{
    array_t a;
    ASSERT_EQ(sbepp::addressof(a), nullptr);

    ASSERT_DEATH({ a.begin(); }, ".*");
    ASSERT_DEATH({ a.end(); }, ".*");
}

TEST_F(DynamicArrayRefTest, ReversedBeginEndRepresentReversedContent)
{
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(a.rbegin(), a.rend());

    static constexpr auto new_size = 10;
    a.resize(new_size);

    ASSERT_EQ(static_cast<void*>(a.rend().operator->()), a.begin() - 1);
    ASSERT_EQ(static_cast<void*>(a.rbegin().operator->()), a.end() - 1);
    ASSERT_EQ(std::distance(a.rbegin(), a.rend()), a.size());
    STATIC_ASSERT(noexcept(a.rbegin()));
    STATIC_ASSERT(noexcept(a.rend()));
}

TEST_F(DynamicArrayRefDeathTest, ReversedBeginEndTerminatesIfHoldsNullptr)
{
    array_t a;
    ASSERT_EQ(sbepp::addressof(a), nullptr);

    ASSERT_DEATH({ a.rbegin(); }, ".*");
    ASSERT_DEATH({ a.rend(); }, ".*");
}

TEST_F(DynamicArrayRefTest, FrontReturnsReferenceToFirstElement)
{
    static constexpr value_type x{'x'};
    static constexpr value_type y{'y'};
    a.push_back(x);
    a.push_back(y);

    ASSERT_EQ(a.front(), x);
    STATIC_ASSERT(noexcept(a.front()));
    STATIC_ASSERT_V(std::is_same<decltype(a.front()), array_t::reference>);
}

TEST_F(DynamicArrayRefDeathTest, FrontTerminatesIfHoldsNullptr)
{
    array_t a;

    ASSERT_DEATH({ a.front(); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, FrontTerminatesIfEmpty)
{
    ASSERT_TRUE(a.empty());

    ASSERT_DEATH({ a.front(); }, ".*");
}

TEST_F(DynamicArrayRefTest, BackReturnsReferenceToLastElement)
{
    static constexpr value_type x{'x'};
    static constexpr value_type y{'y'};
    a.push_back(x);
    a.push_back(y);

    ASSERT_EQ(a.back(), y);
    STATIC_ASSERT(noexcept(a.back()));
    STATIC_ASSERT_V(std::is_same<decltype(a.back()), array_t::reference>);
}

TEST_F(DynamicArrayRefDeathTest, BackTerminatesIfHoldsNullptr)
{
    array_t a;

    ASSERT_DEATH({ a.back(); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, BackTerminatesIfEmpty)
{
    ASSERT_TRUE(a.empty());

    ASSERT_DEATH({ a.back(); }, ".*");
}

TEST_F(DynamicArrayRefTest, DataReturnsPointerToFirstElement)
{
    static constexpr value_type x{'x'};
    static constexpr value_type y{'y'};
    a.push_back(x);
    a.push_back(y);

    ASSERT_EQ(*a.data(), x);
    STATIC_ASSERT(noexcept(a.data()));
}

TEST_F(DynamicArrayRefTest, DataReturnsPointerAfterLengthIfEmpty)
{
    a.clear();
    ASSERT_TRUE(a.empty());

    ASSERT_EQ(
        reinterpret_cast<byte_type*>(a.data()),
        sbepp::addressof(a) + sizeof(sbe_length_type));
}

TEST_F(DynamicArrayRefTest, SubscriptReturnsReferenceToElement)
{
    static constexpr value_type x{'x'};
    static constexpr value_type y{'y'};
    a.push_back(x);
    a.push_back(y);

    ASSERT_EQ(a[0], x);
    ASSERT_EQ(a[1], y);
    STATIC_ASSERT(noexcept(a[0]));
    STATIC_ASSERT_V(std::is_same<decltype(a[0]), array_t::reference>);
}

TEST_F(DynamicArrayRefDeathTest, SubscriptTerminatesIfAccessedOutOfRange)
{
    a.push_back('x');
    static constexpr auto index = 1;
    ASSERT_GT(index + 1, a.size());

    ASSERT_DEATH({ a[index]; }, ".*");
}

TEST_F(DynamicArrayRefTest, SbeSizeReturnsSbeSizeRepresentation)
{
    static constexpr auto new_size = 10;
    a.resize(new_size);

    ASSERT_EQ(a.sbe_size(), sbe_length_type{new_size});
    STATIC_ASSERT(noexcept(a.sbe_size()));
    STATIC_ASSERT_V(std::is_same<decltype(a.sbe_size()), sbe_length_type>);
}

TEST_F(DynamicArrayRefTest, SbeSizeIsZeroByDefault)
{
    ASSERT_EQ(a.sbe_size(), 0);
}

TEST_F(DynamicArrayRefTest, SbeSizeReadsLengthFromBufferStart)
{
    static constexpr auto new_size = 10;
    *reinterpret_cast<sbe_length_type::value_type*>(buf.data()) = new_size;

    ASSERT_EQ(a.sbe_size(), sbe_length_type{new_size});
    STATIC_ASSERT(noexcept(a.sbe_size()));
    STATIC_ASSERT_V(
        std::is_same<decltype(a.sbe_size()), array_t::sbe_size_type>);
}

TEST_F(DynamicArrayRefTest, SizeReturnsSbeSizeValue)
{
    static constexpr auto new_size = 10;
    a.resize(new_size);

    ASSERT_EQ(a.size(), a.sbe_size().value());
    STATIC_ASSERT(noexcept(a.size()));
    STATIC_ASSERT_V(std::is_same<decltype(a.size()), array_t::size_type>);
}

TEST_F(DynamicArrayRefTest, EmptyReflectsSize)
{
    ASSERT_EQ(a.size(), 0);
    ASSERT_TRUE(a.empty());

    a.push_back('x');

    ASSERT_NE(a.size(), 0);
    ASSERT_FALSE(a.empty());

    STATIC_ASSERT(noexcept(a.empty()));
}

TEST_F(DynamicArrayRefTest, MaxSizeReturnsMaxValueOfSbeSizeType)
{
    ASSERT_EQ(array_t::max_size(), sbe_length_type::max_value());
    STATIC_ASSERT(noexcept(array_t::max_size()));
}

TEST_F(DynamicArrayRefTest, ClearSetsSizeToZero)
{
    a.resize(3);
    ASSERT_NE(a.size(), 0);

    a.clear();
    ASSERT_EQ(a.size(), 0);
}

struct test_clear
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.clear())
    {
    }
};

TEST_F(DynamicArrayRefTest, ClearNotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_clear, const_array_t>::value);
}

TEST_F(DynamicArrayRefTest, Resize1InsertsValueInitializedValues)
{
    static constexpr auto x = 'x';
    static constexpr auto y = 'y';
    a.push_back(x);
    a.push_back(y);
    auto new_size = a.size() + 1;

    a.resize(new_size);

    ASSERT_EQ(a.size(), new_size);
    ASSERT_EQ(a[0], x);
    ASSERT_EQ(a[1], y);
    ASSERT_EQ(a[2], value_type{});
    STATIC_ASSERT(noexcept(a.resize(new_size)));
}

TEST_F(DynamicArrayRefTest, Resize1CanDecreaseSize)
{
    static constexpr auto x = 'x';
    a.push_back(x);
    a.push_back('y');
    const auto old_size = a.size();
    const auto new_size = old_size - 1;

    a.resize(new_size);

    ASSERT_EQ(a.size(), new_size);
    ASSERT_EQ(a[0], x);
}

struct test_resize
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.resize(1))
    {
    }
};

TEST_F(DynamicArrayRefTest, Resize1NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_resize, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Resize1TerminatesIfNotEnoughMemory)
{
    // It's not possible to have `buf.size()` elements because `buf` should also
    // contain length
    ASSERT_DEATH({ a.resize(buf.size()); }, ".*");
}

TEST_F(DynamicArrayRefTest, Resize2InsertsProvideValue)
{
    static constexpr auto x = 'x';
    static constexpr auto y = 'y';
    a.push_back(x);
    a.push_back(y);
    auto new_size = a.size() + 1;

    a.resize(new_size, x);

    ASSERT_EQ(a.size(), new_size);
    ASSERT_EQ(a[0], x);
    ASSERT_EQ(a[1], y);
    ASSERT_EQ(a[2], x);
    STATIC_ASSERT(noexcept(a.resize(new_size, x)));
}

TEST_F(DynamicArrayRefTest, Resize2CanDecreaseSize)
{
    static constexpr auto x = 'x';
    a.push_back(x);
    a.push_back('y');
    const auto old_size = a.size();
    const auto new_size = old_size - 1;

    a.resize(new_size, x);

    ASSERT_EQ(a.size(), new_size);
    ASSERT_EQ(a[0], x);
}

struct test_resize2
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.resize(1, 'x'))
    {
    }
};

TEST_F(DynamicArrayRefTest, Resize2NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_resize2, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Resize2TerminatesIfNotEnoughMemory)
{
    // It's not possible to have `buf.size()` elements because `buf` should also
    // contain length
    ASSERT_DEATH({ a.resize(buf.size(), 'x'); }, ".*");
}

TEST_F(DynamicArrayRefTest, Resize3InsertsDefaultInitializedValues)
{
    static constexpr auto x = 'x';
    static constexpr auto y = 'y';
    auto data_ptr =
        reinterpret_cast<value_type*>(buf.data() + sizeof(sbe_length_type));
    data_ptr[0] = x;
    data_ptr[1] = y;
    static constexpr auto new_size = 2;

    a.resize(new_size, sbepp::default_init);

    ASSERT_EQ(a.size(), new_size);
    ASSERT_EQ(a[0], x);
    ASSERT_EQ(a[1], y);
    STATIC_ASSERT(noexcept(a.resize(new_size, sbepp::default_init)));
}

TEST_F(DynamicArrayRefTest, Resize3CanDecreaseSize)
{
    static constexpr auto x = 'x';
    a.push_back(x);
    a.push_back('y');
    const auto old_size = a.size();
    const auto new_size = old_size - 1;

    a.resize(new_size, sbepp::default_init);

    ASSERT_EQ(a.size(), new_size);
    ASSERT_EQ(a[0], x);
}

struct test_resize3
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.resize(1, sbepp::default_init))
    {
    }
};

TEST_F(DynamicArrayRefTest, Resize3NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_resize3, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Resize3TerminatesIfNotEnoughMemory)
{
    // It's not possible to have `buf.size()` elements because `buf` should also
    // contain length
    ASSERT_DEATH({ a.resize(buf.size(), sbepp::default_init); }, ".*");
}

TEST_F(DynamicArrayRefTest, PushBackAddsNewElement)
{
    static constexpr auto x = 'x';
    const auto old_size = a.size();

    a.push_back(x);

    const auto new_size = a.size();
    ASSERT_EQ(new_size - old_size, 1);
    ASSERT_EQ(a.back(), x);
    STATIC_ASSERT(noexcept(a.push_back(x)));
}

TEST_F(DynamicArrayRefDeathTest, PushBackTerminatesIfNotEnoughMemory)
{
    const auto max_size = buf.size() - sizeof(sbe_length_type);
    a.resize(max_size);

    ASSERT_DEATH({ a.push_back('x'); }, ".*");
}

TEST_F(DynamicArrayRefTest, PopBackRemovesLastElement)
{
    a.push_back('x');
    const auto old_size = a.size();

    a.pop_back();

    const auto new_size = a.size();
    ASSERT_EQ(old_size - new_size, 1);
}

TEST_F(DynamicArrayRefDeathTest, PopBackTerminatesIfEmpty)
{
    ASSERT_TRUE(a.empty());
    ASSERT_DEATH({ a.pop_back(); }, ".*");
}

TEST_F(DynamicArrayRefTest, Erase1RemovesElementFromPosition)
{
    static constexpr auto x = 'x';
    static constexpr auto y = 'y';
    static constexpr auto z = 'z';
    a.push_back(x);
    a.push_back(y);
    a.push_back(z);
    const auto old_size = a.size();
    auto pos = a.begin() + 1; // y

    const auto res = a.erase(pos);

    const auto new_size = a.size();
    ASSERT_EQ(old_size - new_size, 1);
    ASSERT_EQ(a[0], x);
    ASSERT_EQ(a[1], z);
    ASSERT_EQ(*res, z);
    STATIC_ASSERT(noexcept(a.erase(pos)));
}

struct test_erase1
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.erase(obj.begin()))
    {
    }
};

TEST_F(DynamicArrayRefTest, Erase1NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_erase1, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Erase1TerminatesIfPositionIsWrong)
{
    ASSERT_DEATH({ a.erase(a.begin()); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Erase1TerminatesIfHoldsNullptr)
{
    array_t a;
    ASSERT_DEATH({ a.erase(a.begin()); }, ".*");
}

TEST_F(DynamicArrayRefTest, Erase2RemovesRangeOfElements)
{
    static constexpr auto x = 'x';
    static constexpr auto y = 'y';
    static constexpr auto z = 'z';
    a.push_back(x);
    a.push_back(y);
    a.push_back(z);
    const auto old_size = a.size();
    const auto erase_begin = a.begin();
    const auto erase_end = a.begin() + 2;

    const auto res = a.erase(erase_begin, erase_end);

    const auto new_size = a.size();
    ASSERT_EQ(old_size - new_size, 2);
    ASSERT_EQ(a[0], z);
    ASSERT_EQ(*res, z);
    STATIC_ASSERT(noexcept(a.erase(erase_begin, erase_end)));
}

struct test_erase2
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.erase(obj.begin(), obj.end()))
    {
    }
};

TEST_F(DynamicArrayRefTest, Erase2NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_erase2, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Erase2TerminatesIfRangeIsWrong)
{
    ASSERT_DEATH({ a.erase(a.begin() - 1, a.end()); }, ".*");
    ASSERT_DEATH({ a.erase(a.begin(), a.end() + 1); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Erase2TerminatesIfHoldsNullptr)
{
    array_t a;
    ASSERT_DEATH({ a.erase(a.begin(), a.end()); }, ".*");
}

TEST_F(DynamicArrayRefTest, Insert1InsertsElementIntoPosition)
{
    static constexpr auto x = 'x';
    static constexpr auto y = 'y';
    static constexpr auto z = 'z';
    const auto old_size = a.size();

    a.insert(a.begin(), x);
    a.insert(a.end(), y);
    const auto res = a.insert(a.begin() + 1, z);

    const auto new_size = a.size();
    ASSERT_EQ(new_size - old_size, 3);
    ASSERT_EQ(a[0], x);
    ASSERT_EQ(a[1], z);
    ASSERT_EQ(a[2], y);
    ASSERT_EQ(*res, z);
    STATIC_ASSERT(noexcept(a.insert(a.begin(), z)));
}

struct test_insert1
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.insert(obj.begin(), 'x'))
    {
    }
};

TEST_F(DynamicArrayRefTest, Insert1NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_insert1, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Insert1TerminatesIfPositionIsWrong)
{
    ASSERT_DEATH({ a.insert(a.begin() - 1, 'x'); }, ".*");
    ASSERT_DEATH({ a.insert(a.end() + 1, 'x'); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Insert1TerminatesIfNotEnoughMemory)
{
    array_t a{buf.data(), sizeof(sbe_length_type)};
    ASSERT_DEATH({ a.insert(a.begin(), 'x'); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Insert1TerminatesIfHoldsNullptr)
{
    array_t a;
    ASSERT_DEATH({ a.insert(a.begin(), 'x'); }, ".*");
}

TEST_F(DynamicArrayRefTest, Insert2InsertsElementsIntoPosition)
{
    static constexpr auto x = 'x';
    static constexpr auto y = 'y';
    static constexpr auto z = 'z';
    const auto old_size = a.size();

    a.insert(a.begin(), 0, '0');
    a.insert(a.begin(), 2, x);
    a.insert(a.end(), 2, y);
    const auto res = a.insert(a.begin() + 2, 2, z);

    const auto new_size = a.size();
    ASSERT_EQ(new_size - old_size, 3 * 2);
    ASSERT_EQ(a[0], x);
    ASSERT_EQ(a[1], x);
    ASSERT_EQ(a[2], z);
    ASSERT_EQ(a[3], z);
    ASSERT_EQ(a[4], y);
    ASSERT_EQ(a[5], y);
    ASSERT_EQ(res, a.begin() + 2);
    STATIC_ASSERT(noexcept(a.insert(a.begin(), 1, 'x')));
}

struct test_insert2
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.insert(obj.begin(), 1, 'x'))
    {
    }
};

TEST_F(DynamicArrayRefTest, Insert2NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_insert2, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Insert2TerminatesIfPositionIsWrong)
{
    ASSERT_DEATH({ a.insert(a.begin() - 1, 1, 'x'); }, ".*");
    ASSERT_DEATH({ a.insert(a.end() + 1, 1, 'x'); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Insert2TerminatesIfNotEnoughMemory)
{
    array_t a{buf.data(), sizeof(sbe_length_type)};
    ASSERT_DEATH({ a.insert(a.begin(), 1, 'x'); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Insert2TerminatesIfHoldsNullptr)
{
    array_t a;
    ASSERT_DEATH({ a.insert(a.begin(), 1, 'x'); }, ".*");
}

TEST_F(DynamicArrayRefTest, Insert3InsertsElementsFromInputRange)
{
    std::string s{"xyz"};
    std::istringstream stream{s};
    const auto old_size = a.size();
    auto insert_begin = std::istream_iterator<char>{stream};
    auto insert_end = std::istream_iterator<char>{};

    const auto res = a.insert(a.begin(), insert_begin, insert_end);

    const auto new_size = a.size();
    ASSERT_EQ(new_size - old_size, s.size());
    ASSERT_EQ(s.size(), a.size());
    ASSERT_TRUE(std::equal(std::begin(s), std::end(s), std::begin(a)));
    ASSERT_EQ(res, a.begin());
}

TEST_F(DynamicArrayRefTest, Insert3InsertsElementsFromRandomAccessRange)
{
    std::string s{"xyz"};
    const auto old_size = a.size();

    const auto res = a.insert(a.begin(), std::begin(s), std::end(s));

    const auto new_size = a.size();
    ASSERT_EQ(new_size - old_size, s.size());
    ASSERT_EQ(s.size(), a.size());
    ASSERT_TRUE(std::equal(std::begin(s), std::end(s), std::begin(a)));
    ASSERT_EQ(res, a.begin());
}

struct test_insert3
{
    template<typename T>
    auto operator()(T obj)
        -> decltype(obj.insert(obj.begin(), (char*)nullptr, (char*)nullptr))
    {
    }
};

TEST_F(DynamicArrayRefTest, Insert3NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_insert3, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Insert3TerminatesIfPositionIsWrong)
{
    char* it{};
    ASSERT_DEATH({ a.insert(a.begin() - 1, it, it); }, ".*");
    ASSERT_DEATH({ a.insert(a.end() + 1, it, it); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Insert3TerminatesIfNotEnoughMemory)
{
    array_t a{buf.data(), sizeof(sbe_length_type) + 1};
    std::string s{"ab"};
    ASSERT_DEATH({ a.insert(a.begin(), std::begin(s), std::end(s)); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Insert3TerminatesIfHoldsNullptr)
{
    array_t a;
    char* it{};
    ASSERT_DEATH({ a.insert(a.begin(), it, it); }, ".*");
}

TEST_F(DynamicArrayRefTest, Insert4InsertsElementsFromInitializerList)
{
    const auto old_size = a.size();

    const auto res = a.insert(a.begin(), {'a', 'b'});

    const auto new_size = a.size();
    ASSERT_EQ(new_size - old_size, 2);
    ASSERT_EQ(a[0], 'a');
    ASSERT_EQ(a[1], 'b');
    ASSERT_EQ(res, a.begin());
    STATIC_ASSERT(noexcept(a.insert(a.begin(), {'a', 'b'})));
}

struct test_insert4
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.insert(obj.begin(), {'a', 'b'}))
    {
    }
};

TEST_F(DynamicArrayRefTest, Insert4NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_insert4, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Insert4TerminatesIfPositionIsWrong)
{
    ASSERT_DEATH({ a.insert(a.begin() - 1, {'a', 'b'}); }, ".*");
    ASSERT_DEATH({ a.insert(a.end() + 1, {'a', 'b'}); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Insert4TerminatesIfNotEnoughMemory)
{
    array_t a{buf.data(), sizeof(sbe_length_type) + 1};
    ASSERT_DEATH({ a.insert(a.begin(), {'a', 'b'}); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Insert4TerminatesIfHoldsNullptr)
{
    array_t a;
    ASSERT_DEATH({ a.insert(a.begin(), {'a', 'b'}); }, ".*");
}

TEST_F(DynamicArrayRefTest, Assign1AssignsValue)
{
    a.push_back('x');
    static constexpr auto count = 3;

    a.assign(count, 'x');

    ASSERT_EQ(a.size(), count);
    ASSERT_EQ(a[0], 'x');
    ASSERT_EQ(a[1], 'x');
    ASSERT_EQ(a[2], 'x');
    STATIC_ASSERT(noexcept(a.assign(count, 'x')));
}

struct test_assign1
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.assign(0, 'x'))
    {
    }
};

TEST_F(DynamicArrayRefTest, Assign1NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_assign1, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Assign1TerminatesIfNotEnoughMemory)
{
    ASSERT_DEATH({ a.assign(buf.size(), 'x'); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Assign1TerminatesIfHoldsNullptr)
{
    array_t a;
    ASSERT_DEATH({ a.assign(0, 'x'); }, ".*");
}

TEST_F(DynamicArrayRefTest, Assign2AssignsRange)
{
    std::string s{"abc"};
    std::istringstream stream{s};
    a.size();
    auto insert_begin = std::istream_iterator<char>{stream};
    auto insert_end = std::istream_iterator<char>{};
    a.push_back('x');

    a.assign(insert_begin, insert_end);

    ASSERT_EQ(a.size(), s.size());
    ASSERT_TRUE(std::equal(std::begin(s), std::end(s), std::begin(a)));
}

struct test_assign2
{
    template<typename T>
    auto operator()(T obj)
        -> decltype(obj.assign((char*)nullptr, (char*)nullptr))
    {
    }
};

TEST_F(DynamicArrayRefTest, Assign2NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_assign2, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Assign2TerminatesIfNotEnoughMemory)
{
    ASSERT_DEATH({ a.assign(&*std::begin(buf), &*std::end(buf)); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Assign2TerminatesIfHoldsNullptr)
{
    array_t a;
    char* it{};
    ASSERT_DEATH({ a.assign(it, it); }, ".*");
}

TEST_F(DynamicArrayRefTest, Assign3AssignsInitializerList)
{
    a.push_back('x');

    a.assign({'a', 'b'});

    ASSERT_EQ(a.size(), 2);
    ASSERT_EQ(a[0], 'a');
    ASSERT_EQ(a[1], 'b');
    STATIC_ASSERT(noexcept(a.assign({'a', 'b'})));
}

struct test_assign3
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.assign({'a', 'b'}))
    {
    }
};

TEST_F(DynamicArrayRefTest, Assign3NotAvailableForConstByteTypes)
{
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<test_assign3, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, Assign3TerminatesIfNotEnoughMemory)
{
    array_t a{buf.data(), sizeof(sbe_length_type) + 1};
    ASSERT_DEATH({ a.assign({'a', 'b'}); }, ".*");
}

TEST_F(DynamicArrayRefDeathTest, Assign3TerminatesIfHoldsNullptr)
{
    array_t a;
    ASSERT_DEATH({ a.assign({'a', 'b'}); }, ".*");
}

TEST_F(DynamicArrayRefTest, SizeBytesReturnsSizeIncludingLengthField)
{
    static constexpr auto new_size = 10;
    a.resize(new_size);

    ASSERT_EQ(sbepp::size_bytes(a), sizeof(a.sbe_size()) + new_size);
    STATIC_ASSERT(noexcept(sbepp::size_bytes(a)));
}

TEST_F(DynamicArrayRefTest, AssignStringCopiesFromString)
{
    const auto str = "abc";
    a.resize(10, 'x');

    a.assign_string(str);

    ASSERT_EQ(std::strlen(str), a.size());
    ASSERT_EQ(std::memcmp(str, a.data(), a.size()), 0);
    STATIC_ASSERT(noexcept(a.assign_string(str)));
}

struct test_assign_string
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.assign_string("abc"))
    {
    }
};

TEST_F(DynamicArrayRefTest, AssignStringNotAvailableForConstByteType)
{
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<test_assign_string, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, AssignStringTerminatesIfStringIsTooLong)
{
    std::string str;
    str.resize(g_buf_size + 1, 'x');

    ASSERT_DEATH({ a.assign_string(str.c_str()); }, ".*");
}

TEST_F(DynamicArrayRefTest, AssignRangeCopiesFromRange)
{
    std::vector<char> range;
    range.resize(10, 'x');

    a.assign_range(range);

    ASSERT_EQ(range.size(), a.size());
    ASSERT_EQ(std::memcmp(range.data(), a.data(), a.size()), 0);
}

struct test_assign_range
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.assign_range({'a', 'b'}))
    {
    }
};

TEST_F(DynamicArrayRefTest, AssignRangeNotAvailableForConstByteType)
{
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<test_assign_range, const_array_t>::value);
}

TEST_F(DynamicArrayRefDeathTest, AssignRangeTerminatesIfRangeIsTooLong)
{
    std::vector<char> range;
    range.resize(g_buf_size + 1, 'x');

    ASSERT_DEATH({ a.assign_range(range); }, ".*");
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test()
{
    std::array<byte_type, 512> buf{};
    std::array<byte_type, 16> range{};
    array_t a1;
    a1 = array_t{buf.data(), buf.size()};
    array_t a2;
    a2 = std::move(a1);
    auto a3{a2};
    auto a4{std::move(a3)};
    auto a5 = a4.raw();

    sbepp::addressof(a5);
    sbepp::size_bytes(a5);

    a5.begin();
    a5.end();
    a5.rbegin();
    a5.rend();
    a5.resize(10);
    a5.front();
    a5.back();
    a5.data();
    a5[1];
    a5.sbe_size();
    a5.size();
    (void)a5.empty();
    a5.max_size();
    a5.clear();
    a5.resize(10, 'x');
    a5.resize(10, sbepp::default_init);
    a5.push_back('x');
    a5.pop_back();
    a5.erase(a5.begin());
    a5.erase(a5.begin(), a5.begin() + 1);
    a5.insert(a5.begin(), 'x');
    a5.insert(a5.begin(), 3, 'x');
    auto str = "string";
    a5.insert(a5.begin(), str, str + 2);
    a5.insert(a5.begin(), {'a', 'b'});
    a5.assign(1, 'a');
    a5.assign(str, str + 2);
    a5.assign({'a', 'b'});
    a5.assign_string("abc");
    a5.assign_range(range);

    return buf;
}

constexpr auto buf = constexpr_test();
#endif
} // namespace
