// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/unique_set.hpp>

#include <vector>

namespace sbepp::sbeppc
{
class message_manager
{
public:
    void add_unique(sbe::message message)
    {
        unique_names.add_or_throw(
            message.name,
            "{}: message with name `{}` already exists",
            message.location,
            message.name);
        unique_ids.add_or_throw(
            message.id,
            "{}: message with id `{}` already exists",
            message.location,
            message.id);

        messages.push_back(std::move(message));
    }

    void merge(const message_manager& other)
    {
        for(const auto& message : other.messages)
        {
            add_unique(message);
        }
    }

    template<typename F>
    void for_each(F cb)
    {
        for(auto& message : messages)
        {
            cb(message);
        }
    }

    template<typename F>
    void for_each(F cb) const
    {
        for(const auto& message : messages)
        {
            cb(message);
        }
    }

    bool contains(const std::string& name) const
    {
        return unique_names.contains(name);
    }

private:
    std::vector<sbe::message> messages;
    unique_set<std::string> unique_names;
    unique_set<message_id_t> unique_ids;
};
} // namespace sbepp::sbeppc
