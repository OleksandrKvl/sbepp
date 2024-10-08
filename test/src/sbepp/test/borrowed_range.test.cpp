// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <test_schema/messages/msg2.hpp>

#if SBEPP_HAS_RANGES && SBEPP_HAS_CONCEPTS
#    include <ranges>

namespace unused
{
void borrowed_range_test(char* ptr, std::size_t size)
{
    test_schema::messages::msg2<char> m{ptr, size};

    // static_array_ref
    (void)*std::ranges::max_element(m.array());
    // dynamic_array_ref
    (void)*std::ranges::max_element(m.data());

    // nested_group_base
    (void)*std::ranges::max_element(
        m.group(),
        [](auto, auto)
        {
            return true;
        });

    // flat_group_base
    (void)*std::ranges::max_element(
        m.group().front().group(),
        [](auto, auto)
        {
            return true;
        });
}
} // namespace unused
#endif
