// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/source_location.hpp>

#include <algorithm>
#include <cassert>
#include <string_view>
#include <string>
#include <vector>

namespace sbepp::sbeppc
{
class location_manager
{
public:
    location_manager() = default;

    location_manager(
        const std::string_view path, const std::string_view content)
        : path{path}
    {
        std::size_t line_begin{};
        for(std::size_t i = 0; i != content.size(); i++)
        {
            if(content[i] == '\n')
            {
                ranges.push_back({line_begin, i});
                line_begin = i + 1;
            }
        }

        ranges.push_back({line_begin, content.size()});
    }

    source_location find(const std::size_t offset) const
    {
        const auto search = std::lower_bound(
            std::begin(ranges),
            std::end(ranges),
            offset,
            [](const auto& lhs, const auto rhs)
            {
                return lhs.end < rhs;
            });

        assert(search != std::end(ranges) && "Offset is out of range");

        const auto line =
            static_cast<std::size_t>(search - std::begin(ranges) + 1);
        const auto column = offset - search->begin + 1;
        return {path, line, column};
    }

private:
    std::string path;

    struct line_range
    {
        std::size_t begin;
        std::size_t end;
    };
    std::vector<line_range> ranges;
};
} // namespace sbepp::sbeppc
