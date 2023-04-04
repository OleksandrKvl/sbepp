// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/throw_error.hpp>

#include <string>
#include <unordered_map>

namespace sbepp::sbeppc
{
class type_manager
{
public:
    void add_unique(sbe::encoding e)
    {
        // SBE requires type lookup to be case insensitive
        auto name = utils::to_lower(utils::get_encoding_name(e));
        const auto [it, inserted] =
            types.try_emplace(std::move(name), std::move(e));

        if(!inserted)
        {
            throw_error(
                "{}: encoding `{}` already exists at {}",
                utils::get_location(e),
                utils::get_encoding_name(e),
                utils::get_location(it->second));
        }
    }

    void merge(const type_manager& other)
    {
        for(const auto& [name, e] : other.types)
        {
            add_unique(e);
        }
    }

    template<typename Format, typename... Args>
    sbe::encoding& get_or_throw(
        const std::string& name, const Format& format, Args&&... args)
    {
        const auto lower_name = utils::to_lower(name);
        auto search = types.find(lower_name);
        if(search != std::end(types))
        {
            return search->second;
        }
        throw_error(format, std::forward<Args>(args)...);
    }

    template<typename Format, typename... Args>
    const sbe::encoding& get_or_throw(
        const std::string& name, const Format& format, Args&&... args) const
    {
        const auto lower_name = utils::to_lower(name);
        auto search = types.find(lower_name);
        if(search != std::end(types))
        {
            return search->second;
        }
        throw_error(format, std::forward<Args>(args)...);
    }

    template<typename T, typename Format, typename... Args>
    const T& get_as_or_throw(
        const std::string& name, const Format& format, Args&&... args) const
    {
        const auto& enc =
            get_or_throw(name, format, std::forward<Args>(args)...);
        auto desired_enc = std::get_if<T>(&enc);
        if(!desired_enc)
        {
            throw_error(format, std::forward<Args>(args)...);
        }

        return *desired_enc;
    }

    template<typename F>
    void for_each(F cb)
    {
        for(auto& [name, type] : types)
        {
            cb(type);
        }
    }

    template<typename F>
    void for_each(F cb) const
    {
        for(const auto& [name, type] : types)
        {
            cb(type);
        }
    }

    bool contains(const std::string& type) const
    {
        const auto search = types.find(type);
        return (search != std::end(types));
    }

private:
    std::unordered_map<std::string, sbe::encoding> types;
};
} // namespace sbepp::sbeppc
