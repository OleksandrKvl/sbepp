// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <string_view>

namespace sbepp::sbeppc
{
struct build_info
{
    static std::string_view get_version();
};
} // namespace sbepp::sbeppc
