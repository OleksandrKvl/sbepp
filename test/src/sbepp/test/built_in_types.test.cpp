// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <sbepp/sbepp.hpp>

#include <gtest/gtest.h>

#include <type_traits>
#include <cstdint>
#include <limits>
#include <cmath>

namespace
{
template<typename T, typename = void>
struct built_in_limits
{
    static constexpr T min()
    {
        return std::numeric_limits<T>::min() + 1;
    }

    static constexpr T max()
    {
        return std::numeric_limits<T>::max();
    }

    static constexpr T null()
    {
        return std::numeric_limits<T>::min();
    }
};

template<>
struct built_in_limits<char>
{
    static constexpr char min()
    {
        return 0x20;
    }

    static constexpr char max()
    {
        return 0x7E;
    }

    static constexpr char null()
    {
        return 0;
    }
};

template<typename T>
struct built_in_limits<
    T,
    typename std::enable_if<std::is_unsigned<T>::value>::type>
{
    static constexpr T min()
    {
        return std::numeric_limits<T>::min();
    }

    static constexpr T max()
    {
        return std::numeric_limits<T>::max() - 1;
    }

    static constexpr T null()
    {
        return std::numeric_limits<T>::max();
    }
};

template<typename T>
struct built_in_limits<
    T,
    typename std::enable_if<std::is_floating_point<T>::value>::type>
{
    static constexpr T min()
    {
        return std::numeric_limits<T>::min();
    }

    static constexpr T max()
    {
        return std::numeric_limits<T>::max();
    }

    static constexpr T null()
    {
        return std::numeric_limits<T>::quiet_NaN();
    }
};

template<typename BuiltIn, typename Underlying>
struct type_pair
{
    using built_in_t = BuiltIn;
    using underlying_t = Underlying;
};

template<typename TypePair>
struct type_storage : public ::testing::Test
{
    using type_pair = TypePair;
};

using RequiredBuiltInTypes = ::testing::Types<
    type_pair<sbepp::char_t, char>,
    type_pair<sbepp::int8_t, std::int8_t>,
    type_pair<sbepp::int16_t, std::int16_t>,
    type_pair<sbepp::int32_t, std::int32_t>,
    type_pair<sbepp::int64_t, std::int64_t>,
    type_pair<sbepp::float_t, float>,
    type_pair<sbepp::double_t, double>>;

using OptionalBuiltInTypes = ::testing::Types<
    type_pair<sbepp::char_opt_t, char>,
    type_pair<sbepp::int8_opt_t, std::int8_t>,
    type_pair<sbepp::int16_opt_t, std::int16_t>,
    type_pair<sbepp::int32_opt_t, std::int32_t>,
    type_pair<sbepp::int64_opt_t, std::int64_t>>;

using FloatingPointOptionalBuiltInTypes = ::testing::Types<
    type_pair<sbepp::float_opt_t, float>,
    type_pair<sbepp::double_opt_t, double>>;

template<typename T>
using RequiredBuiltInTypesTest = type_storage<T>;

template<typename T>
using OptionalBuiltInTypesTest = type_storage<T>;

template<typename T>
using FloatingPointOptionalBuiltInTypesTest = type_storage<T>;

TYPED_TEST_SUITE(RequiredBuiltInTypesTest, RequiredBuiltInTypes);
TYPED_TEST_SUITE(OptionalBuiltInTypesTest, OptionalBuiltInTypes);
TYPED_TEST_SUITE(
    FloatingPointOptionalBuiltInTypesTest, FloatingPointOptionalBuiltInTypes);

TYPED_TEST(RequiredBuiltInTypesTest, ProvidesCorrectMinMaxValues)
{
    using type_pair_t = typename TestFixture::type_pair;
    using built_in_t = typename type_pair_t::built_in_t;
    using underlying_t = typename type_pair_t::underlying_t;

    ASSERT_EQ(built_in_t::min_value(), built_in_limits<underlying_t>::min());
    ASSERT_EQ(built_in_t::max_value(), built_in_limits<underlying_t>::max());
    ASSERT_EQ(
        sbepp::type_traits<built_in_t>::min_value(),
        built_in_limits<underlying_t>::min());
    ASSERT_EQ(
        sbepp::type_traits<built_in_t>::max_value(),
        built_in_limits<underlying_t>::max());
}

TYPED_TEST(OptionalBuiltInTypesTest, ProvidesCorrectMinMaxNullValues)
{
    using type_pair_t = typename TestFixture::type_pair;
    using built_in_t = typename type_pair_t::built_in_t;
    using underlying_t = typename type_pair_t::underlying_t;

    ASSERT_EQ(built_in_t::min_value(), built_in_limits<underlying_t>::min());
    ASSERT_EQ(built_in_t::max_value(), built_in_limits<underlying_t>::max());
    ASSERT_EQ(built_in_t::null_value(), built_in_limits<underlying_t>::null());
    ASSERT_EQ(
        sbepp::type_traits<built_in_t>::min_value(),
        built_in_limits<underlying_t>::min());
    ASSERT_EQ(
        sbepp::type_traits<built_in_t>::max_value(),
        built_in_limits<underlying_t>::max());
    ASSERT_EQ(
        sbepp::type_traits<built_in_t>::null_value(),
        built_in_limits<underlying_t>::null());
}

// optional floating-point types require separate test because they need to use
// `std::isnan` to check their null values
TYPED_TEST(
    FloatingPointOptionalBuiltInTypesTest, ProvidesCorrectMinMaxNullValues)
{
    using type_pair_t = typename TestFixture::type_pair;
    using built_in_t = typename type_pair_t::built_in_t;
    using underlying_t = typename type_pair_t::underlying_t;

    ASSERT_EQ(built_in_t::min_value(), built_in_limits<underlying_t>::min());
    ASSERT_EQ(built_in_t::max_value(), built_in_limits<underlying_t>::max());
    ASSERT_TRUE(std::isnan(built_in_t::null_value()));
    ASSERT_EQ(
        sbepp::type_traits<built_in_t>::min_value(),
        built_in_limits<underlying_t>::min());
    ASSERT_EQ(
        sbepp::type_traits<built_in_t>::max_value(),
        built_in_limits<underlying_t>::max());
    ASSERT_TRUE(std::isnan(sbepp::type_traits<built_in_t>::null_value()));
}
} // namespace