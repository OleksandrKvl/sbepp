// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#ifdef USE_TOP_FILE
#    include <test_schema/test_schema.hpp>
#else
#    include <test_schema/types/numbers_enum.hpp>
#endif

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace
{
// code from https://stackoverflow.com/a/10724828
template<typename T, bool B = std::is_enum<T>::value>
struct is_scoped_enum : std::false_type
{
};

template<typename T>
struct is_scoped_enum<T, true>
    : std::integral_constant<
          bool,
          !std::is_convertible<T, typename std::underlying_type<T>::type>::
              value>
{
};

using enum_t = test_schema::types::numbers_enum;
// SBE enums are represented using C++11 scoped enumerations
STATIC_ASSERT_V(is_scoped_enum<enum_t>);

STATIC_ASSERT(sbepp::is_enum<enum_t>::value);

#if SBEPP_HAS_INLINE_VARS
STATIC_ASSERT(sbepp::is_enum_v<enum_t>);
#endif

#if SBEPP_HAS_CONCEPTS
STATIC_ASSERT(sbepp::enumeration<enum_t>);
#endif

TEST(EnumTest, EnumsHaveTheSameValuesAsInSchema)
{
    ASSERT_EQ(sbepp::to_underlying(enum_t::One), 1);
    ASSERT_EQ(sbepp::to_underlying(enum_t::Two), 2);
}

TEST(EnumTest, EnumToStringReturnsEnumeratorName)
{
    ASSERT_STREQ(sbepp::enum_to_string(enum_t::One), "One");
    ASSERT_STREQ(sbepp::enum_to_string(enum_t::Two), "Two");
}

TEST(EnumTest, EnumToStringReturnsNullptrIfValueIsUnknown)
{
    const auto e = static_cast<enum_t>(4);

    ASSERT_EQ(sbepp::enum_to_string(e), nullptr);
}

template<enum_t Enumerator, typename ValidTag>
class enum_test_visitor
{
public:
    void on_enum_value(enum_t value, ValidTag)
    {
        valid = (value == Enumerator);
    }

    template<typename WrongTag>
    void on_enum_value(enum_t /*value*/, WrongTag)
    {
        valid = false;
    }

    bool is_valid() const
    {
        return valid;
    }

private:
    bool valid{};
};

TEST(EnumTest, EnumVisitorIsCalledWithCorrectValueAndTag)
{
    const auto visitor = sbepp::visit<enum_test_visitor<
        enum_t::One,
        test_schema::schema::types::numbers_enum::One>>(enum_t::One);

    ASSERT_TRUE(visitor.is_valid());
}

TEST(EnumTest, EnumVisitorIsCalledWithUnknownTagForUnknownValue)
{
    constexpr auto unknown_value = static_cast<enum_t>(4);
    const auto visitor = sbepp::visit<
        enum_test_visitor<unknown_value, sbepp::unknown_enum_value_tag>>(
        unknown_value);

    ASSERT_TRUE(visitor.is_valid());
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto underlying = sbepp::to_underlying(enum_t::One);
#endif
} // namespace
