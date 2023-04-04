// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <fmt/core.h>

#include <string_view>

namespace sbepp::sbeppc
{
class ireporter
{
public:
    virtual ~ireporter() = default;

    template<typename F, typename... Args>
    void error(const F& format, Args&&... args)
    {
        const auto msg = fmt::format(format, std::forward<Args>(args)...);
        report_error(msg);
    }

    template<typename F, typename... Args>
    void warning(const F& format, Args&&... args)
    {
        const auto msg = fmt::format(format, std::forward<Args>(args)...);
        report_warning(msg);
    }

private:
    virtual void report_error(std::string_view msg) = 0;
    virtual void report_warning(std::string_view msg) = 0;
};
} // namespace sbepp::sbeppc
