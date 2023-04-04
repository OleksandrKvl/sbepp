// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#ifdef USE_TOP_FILE
#    include <test_schema/test_schema.hpp>
#else
#    include <test_schema/types/composite_a.hpp>
#    include <test_schema/types/composite_b.hpp>
#    include <test_schema/types/refs_composite.hpp>
#    include <test_schema/types/composite_10.hpp>
#endif

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <type_traits>

namespace
{
using composite_t = test_schema::types::composite_a<char>;
using const_composite_t = test_schema::types::composite_a<const char>;

#if SBEPP_SIZE_CHECKS_ENABLED
STATIC_ASSERT(sizeof(composite_t) == sizeof(void*) * 2);
#else
STATIC_ASSERT(sizeof(composite_t) == sizeof(void*));
#endif

STATIC_ASSERT_V(std::is_nothrow_default_constructible<composite_t>);
STATIC_ASSERT_V(std::is_trivially_copy_constructible<composite_t>);
STATIC_ASSERT_V(std::is_trivially_copy_assignable<composite_t>);
STATIC_ASSERT_V(std::is_trivially_move_constructible<composite_t>);
STATIC_ASSERT_V(std::is_trivially_move_assignable<composite_t>);
STATIC_ASSERT_V(std::is_trivially_destructible<composite_t>);

STATIC_ASSERT(sbepp::is_composite<composite_t>::value);

#if SBEPP_HAS_INLINE_VARS
STATIC_ASSERT(sbepp::is_composite_v<composite_t>);
#endif

#if SBEPP_HAS_CONCEPTS
STATIC_ASSERT(sbepp::composite<composite_t>);
#endif

class CompositeTest : public ::testing::Test
{
public:
    std::array<char, 100> buf{};
};

TEST_F(CompositeTest, DefaultConstructedToNullptr)
{
    composite_t c;

    ASSERT_EQ(sbepp::addressof(c), nullptr);
}

TEST_F(CompositeTest, CanBeConstructedFromBeginEndPointers)
{
    composite_t c{std::begin(buf), std::end(buf)};

    ASSERT_EQ(sbepp::addressof(c), buf.data());
    STATIC_ASSERT_V(std::is_nothrow_constructible<
                    composite_t,
                    decltype(std::begin(buf)),
                    decltype(std::end(buf))>);
}

TEST_F(CompositeTest, CanBeConstructedFromPointerAndSize)
{
    composite_t c{buf.data(), buf.size()};

    ASSERT_EQ(sbepp::addressof(c), buf.data());
    STATIC_ASSERT_V(std::is_nothrow_constructible<
                    composite_t,
                    decltype(buf.data()),
                    decltype(buf.size())>);
}

TEST_F(CompositeTest, CopyAndMoveCopyPointer)
{
    composite_t c{buf.data(), buf.size()};
    auto c2{c};

    ASSERT_EQ(sbepp::addressof(c2), sbepp::addressof(c));

    composite_t c3;
    c2 = c3;

    ASSERT_EQ(sbepp::addressof(c2), sbepp::addressof(c3));

    auto c4{std::move(c)};

    ASSERT_EQ(sbepp::addressof(c4), sbepp::addressof(c));

    c = std::move(c3);

    ASSERT_EQ(sbepp::addressof(c), sbepp::addressof(c3));
}

TEST_F(CompositeTest, CanBeConstructedFromLessConstType)
{
    STATIC_ASSERT_V(
        !std::is_nothrow_constructible<composite_t, const_composite_t>);
    STATIC_ASSERT_V(
        std::is_nothrow_constructible<const_composite_t, composite_t>);

    composite_t c{buf.data(), buf.size()};
    const_composite_t c2{c};

    ASSERT_EQ(sbepp::addressof(c2), sbepp::addressof(c));

    c2 = composite_t{};

    ASSERT_EQ(sbepp::addressof(c2), nullptr);
}

using CompositeDeathTest = CompositeTest;

TEST_F(CompositeDeathTest, TerminatesIfAccessedOutOfRange)
{
    using first_field_t = std::uint32_t;
    // provided buffer is enough only to represent the `x` field, not `y`
    composite_t c{buf.data(), sizeof(first_field_t)};
    c.x(2);

    ASSERT_EQ(c.x(), 2);

    ASSERT_DEATH({ c.y(); }, ".*");
}

template<typename Composite>
class CompositeStorage : public ::testing::Test
{
public:
    std::array<char, 256> buf{};
    Composite composite{buf.data(), buf.size()};
};

// refs provide exactly the same interface as inline-defined types
using MutableComposites = ::testing::Types<
    test_schema::types::composite_b<char>,
    test_schema::types::refs_composite<char>>;

template<typename T>
using MutableCompositeAccessorsTest = CompositeStorage<T>;

using ImmutableComposites = ::testing::Types<
    test_schema::types::composite_b<const char>,
    test_schema::types::refs_composite<const char>>;

template<typename T>
using ImmutableCompositeAccessorsTest = CompositeStorage<T>;

using AllComposites = ::testing::Types<
    test_schema::types::composite_b<char>,
    test_schema::types::refs_composite<char>,
    test_schema::types::composite_b<const char>,
    test_schema::types::refs_composite<const char>>;

template<typename T>
using CompositeAccessorsTest = CompositeStorage<T>;

TYPED_TEST_SUITE(MutableCompositeAccessorsTest, MutableComposites);
TYPED_TEST_SUITE(ImmutableCompositeAccessorsTest, ImmutableComposites);
TYPED_TEST_SUITE(CompositeAccessorsTest, AllComposites);

TYPED_TEST(CompositeAccessorsTest, NonArrayTypeGettersReturnByValue)
{
    // composite has reference semantics, hence all getters/setters are `const`
    const auto& c = this->composite;
    auto n = c.number();
    (void)n;

    STATIC_ASSERT_V(!std::is_reference<decltype(c.number())>);
    STATIC_ASSERT(noexcept(c.number()));
}

TYPED_TEST(MutableCompositeAccessorsTest, NonArrayTypeSettersTakeByValue)
{
    const auto& c = this->composite;
    auto n = c.number();
    n = 3;

    c.number(n);

    ASSERT_EQ(c.number(), n);
    STATIC_ASSERT(noexcept(c.number(n)));
}

struct set_number
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.number(obj.number()))
    {
    }
};

TYPED_TEST(
    ImmutableCompositeAccessorsTest, ImmutableCompositesDontProvideTypeSetters)
{
    STATIC_ASSERT(
        !sbepp::test::utils::
            is_invocable<set_number, decltype(this->composite)>::value);
}

TYPED_TEST(CompositeAccessorsTest, ArrayTypeGettersReturnByValue)
{
    const auto& c = this->composite;
    auto arr = c.array();
    (void)arr;

    STATIC_ASSERT_V(!std::is_reference<decltype(c.array())>);
    STATIC_ASSERT(noexcept(c.array()));
}

struct set_array
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.array(obj.array()))
    {
    }
};

TYPED_TEST(CompositeAccessorsTest, ArrayTypesDontHaveSetters)
{
    STATIC_ASSERT(
        !sbepp::test::utils::
            is_invocable<set_array, decltype(this->composite)>::value);
}

TYPED_TEST(CompositeAccessorsTest, ArrayTypeGettersPreservePointerType)
{
    const auto& c = this->composite;
    auto arr = c.array();

    STATIC_ASSERT_V(std::is_same<
                    decltype(sbepp::addressof(c)),
                    decltype(sbepp::addressof(arr))>);
}

TYPED_TEST(CompositeAccessorsTest, CompositeGettersReturnByValue)
{
    const auto& c = this->composite;
    auto c2 = c.composite();
    (void)c2;

    STATIC_ASSERT_V(!std::is_reference<decltype(c.composite())>);
    STATIC_ASSERT(noexcept(c.composite()));
}

struct set_composite
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.composite(obj.composite()))
    {
    }
};

TYPED_TEST(CompositeAccessorsTest, CompositesDontHaveSetters)
{
    STATIC_ASSERT(
        !sbepp::test::utils::
            is_invocable<set_composite, decltype(this->composite)>::value);
}

TYPED_TEST(CompositeAccessorsTest, CompositeGettersPreservePointerType)
{
    const auto& c = this->composite;
    auto c2 = c.composite();

    STATIC_ASSERT_V(std::is_same<
                    decltype(sbepp::addressof(c)),
                    decltype(sbepp::addressof(c2))>);
}

TYPED_TEST(CompositeAccessorsTest, EnumGettersReturnByValue)
{
    const auto& c = this->composite;
    auto e = c.enumeration();
    (void)e;

    STATIC_ASSERT_V(!std::is_reference<decltype(c.enumeration())>);
    STATIC_ASSERT(noexcept(c.enumeration()));
}

TYPED_TEST(MutableCompositeAccessorsTest, EnumSettersTakeByValue)
{
    const auto& c = this->composite;
    auto e = c.enumeration();
    e = decltype(e)::One;

    c.enumeration(e);

    ASSERT_EQ(c.enumeration(), e);
    STATIC_ASSERT(noexcept(c.enumeration(e)));
}

struct set_enum
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.enumeration(obj.enumeration()))
    {
    }
};

TYPED_TEST(
    ImmutableCompositeAccessorsTest, ImmutableCompositesDontProvideEnumSetters)
{
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_enum, decltype(this->composite)>::value);
}

TYPED_TEST(CompositeAccessorsTest, SetGettersReturnByValue)
{
    const auto& c = this->composite;
    auto s = c.set();
    (void)s;

    STATIC_ASSERT_V(!std::is_reference<decltype(c.set())>);
    STATIC_ASSERT(noexcept(c.set()));
}

TYPED_TEST(MutableCompositeAccessorsTest, SetSettersTakeByValue)
{
    const auto& c = this->composite;
    auto s = c.set();
    s.A(true);

    c.set(s);

    ASSERT_EQ(c.set(), s);
    STATIC_ASSERT(noexcept(c.set(s)));
}

struct set_set
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.set(obj.set()))
    {
    }
};

TYPED_TEST(
    ImmutableCompositeAccessorsTest, ImmutableCompositesDontProvideSetSetters)
{
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_set, decltype(this->composite)>::value);
}

TEST(SizeBytesTest, SizeBytesReturnsSizeOfAllFields)
{
    // use composite_11 to test with custom offsets?
    std::array<char, 256> buf{};
    test_schema::types::composite_a<char> c{buf.data(), buf.size()};
    const auto total_fields_size = sizeof(c.x()) + sizeof(c.y());

    ASSERT_EQ(sbepp::size_bytes(c), total_fields_size);
    STATIC_ASSERT(noexcept(sbepp::size_bytes(c)));
}

TEST(SizeBytesTest, SizeBytesTakesCustomOffsetsIntoAccount)
{
    // `composite_10` contains a single static array field
    std::array<char, 256> buf{};
    test_schema::types::composite_10<char> c{buf.data(), buf.size()};
    auto field = c.field();
    const auto correct_size =
        sbepp::addressof(field) + field.size() - sbepp::addressof(c);

    ASSERT_EQ(sbepp::size_bytes(c), correct_size);
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test()
{
    using composite_t = test_schema::types::refs_composite<char>;
    std::array<char, 512> buf{};
    composite_t c1;
    c1 = composite_t{buf.data(), buf.size()};
    auto c2 = std::move(c1);
    composite_t c3{std::move(c2)};
    auto c4{c3};
    c3.composite();
    c3.array();
    c3.enumeration();
    c3.number();
    auto s = c3.set();
    c3.enumeration(test_schema::types::numbers_enum::One);
    c3.set(s.A(true));
    c3.number(3);
    sbepp::size_bytes(c3);
    sbepp::addressof(c3);

    return buf;
}

constexpr auto buf = constexpr_test();
#endif
} // namespace
