// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <test_schema/types/uint32_opt.hpp>

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace
{
using type_t = test_schema::types::uint32_opt;
using value_type = type_t::value_type;

STATIC_ASSERT(sizeof(type_t) == sizeof(value_type));
STATIC_ASSERT(
    std::alignment_of<type_t>::value == std::alignment_of<value_type>::value);

STATIC_ASSERT_V(std::is_nothrow_default_constructible<type_t>);
STATIC_ASSERT_V(std::is_trivially_copy_constructible<type_t>);
STATIC_ASSERT_V(std::is_trivially_copy_assignable<type_t>);
STATIC_ASSERT_V(std::is_trivially_move_constructible<type_t>);
STATIC_ASSERT_V(std::is_trivially_move_assignable<type_t>);
STATIC_ASSERT_V(std::is_trivially_destructible<type_t>);

STATIC_ASSERT(sbepp::is_optional_type<type_t>::value);
STATIC_ASSERT(sbepp::is_non_array_type<type_t>::value);
STATIC_ASSERT(sbepp::is_type<type_t>::value);

#if SBEPP_HAS_INLINE_VARS
STATIC_ASSERT(sbepp::is_optional_type_v<type_t>);
STATIC_ASSERT(sbepp::is_non_array_type_v<type_t>);
STATIC_ASSERT(sbepp::is_type_v<type_t>);
#endif

#if SBEPP_HAS_CONCEPTS
STATIC_ASSERT(sbepp::optional_type<type_t>);
STATIC_ASSERT(sbepp::non_array_type<type_t>);
STATIC_ASSERT(sbepp::type<type_t>);
#endif

TEST(OptionalTest, NullByDefault)
{
    type_t t;

    ASSERT_EQ(*t, type_t::null_value());
    ASSERT_FALSE(t.has_value());
}

TEST(OptionalTest, ImplicitlyConstructibleFromValueType)
{
    static constexpr value_type value{1};
    type_t t = value;

    ASSERT_EQ(*t, value);
}

TEST(OptionalTest, ImplicitlyConstructibleFromNullopt)
{
    type_t t = sbepp::nullopt;

    ASSERT_FALSE(t.has_value());
}

TEST(OptionalTest, ValueReturnsCurrentValue)
{
    static constexpr value_type value{2};
    type_t t{2};
    const auto& crt = t;

    STATIC_ASSERT_V(!std::is_reference<decltype(t.value())>);
    STATIC_ASSERT_V(!std::is_reference<decltype(crt.value())>);

    ASSERT_EQ(crt.value(), value);
    ASSERT_EQ(t.value(), crt.value());
}

TEST(OptionalTest, DereferenceReturnsCurrentValue)
{
    static constexpr value_type value{2};
    type_t t{2};
    const auto& crt = t;

    STATIC_ASSERT_V(std::is_lvalue_reference<decltype(*t)>);
    STATIC_ASSERT_V(!std::is_reference<decltype(*crt)>);

    ASSERT_EQ(*t, value);
    ASSERT_EQ(*t, *crt);
}

TEST(OptionalTest, ModifiableThroughDereference)
{
    static constexpr value_type value{2};
    type_t t;

    ASSERT_EQ(*t, type_t::null_value());

    *t = value;

    ASSERT_EQ(*t, value);
}

TEST(OptionalTest, InRangeReturnsTrueIfValueIsBetweenMinAndMax)
{
    type_t t;
    // NOLINTNEXTLINE: it's here for demonstrational purpose
    *t = t.min_value();

    ASSERT_TRUE(t.in_range());

    // `min/max_value()` are static functions
    *t = type_t::max_value() + 1;

    ASSERT_FALSE(t.in_range());
}

TEST(OptionalTest, HasValueReturnsTrueIfNotNull)
{
    type_t t;

    ASSERT_FALSE(t.has_value());

    t = 1;

    ASSERT_TRUE(t.has_value());
}

TEST(OptionalTest, ConvertsToTrueIfNotNull)
{
    type_t t;

    ASSERT_FALSE(t);

    t = 1;

    ASSERT_TRUE(t);
}

TEST(OptionalTest, ValueOrReturnsValueIfNotNull)
{
    static constexpr value_type value{2};
    type_t t{value};

    ASSERT_EQ(t.value_or(0), value);
}

TEST(OptionalTest, ValueOrReturnsGivenValueIfNull)
{
    static constexpr value_type value{2};
    type_t t;

    ASSERT_EQ(t.value_or(value), value);
}

TEST(OptionalTest, CopyAndMoveCopyValue)
{
    static constexpr value_type value1{2};
    static constexpr value_type value2{3};
    type_t t{value1};
    type_t t2{t};

    ASSERT_EQ(*t, value1);
    ASSERT_EQ(*t2, value1);

    *t = value2;
    t2 = t;

    ASSERT_EQ(*t, value2);
    ASSERT_EQ(*t2, value2);

    // NOLINTNEXTLINE: move constructor test
    type_t t3{std::move(t)};

    ASSERT_EQ(*t3, value2);

    *t2 = value1;
    // NOLINTNEXTLINE: move assignment test
    t3 = std::move(t2);

    ASSERT_EQ(*t3, value1);
}

TEST(OptionalTest, HasValueBasedComparisonOps)
{
    type_t one{1};
    type_t two{2};
    type_t null;

    ASSERT_EQ(one, one);
    ASSERT_NE(one, two);
    ASSERT_LT(one, two);
    ASSERT_LE(one, two);
    ASSERT_LE(one, one);
    ASSERT_GT(two, one);
    ASSERT_GE(two, one);
    ASSERT_GE(two, two);

    ASSERT_EQ(null, null);
    ASSERT_NE(null, one);
    ASSERT_LT(null, one);
    ASSERT_LE(null, one);
    ASSERT_LE(null, null);
    ASSERT_GT(one, null);
    ASSERT_GE(one, null);
    ASSERT_GE(null, null);
}

#if defined(__cpp_impl_three_way_comparison) \
    && defined(__cpp_lib_three_way_comparison)
#    if (__cpp_impl_three_way_comparison >= 201907L) \
        && (__cpp_lib_three_way_comparison >= 201907L)
TEST(OptionalTest, SupportsThreeWayComparisons)
{
    type_t one{1};
    type_t two{2};
    type_t null;

    STATIC_ASSERT_V(std::is_same<
                    std::compare_three_way_result_t<type_t, type_t>,
                    std::strong_ordering>);

    ASSERT_EQ(one <=> one, std::strong_ordering::equal);
    ASSERT_EQ(one <=> two, std::strong_ordering::less);
    ASSERT_EQ(two <=> one, std::strong_ordering::greater);

    ASSERT_EQ(null <=> null, std::strong_ordering::equal);
    ASSERT_EQ(null <=> one, std::strong_ordering::less);
    ASSERT_EQ(one <=> null, std::strong_ordering::greater);
}

#        if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test1()
{
    type_t a{1};
    type_t b{2};

    return a <=> b;
}

constexpr auto res1 = constexpr_test1();
#        endif
#    endif
#endif

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test2()
{
    type_t x;
    x = type_t{1};
    auto x2 = type_t{sbepp::nullopt};
    (void)x2;
    x.min_value();
    x.max_value();
    x.null_value();
    x.value();
    *x = 2;
    x.in_range();
    x.value_or(2);
    x.has_value();
    !!x;

    (void)(x == x);
    (void)(x != x);
    (void)(x > x);
    (void)(x >= x);
    (void)(x < x);
    (void)(x <= x);

    return x;
}

constexpr auto res2 = constexpr_test2();
#endif
} // namespace
