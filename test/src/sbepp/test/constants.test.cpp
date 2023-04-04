// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#ifdef USE_TOP_FILE
#    include <test_schema/test_schema.hpp>
#else
#    include <test_schema/types/constant_refs.hpp>
#    include <test_schema/types/constants.hpp>
#    include <test_schema/types/numbers_enum.hpp>
#    include <test_schema/messages/Msg1.hpp>
#endif

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace
{
// standalone constants are not stored anywhere and can be accessed only when
// they are defined or referenced inside composites or messages

// these values are copy-pasted from schema XML
const std::uint32_t g_num_const{123};
const char g_char_const{'X'};
const std::string g_str_const1{"hello world"};
// without explicit length `\0` padding is not included into a string
const std::string g_str_const2{"hi\0\0", sizeof("hi\0\0") - 1};

template<typename T>
class ConstantsTest : public ::testing::Test
{
public:
    using constant_container_t = T;
};

// constants have the same interface for all possible ways of using them:
// via reference/inline definition in composite or via message field
using ConstantContainers = ::testing::Types<
    test_schema::types::constant_refs<char>,
    test_schema::types::constants<char>,
    test_schema::messages::Msg1<char>>;

TYPED_TEST_SUITE(ConstantsTest, ConstantContainers);

TYPED_TEST(
    ConstantsTest,
    ArithmeticConstantsRepresentedUsingBuiltinTypesThroughStaticFunctions)
{
    using container_t = typename TestFixture::constant_container_t;

    STATIC_ASSERT_V(std::is_same<
                    decltype(container_t::num_const_value()),
                    typename std::remove_const<decltype(g_num_const)>::type>);
    STATIC_ASSERT_V(std::is_same<
                    decltype(container_t::char_const_value()),
                    typename std::remove_const<decltype(g_char_const)>::type>);

    ASSERT_EQ(container_t::num_const_value(), g_num_const);
    ASSERT_EQ(container_t::char_const_value(), g_char_const);
}

TYPED_TEST(
    ConstantsTest,
    NumericEnumeratorConstantRepresentedUsingBuiltinTypesThroughStaticFunction)
{
    using container_t = typename TestFixture::constant_container_t;

    STATIC_ASSERT_V(std::is_same<
                    decltype(container_t::num_const_from_enum_value()),
                    std::uint32_t>);

    ASSERT_EQ(
        container_t::num_const_from_enum_value(),
        sbepp::to_underlying(test_schema::types::numbers_enum::Two));
}

TYPED_TEST(
    ConstantsTest,
    StringConstantsRepresentedUsingSpanLikeTypeThroughStaticFunction)
{
    using container_t = typename TestFixture::constant_container_t;

    const auto str_const1 = container_t::str_const1_value();
    const auto str_const2 = container_t::str_const2_value();

    ASSERT_EQ(
        std::distance(std::begin(str_const1), std::end(str_const1)),
        std::distance(std::begin(g_str_const1), std::end(g_str_const1)));
    ASSERT_TRUE(std::equal(
        std::begin(str_const1),
        std::end(str_const1),
        std::begin(g_str_const1)));

    ASSERT_EQ(
        std::distance(std::begin(str_const2), std::end(str_const2)),
        std::distance(std::begin(g_str_const2), std::end(g_str_const2)));
    ASSERT_TRUE(std::equal(
        std::begin(str_const2),
        std::end(str_const2),
        std::begin(g_str_const2)));
}

TEST(EnumConstant, EnumConstantRepresentedUsingEnumeratorThroughStaticFunction)
{
    using message_t = test_schema::messages::Msg1<char>;

    STATIC_ASSERT_V(std::is_same<
                    decltype(message_t::enum_const_value()),
                    test_schema::types::numbers_enum>);

    ASSERT_EQ(
        message_t::enum_const_value(), test_schema::types::numbers_enum::Two);
}

namespace constexpr_tests
{
using composite_t = test_schema::types::constants<char>;
constexpr auto c1 = composite_t::num_const_value();
constexpr auto c2 = composite_t::char_const_value();
constexpr auto c3 = composite_t::num_const_from_enum_value();

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr void constexpr_test()
{
    constexpr auto c4 = composite_t::str_const1_value();
    (void)c4;
    constexpr auto c5 = composite_t::str_const2_value();
    (void)c5;
}
#endif
} // namespace constexpr_tests
} // namespace
