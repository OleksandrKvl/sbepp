// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <fmt/format.h>

#include <cstddef>
#include <string>

namespace sbepp::sbeppc
{
struct source_location
{
    std::string file;
    std::size_t line;
    std::size_t column;
};
} // namespace sbepp::sbeppc

template<>
class fmt::formatter<sbepp::sbeppc::source_location>
{
public:
    static constexpr auto parse(format_parse_context& ctx)
    {
        // support only `{}`
        if(ctx.begin() != ctx.end())
        {
            throw format_error{"invalid format"};
        }
        return ctx.begin();
    }

    template<typename Context>
    constexpr auto
        format(const sbepp::sbeppc::source_location& l, Context& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}:{}:{}", l.file, l.line, l.column);
    }
};
