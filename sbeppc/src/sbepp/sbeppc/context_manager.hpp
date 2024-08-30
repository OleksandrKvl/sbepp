// SPDX-License-Identifier: MIT
// Copyright (c) 2024, Oleksandr Koval
#pragma once

#include "sbepp/sbeppc/throw_error.hpp"
#include <sbepp/sbeppc/sbe.hpp>

#include <unordered_map>
#include <string>
#include <cstddef>
#include <optional>

namespace sbepp::sbeppc
{
struct type_context
{
    std::string tag;
    std::string impl_name;
    std::string impl_type;
    std::string underlying_type;
    std::size_t size;
    std::string public_type;
    bool is_template;
    // set for anonymous types only
    std::optional<offset_t> actual_offset;
};

struct composite_context
{
    std::string tag;
    std::string impl_name;
    std::string impl_type;
    std::size_t size;
    std::string public_type;
    // set for anonymous types only
    std::optional<offset_t> actual_offset;
};

struct ref_context
{
    std::string tag;
    offset_t actual_offset;
};

struct enum_valid_value_context
{
    std::string tag;
};

struct enumeration_context
{
    std::string tag;
    std::string impl_name;
    std::string impl_type;
    std::string underlying_type;
    std::size_t size;
    std::string public_type;
    // set for anonymous types only
    std::optional<offset_t> actual_offset;
};

struct set_choice_context
{
    std::string tag;
};

struct set_context
{
    // TODO: seems this context is similar to enum's one
    std::string tag;
    std::string impl_name;
    std::string impl_type;
    std::string underlying_type;
    std::size_t size;
    std::string public_type;
    // set for anonymous types only
    std::optional<offset_t> actual_offset;
};

struct message_context
{
    std::string tag;
    std::string impl_name;
    std::string impl_type;
    std::string public_type;
    block_length_t actual_block_length;
};

struct field_context
{
    std::string tag;
    // does NOT include message/group header size
    offset_t actual_offset;
    std::size_t size;
    field_presence actual_presence;
    std::string value_type;
    std::string value_type_tag;
    bool is_template;
};

struct group_context
{
    std::string tag;
    std::string entry_impl_type;
    std::string impl_name;
    std::string impl_type;
    block_length_t actual_block_length;
};

struct data_context
{
    std::string tag;
    const sbe::type* length_type;
    std::string impl_type;
};

struct message_schema_context
{
    std::string name;
    std::string tag;
};

class context_manager
{
public:
    type_context& create(const sbe::type& t)
    {
        auto [it, inserted] = type_contexts.emplace(&t, type_context{});
        if(!inserted)
        {
            throw_error(
                "{}: context for a type `{}` already exists",
                t.location,
                t.name);
        }

        return it->second;
    }

    type_context& get(const sbe::type& t)
    {
        return type_contexts.at(&t);
    }

    const type_context& get(const sbe::type& t) const
    {
        return type_contexts.at(&t);
    }

private:
    std::unordered_map<const sbe::type*, type_context> type_contexts;
};
} // namespace sbepp::sbeppc