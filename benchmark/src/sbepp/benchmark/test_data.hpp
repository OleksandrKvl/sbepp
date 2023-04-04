// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <cstdint>
#include <array>

namespace sbepp
{
namespace benchmark
{
using byte_type = std::uint8_t;
using buffer_type = std::array<byte_type, 0x4000>;

struct test_data
{
    buffer_type buffer;
    std::uint64_t top_level_checksum;
    std::uint64_t flat_group_checksum;
    std::uint64_t nested_group_checksum;
    std::uint64_t nested_group2_checksum;
    std::uint64_t data_checksum;
};
} // namespace benchmark
} // namespace sbepp