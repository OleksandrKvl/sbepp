// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/sbe_error.hpp>

#include <fmt/format.h>

namespace sbepp::sbeppc
{
template<typename F, typename... Args>
[[noreturn]] void throw_error(const F& format, Args&&... args)
{
    const auto msg = fmt::format(format, std::forward<Args>(args)...);
    throw sbe_error{msg};
}
} // namespace sbepp::sbeppc
