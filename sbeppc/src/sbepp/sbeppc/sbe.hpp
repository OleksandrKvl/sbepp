// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/source_location.hpp>

#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

namespace sbepp::sbeppc::sbe
{
enum class byte_order_kind
{
    little_endian,
    big_endian
};

struct type;
struct composite;
struct ref;
struct enumeration;
struct set;

using encoding = std::variant<type, composite, enumeration, set>;
using composite_element = std::variant<type, composite, ref, enumeration, set>;

struct type
{
    std::string name;
    std::string description;
    field_presence presence;
    std::optional<std::string> null_value;
    std::optional<std::string> min_value;
    std::optional<std::string> max_value;
    length_t length;
    std::optional<offset_t> offset;
    std::string primitive_type;
    std::string semantic_type;
    version_t added_since;
    std::optional<version_t> deprecated_since;
    std::optional<std::string> character_encoding;
    std::optional<std::string> constant_value;
    std::optional<std::string> value_ref;
    source_location location;
};

struct composite
{
    std::string name;
    std::optional<offset_t> offset;
    std::string description;
    std::string semantic_type;
    version_t added_since;
    std::optional<version_t> deprecated_since;
    std::vector<composite_element> elements;
    source_location location;
};

struct ref
{
    std::string name;
    std::string type;
    std::optional<offset_t> offset;
    version_t added_since;
    std::optional<version_t> deprecated_since;
    source_location location;
};

struct enum_valid_value
{
    std::string name;
    std::string description;
    version_t added_since;
    std::optional<version_t> deprecated_since;
    std::string value;
    source_location location;
};

struct enumeration
{
    std::string name;
    std::string description;
    std::string type;
    version_t added_since;
    std::optional<version_t> deprecated_since;
    std::optional<offset_t> offset;
    std::vector<enum_valid_value> valid_values;
    source_location location;
};

struct set_choice
{
    std::string name;
    std::string description;
    version_t added_since;
    std::optional<version_t> deprecated_since;
    choice_index_t value;
    source_location location;
};

struct set
{
    std::string name;
    std::string description;
    std::string type;
    version_t added_since;
    std::optional<version_t> deprecated_since;
    std::optional<offset_t> offset;
    std::vector<set_choice> choices;
    source_location location;
};

struct field;
struct group;
struct data;

using message_member = std::variant<field, group, data>;

struct level_members
{
    std::vector<field> fields;
    std::vector<group> groups;
    std::vector<sbe::data> data;
};

struct message
{
    std::string name;
    message_id_t id;
    std::string description;
    std::optional<block_length_t> block_length;
    std::string semantic_type;
    version_t added_since;
    std::optional<version_t> deprecated_since;
    level_members members;
    source_location location;
};

struct field
{
    std::string name;
    member_id_t id;
    std::string description;
    std::string type;
    std::optional<offset_t> offset;
    field_presence presence;
    std::optional<std::string> value_ref;
    version_t added_since;
    std::optional<version_t> deprecated_since;
    source_location location;
};

struct group
{
    std::string name;
    member_id_t id;
    std::string description;
    std::string semantic_type;
    std::string dimension_type;
    level_members members;
    std::optional<block_length_t> block_length;
    version_t added_since;
    std::optional<version_t> deprecated_since;
    source_location location;
};

struct data
{
    std::string name;
    member_id_t id;
    std::string description;
    std::string type;
    version_t added_since;
    std::optional<version_t> deprecated_since;
    source_location location;
};

struct message_schema
{
    std::string package;
    schema_id_t id;
    version_t version;
    std::string semantic_version;
    byte_order_kind byte_order;
    std::string description;
    std::string header_type;
    source_location location;

    std::unordered_map<std::string, encoding> types;
    std::vector<message> messages;
};
} // namespace sbepp::sbeppc::sbe
