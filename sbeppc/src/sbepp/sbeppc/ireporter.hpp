// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <string_view>

#include <sbepp/sbeppc/fmt_integration.hpp>

namespace sbepp::sbeppc
{
class ireporter
{
public:
    virtual ~ireporter() = default;

    template<typename F,
             typename... Args,
             typename = std::enable_if_t<is_fmt_compile_string<F>>>
    void error(F format, Args&&... args)
    {
        const auto msg = fmt::format(format, std::forward<Args>(args)...);
        report_error(msg);
    }

    template<typename F,
             typename... Args,
             typename = std::enable_if_t<!is_fmt_compile_string<F>>>
    void error(const F& format, Args&&... args)
    {
        const auto msg = fmt::format(::fmt::runtime(format), std::forward<Args>(args)...);
        report_error(msg);
    }

    template<typename F,
             typename... Args,
             typename = std::enable_if_t<is_fmt_compile_string<F>>>
    void warning(F format, Args&&... args)
    {
        const auto msg = fmt::format(format, std::forward<Args>(args)...);
        report_warning(msg);
    }

    template<typename F,
             typename... Args,
             typename = std::enable_if_t<!is_fmt_compile_string<F>>>
    void warning(const F& format, Args&&... args)
    {
        const auto msg = fmt::format(::fmt::runtime(format), std::forward<Args>(args)...);
        report_warning(msg);
    }

private:
    virtual void report_error(std::string_view msg) = 0;
    virtual void report_warning(std::string_view msg) = 0;
};
} // namespace sbepp::sbeppc
