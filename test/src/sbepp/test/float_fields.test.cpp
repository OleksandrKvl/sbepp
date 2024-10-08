// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <test_schema/types/composite_19.hpp>

#include <gtest/gtest.h>

#include <array>

namespace
{
using byte_type = std::uint8_t;

class FloatFieldsTest : public ::testing::Test
{
public:
    std::array<byte_type, 128> buf{};
    test_schema::types::composite_19<byte_type> c{buf.data(), buf.size()};
};

TEST_F(FloatFieldsTest, SetsFloatFieldCorrectly)
{
    static constexpr float valid_value = 1.2;

    c.float_field(valid_value);

    ASSERT_EQ(*c.float_field(), valid_value);
}

TEST_F(FloatFieldsTest, SetsDoubleFieldCorrectly)
{
    static constexpr double valid_value = 1.2;

    c.double_field(valid_value);

    ASSERT_EQ(*c.double_field(), valid_value);
}
} // namespace