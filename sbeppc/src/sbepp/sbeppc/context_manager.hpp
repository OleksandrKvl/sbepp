// SPDX-License-Identifier: MIT
// Copyright (c) 2024, Oleksandr Koval
#pragma once

#include <sbepp/sbeppc/sbe.hpp>

#include <unordered_map>
#include <string>
#include <cstddef>
#include <optional>
#include <tuple>

namespace sbepp::sbeppc
{
// TODO: extract context types into separate headers?
struct type_context
{
    // std::string tag;
    // std::string impl_name;
    // std::string impl_type;
    // std::string underlying_type;
    std::size_t size;
    // std::string public_type;
    // bool is_template;
    // here and below, set only if encoding is located inside a composite
    std::optional<offset_t> offset_in_composite;
};

struct composite_context
{
    // std::string tag;
    // std::string impl_name;
    // std::string impl_type;
    std::size_t size;
    // std::string public_type;
    std::optional<offset_t> offset_in_composite;
};

struct ref_context
{
    std::size_t size;
    // std::string tag;
    // unlike other encodings, ref is always within a composite so the member
    // is not optional
    offset_t offset_in_composite;
};

struct enum_valid_value_context
{
    std::string tag;
};

struct enumeration_context
{
    // std::string tag;
    // std::string impl_name;
    // std::string impl_type;
    // std::string underlying_type;
    std::size_t size;
    // std::string public_type;
    std::optional<offset_t> offset_in_composite;
};

struct set_choice_context
{
    std::string tag;
};

struct set_context
{
    // TODO: seems this context is similar to enum's one
    // std::string tag;
    // std::string impl_name;
    // std::string impl_type;
    // std::string underlying_type;
    std::size_t size;
    // std::string public_type;
    std::optional<offset_t> offset_in_composite;
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

template<typename T>
struct context_type;

template<typename T>
using context_type_t = typename context_type<T>::type;

template<>
struct context_type<sbe::type>
{
    using type = type_context;
};

template<>
struct context_type<sbe::enumeration>
{
    using type = enumeration_context;
};

template<>
struct context_type<sbe::set>
{
    using type = set_context;
};

template<>
struct context_type<sbe::composite>
{
    using type = composite_context;
};

template<>
struct context_type<sbe::ref>
{
    using type = ref_context;
};

class context_manager
{
public:
    template<typename T>
    context_type_t<T>& get(const T& t)
    {
        return std::get<map_type<T>>(contexts)[&t];
    }

private:
    template<typename T>
    using map_type = std::unordered_map<const T*, context_type_t<T>>;

    std::tuple<
        map_type<sbe::type>,
        map_type<sbe::enumeration>,
        map_type<sbe::set>,
        map_type<sbe::composite>,
        map_type<sbe::ref>>
        contexts;
};
} // namespace sbepp::sbeppc