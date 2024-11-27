// SPDX-License-Identifier: MIT
// Copyright (c) 2024, Oleksandr Koval

#include <test_schema/test_schema.hpp>

#include <gtest/gtest.h>

namespace
{
template<typename T>
void unused(T&&)
{
}

TEST(TopHeaderIncludesTest, AllPublicSchemaTypesAreAvailable)
{
    // schema
    unused(test_schema::schema{});

    // types
    unused(test_schema::types::numbers_enum{});
    unused(test_schema::types::composite_14<char>{});
    unused(test_schema::types::options_set{});
    unused(test_schema::types::composite_13<char>{});
    unused(test_schema::types::composite_11<char>{});
    unused(test_schema::types::composite_9<char>{});
    unused(test_schema::types::composite_8<char>{});
    unused(test_schema::types::composite_19<char>{});
    unused(test_schema::types::composite_a<char>{});
    unused(test_schema::types::composite_17<char>{});
    unused(test_schema::types::composite_16<char>{});
    unused(test_schema::types::composite_15<char>{});
    unused(test_schema::types::composite_6<char>{});
    unused(test_schema::types::composite_5<char>{});
    unused(test_schema::types::composite_4<char>{});
    unused(test_schema::types::composite_3<char>{});
    unused(test_schema::types::composite_7<char>{});
    unused(test_schema::types::uint32_req{});
    unused(test_schema::types::str128<char>{});
    unused(test_schema::types::refs_composite<char>{});
    unused(test_schema::types::char_const{});
    unused(test_schema::types::composite_b<char>{});
    unused(test_schema::types::uint32_opt{});
    unused(test_schema::types::composite_1<char>{});
    unused(test_schema::types::varDataEncoding<char>{});
    unused(test_schema::types::constants<char>{});
    unused(test_schema::types::composite_2<char>{});
    unused(test_schema::types::num_const{});
    unused(test_schema::types::num_const_from_enum{});
    unused(test_schema::types::str_const1{});
    unused(test_schema::types::str_const2{});
    unused(test_schema::types::constant_refs<char>{});
    unused(test_schema::types::arr8<char>{});
    unused(test_schema::types::varStrEncoding<char>{});
    unused(test_schema::types::composite_12<char>{});
    unused(test_schema::types::composite_18<char>{});
    unused(test_schema::types::groupSizeEncoding<char>{});
    unused(test_schema::types::messageHeader<char>{});
    unused(test_schema::types::composite_10<char>{});

    // messages
    unused(test_schema::messages::Msg1<char>{});
    unused(test_schema::messages::msg2<char>{});
    unused(test_schema::messages::msg3<char>{});
    unused(test_schema::messages::msg4<char>{});
    unused(test_schema::messages::msg5<char>{});
    unused(test_schema::messages::msg6<char>{});
    unused(test_schema::messages::msg7<char>{});
    unused(test_schema::messages::msg8<char>{});
    unused(test_schema::messages::msg9<char>{});
    unused(test_schema::messages::msg10<char>{});
    unused(test_schema::messages::msg11<char>{});
    unused(test_schema::messages::msg12<char>{});
    unused(test_schema::messages::msg13<char>{});
    unused(test_schema::messages::msg14<char>{});
    unused(test_schema::messages::msg15<char>{});
    unused(test_schema::messages::msg16<char>{});
    unused(test_schema::messages::msg17<char>{});
    unused(test_schema::messages::msg24<char>{});
    unused(test_schema::messages::msg25<char>{});
    unused(test_schema::messages::msg18<char>{});
    unused(test_schema::messages::msg19<char>{});
    unused(test_schema::messages::msg20<char>{});
    unused(test_schema::messages::msg21<char>{});
    unused(test_schema::messages::msg22<char>{});
    unused(test_schema::messages::msg23<char>{});
    unused(test_schema::messages::msg26<char>{});
    unused(test_schema::messages::msg27<char>{});
    unused(test_schema::messages::msg28<char>{});
    unused(test_schema::messages::msg29<char>{});
}
} // namespace