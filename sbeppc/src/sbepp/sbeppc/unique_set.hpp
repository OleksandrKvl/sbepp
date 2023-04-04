// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/throw_error.hpp>

#include <unordered_set>
#include <string>

namespace sbepp::sbeppc
{
template<typename T>
class unique_set
{
public:
    template<typename S, typename Format, typename... Args>
    void add_or_throw(S&& value, const Format& format, Args&&... args)
    {
        const auto [it, inserted] = items.emplace(std::forward<S>(value));
        if(!inserted)
        {
            throw_error(format, std::forward<Args>(args)...);
        }
    }

    template<typename T2>
    bool contains(T2&& value) const
    {
        const auto search = items.find(std::forward<T2>(value));
        return (search != std::end(items));
    }

private:
    std::unordered_set<T> items;
};
} // namespace sbepp::sbeppc
