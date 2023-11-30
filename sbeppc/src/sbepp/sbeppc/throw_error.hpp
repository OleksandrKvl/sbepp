// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/sbe_error.hpp>
#include <sbepp/sbeppc/fmt_integration.hpp>

namespace sbepp::sbeppc
{

template<typename F,
         typename... Args,
         typename = std::enable_if_t<is_fmt_compile_string<F>>>
[[noreturn]] void throw_error(F format, Args&&... args)
{
    const auto msg = fmt::format(format, std::forward<Args>(args)...);
    throw sbe_error{msg};
}

template<typename F,
         typename... Args,
         typename = std::enable_if_t<!is_fmt_compile_string<F>>>
[[noreturn]] void throw_error(const F& format, Args&&... args)
{
    const auto msg = fmt::format(::fmt::runtime(format), std::forward<Args>(args)...);
    throw sbe_error{msg};
}

} // namespace sbepp::sbeppc
