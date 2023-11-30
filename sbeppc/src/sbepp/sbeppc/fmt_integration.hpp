// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval
// Contributed by Nicolai Grodzitski.

#pragma once

#include <fmt/format.h>

namespace sbepp::sbeppc
{

#if FMT_VERSION < 90000
    using fmt_compile_string = ::fmt::compile_string;
#else
    using fmt_compile_string = ::fmt::detail::compile_string;
#endif

template <typename Fmt_String>
inline constexpr bool is_fmt_compile_string =
    std::is_base_of_v<fmt_compile_string, Fmt_String>;

} // namespace sbepp::sbeppc
