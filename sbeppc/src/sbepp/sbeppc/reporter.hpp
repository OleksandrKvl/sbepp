// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/ireporter.hpp>

#include <fmt/core.h>
#include <fmt/color.h>

#include <cstdio>

namespace sbepp::sbeppc
{
class reporter : public ireporter
{
private:
    void report_error(std::string_view msg) override
    {
        fmt::print("{}: {}\n", fmt::styled("Error", fmt::emphasis::bold), msg);
        std::fflush({});
    }

    void report_warning(std::string_view msg) override
    {
        fmt::print(
            "{}: {}\n", fmt::styled("Warning", fmt::emphasis::bold), msg);
    }
};
} // namespace sbepp::sbeppc
