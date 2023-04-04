// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/type_manager.hpp>
#include <sbepp/sbeppc/message_manager.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>

#include <fmt/core.h>

#include <string>

namespace sbepp::sbeppc
{
class traits_generator
{
public:
    traits_generator(
        const sbe::message_schema& schema, const type_manager& types)
        : schema{&schema}, types{&types}
    {
    }

    std::string make_schema_traits() const
    {
        const auto& header_type = types->get_as_or_throw<sbe::composite>(
            schema->header_type,
            "{}: type `{}` doesn't exist or it's not a composite",
            schema->location,
            schema->header_type);

        return fmt::format(
            // clang-format off
R"(
template<>
class schema_traits<{tag}>
{{
public:
    static constexpr const char* package() noexcept
    {{
        return "{package}";
    }}

    static constexpr schema_id_t id() noexcept
    {{
        return {id};
    }}

    static constexpr version_t version() noexcept
    {{
        return {version};
    }}

    static constexpr const char* semantic_version() noexcept
    {{
        return "{semantic_version}";
    }}

    static constexpr endian byte_order() noexcept
    {{
        return {endian};
    }}

    static constexpr const char* description() noexcept
    {{
        return "{description}";
    }}

    {header_type}
    {header_type_tag}
}};
)",
            // clang-format on
            fmt::arg("tag", schema->tag),
            fmt::arg("package", schema->package),
            fmt::arg("id", schema->id),
            fmt::arg("version", schema->version),
            fmt::arg("semantic_version", schema->semantic_version),
            fmt::arg("endian", utils::byte_order_to_endian(schema->byte_order)),
            fmt::arg("description", schema->description),
            fmt::arg(
                "header_type",
                utils::make_alias_template(
                    "header_type", header_type.impl_type)),
            fmt::arg(
                "header_type_tag",
                utils::make_type_alias("header_type_tag", header_type.tag)));
    }

    std::string make_message_traits(const sbe::message& m) const
    {
        return make_message_root_traits(m) + make_level_traits(m.members);
    }

    std::string make_type_traits(const sbe::encoding& enc) const
    {
        return std::visit(
            [this](const auto& enc)
            {
                return this->make_traits(enc);
            },
            enc);
    }

private:
    const sbe::message_schema* schema;
    const type_manager* types;

    static std::string make_min_max_null_values(const sbe::type& t)
    {
        if((t.length == 1) && (t.presence != field_presence::constant))
        {
            std::string getters =
                // clang-format off
R"(
    static constexpr primitive_type min_value() noexcept
    {
        return value_type::min_value();
    }
    
    static constexpr primitive_type max_value() noexcept
    {
        return value_type::max_value();
    }
)";
            // clang-format on

            if(t.presence == field_presence::optional)
            {
                getters +=
                    // clang-format off
R"(
    static constexpr primitive_type null_value() noexcept
    {
        return value_type::null_value();
    }
)";
                // clang-format on
            }

            return getters;
        }

        return {};
    }

    static std::string make_value_type(const sbe::type& t)
    {
        if(t.is_template)
        {
            return utils::make_alias_template("value_type", t.public_type);
        }

        return utils::make_type_alias("value_type", t.public_type);
    }

    static std::string
        make_deprecated(const std::optional<version_t>& deprecated_since)
    {
        if(deprecated_since)
        {
            return fmt::format(
                // clang-format off
R"(static constexpr version_t deprecated() noexcept
    {{
        return {{{deprecated_since}}};
    }}
)",
                // clang-format on
                fmt::arg("deprecated_since", *deprecated_since));
        }
        return {};
    }

    static std::string make_offset_impl(
        const std::optional<offset_t>& offset_a,
        const std::optional<offset_t>& offset_b = {})
    {
        const auto& offset = offset_a ? offset_a : offset_b;
        if(offset)
        {
            return fmt::format(
                // clang-format off
R"(static constexpr offset_t offset() noexcept
    {{
        return {{{offset}}};
    }}
)",
                // clang-format on
                fmt::arg("offset", *offset));
        }
        return {};
    }

    static std::string make_traits(const sbe::type& t)
    {
        return fmt::format(
            // clang-format off
R"(
template<>
class type_traits<{tag}>
{{
public:
    static constexpr const char* name() noexcept
    {{
        return "{name}";
    }}

    static constexpr const char* description() noexcept
    {{
        return "{description}";
    }}

    static constexpr field_presence presence() noexcept
    {{
        return {presence};
    }}

    {primitive_type}
    {min_max_null_values}
    
    static constexpr length_t length() noexcept
    {{
        return {length};
    }}
    
    {offset_impl}
    
    static constexpr const char* semantic_type() noexcept
    {{
        return "{semantic_type}";
    }}
    
    static constexpr version_t since_version() noexcept
    {{
        return {since_version};
    }}

    static constexpr const char* character_encoding() noexcept
    {{
        return "{character_encoding}";
    }}

    {deprecated_impl}
    {value_type}
}};
)",
            // clang-format on
            fmt::arg("tag", t.tag),
            fmt::arg("name", t.name),
            fmt::arg("description", t.description),
            fmt::arg("presence", utils::presence_to_string(t.presence)),
            fmt::arg("length", t.length),
            fmt::arg(
                "offset_impl", make_offset_impl(t.offset, t.actual_offset)),
            fmt::arg(
                "primitive_type",
                utils::make_type_alias("primitive_type", t.underlying_type)),
            fmt::arg("semantic_type", t.semantic_type),
            fmt::arg("since_version", t.added_since),
            fmt::arg("value_type", make_value_type(t)),
            fmt::arg("min_max_null_values", make_min_max_null_values(t)),
            fmt::arg("character_encoding", t.character_encoding.value_or("")),
            fmt::arg("deprecated_impl", make_deprecated(t.deprecated_since)));
    }

    static std::string make_enum_traits(const sbe::enumeration& e)
    {
        return fmt::format(
            // clang-format off
R"(
template<>
class enum_traits<{tag}>
{{
public:
    static constexpr const char* name() noexcept
    {{
        return "{name}";
    }}

    static constexpr const char* description() noexcept
    {{
        return "{description}";
    }}

    {encoding_type}
    {offset_impl}
    
    static constexpr version_t since_version() noexcept
    {{
        return {since_version};
    }}

    {deprecated_impl}
    {value_type}
}};
)",
            // clang-format on
            fmt::arg("tag", e.tag),
            fmt::arg("name", e.name),
            fmt::arg("description", e.description),
            fmt::arg(
                "offset_impl", make_offset_impl(e.offset, e.actual_offset)),
            fmt::arg(
                "encoding_type",
                utils::make_type_alias("encoding_type", e.underlying_type)),
            fmt::arg("since_version", e.added_since),
            fmt::arg(
                "value_type",
                utils::make_type_alias("value_type", e.public_type)),
            fmt::arg("deprecated_impl", make_deprecated(e.deprecated_since)));
    }

    static std::string make_enum_value_traits(const sbe::enumeration& e)
    {
        std::string res;
        for(const auto& value : e.valid_values)
        {
            res += fmt::format(
                // clang-format off
R"(
template<>
class enum_value_traits<{tag}>
{{
public:
    static constexpr const char* name() noexcept
    {{
        return "{name}";
    }}

    static constexpr const char* description() noexcept
    {{
        return "{description}";
    }}
    
    static constexpr version_t since_version() noexcept
    {{
        return {since_version};
    }}

    {deprecated_impl}

    static constexpr {enum_type} value() noexcept
    {{
        return {enum_type}::{name};
    }}
}};
)",
                // clang-format on
                fmt::arg("tag", value.tag),
                fmt::arg("name", value.name),
                fmt::arg("description", value.description),
                fmt::arg("since_version", value.added_since),
                fmt::arg("enum_type", e.public_type),
                fmt::arg(
                    "deprecated_impl",
                    make_deprecated(value.deprecated_since)));
        }

        return res;
    }

    static std::string make_traits(const sbe::enumeration& e)
    {
        return make_enum_traits(e) + make_enum_value_traits(e);
    }

    static std::string make_set_traits(const sbe::set& s)
    {
        return fmt::format(
            // clang-format off
R"(
template<>
class set_traits<{tag}>
{{
public:
    static constexpr const char* name() noexcept
    {{
        return "{name}";
    }}

    static constexpr const char* description() noexcept
    {{
        return "{description}";
    }}

    {encoding_type}
    {offset_impl}
    
    static constexpr version_t since_version() noexcept
    {{
        return {since_version};
    }}

    {deprecated_impl}
    {value_type}
}};
)",
            // clang-format on
            fmt::arg("tag", s.tag),
            fmt::arg("name", s.name),
            fmt::arg("description", s.description),
            fmt::arg(
                "offset_impl", make_offset_impl(s.offset, s.actual_offset)),
            fmt::arg(
                "encoding_type",
                utils::make_type_alias("encoding_type", s.underlying_type)),
            fmt::arg("since_version", s.added_since),
            fmt::arg(
                "value_type",
                utils::make_type_alias("value_type", s.public_type)),
            fmt::arg("deprecated_impl", make_deprecated(s.deprecated_since)));
    }

    static std::string make_set_choice_traits(const sbe::set& s)
    {
        std::string res;
        for(const auto& choice : s.choices)
        {
            res += fmt::format(
                // clang-format off
R"(
template<>
class set_choice_traits<{tag}>
{{
public:
    static constexpr const char* name() noexcept
    {{
        return "{name}";
    }}

    static constexpr const char* description() noexcept
    {{
        return "{description}";
    }}
    
    static constexpr version_t since_version() noexcept
    {{
        return {since_version};
    }}

    {deprecated_impl}

    static constexpr choice_index_t index() noexcept
    {{
        return {index};
    }}
}};
)",
                // clang-format on
                fmt::arg("tag", choice.tag),
                fmt::arg("name", choice.name),
                fmt::arg("description", choice.description),
                fmt::arg("since_version", choice.added_since),
                fmt::arg("index", choice.value),
                fmt::arg(
                    "deprecated_impl",
                    make_deprecated(choice.deprecated_since)));
        }

        return res;
    }

    static std::string make_traits(const sbe::set& s)
    {
        return make_set_traits(s) + make_set_choice_traits(s);
    }

    static std::string make_composite_traits(const sbe::composite& c)
    {
        return fmt::format(
            // clang-format off
R"(
template<>
class composite_traits<{tag}>
{{
public:
    static constexpr const char* name() noexcept
    {{
        return "{name}";
    }}

    static constexpr const char* description() noexcept
    {{
        return "{description}";
    }}

    {offset_impl}

    static constexpr const char* semantic_type() noexcept
    {{
        return "{semantic_type}";
    }}

    static constexpr version_t since_version() noexcept
    {{
        return {since_version};
    }}

    {deprecated_impl}
    {value_type}

    static constexpr std::size_t size_bytes() noexcept
    {{
        return {size};
    }}
}};
)",
            // clang-format on
            fmt::arg("tag", c.tag),
            fmt::arg("name", c.name),
            fmt::arg("description", c.description),
            fmt::arg("semantic_type", c.semantic_type),
            fmt::arg(
                "offset_impl", make_offset_impl(c.offset, c.actual_offset)),
            fmt::arg("since_version", c.added_since),
            fmt::arg(
                "value_type",
                utils::make_alias_template("value_type", c.public_type)),
            fmt::arg("size", c.size),
            fmt::arg("deprecated_impl", make_deprecated(c.deprecated_since)));
    }

    std::string make_traits(const sbe::composite& c) const
    {
        std::string res;

        for(const auto& element : c.elements)
        {
            res += std::visit(
                [this](const auto& encoding)
                {
                    return this->make_traits(encoding);
                },
                element);
        }

        res += make_composite_traits(c);

        return res;
    }

    static std::string_view get_traits_name(const sbe::encoding& encoding)
    {
        return std::visit(
            utils::overloaded{
                [](const sbe::type&)
                {
                    return "type_traits";
                },
                [](const sbe::enumeration&)
                {
                    return "enum_traits";
                },
                [](const sbe::set&)
                {
                    return "set_traits";
                },
                [](const sbe::composite&)
                {
                    return "composite_traits";
                }},
            encoding);
    }

    static std::string_view get_tag(const sbe::encoding& encoding)
    {
        return std::visit(
            [](const auto& enc) -> std::string_view
            {
                return enc.tag;
            },
            encoding);
    }

    std::string make_traits(const sbe::ref& r) const
    {
        const auto& target_type = types->get_or_throw(
            r.type, "{}: encoding `{}` doesn't exist", r.location);

        const auto& traits_name = get_traits_name(target_type);
        const auto& target_tag = get_tag(target_type);

        return fmt::format(
            // clang-format off
R"(
template<>
class {traits_name}<{tag}> : public {traits_name}<{target_tag}>
{{
public:
    static constexpr const char* name() noexcept
    {{
        return "{name}";
    }}

    {offset_impl}

    static constexpr version_t since_version() noexcept
    {{
        return {since_version};
    }}

    {deprecated_impl}
}};
)",
            // clang-format on
            fmt::arg("traits_name", traits_name),
            fmt::arg("tag", r.tag),
            fmt::arg("target_tag", target_tag),
            fmt::arg("name", r.name),
            fmt::arg("offset_impl", make_offset_impl(r.actual_offset)),
            fmt::arg("since_version", r.added_since),
            fmt::arg("deprecated_impl", make_deprecated(r.deprecated_since)));
    }

    static std::string make_message_root_traits(const sbe::message& m)
    {
        return fmt::format(
            // clang-format off
R"(
template<>
class message_traits<{tag}>
{{
public:
    static constexpr const char* name() noexcept
    {{
        return "{name}";
    }}

    static constexpr const char* description() noexcept
    {{
        return "{description}";
    }}

    static constexpr message_id_t id() noexcept
    {{
        return {id};
    }}

    static constexpr block_length_t block_length() noexcept
    {{
        return {block_length};
    }}

    static constexpr const char* semantic_type() noexcept
    {{
        return "{semantic_type}";
    }}

    static constexpr version_t since_version() noexcept
    {{
        return {since_version};
    }}

    {deprecated_impl}
    {value_type}
}};
)",
            // clang-format on
            fmt::arg("tag", m.tag),
            fmt::arg("name", m.name),
            fmt::arg("description", m.description),
            fmt::arg("id", m.id),
            fmt::arg("block_length", m.actual_block_length),
            fmt::arg("semantic_type", m.semantic_type),
            fmt::arg("since_version", m.added_since),
            fmt::arg(
                "value_type",
                utils::make_alias_template("value_type", m.public_type)),
            fmt::arg("deprecated_impl", make_deprecated(m.deprecated_since)));
    }

    static std::string make_field_value_type(const sbe::field& f)
    {
        if((f.actual_presence == field_presence::constant) || (!f.is_template))
        {
            return utils::make_type_alias("value_type", f.value_type);
        }
        else
        {
            return utils::make_alias_template("value_type", f.value_type);
        }
    }

    static std::string make_field_value_type_tag(const sbe::field& f)
    {
        if(f.actual_presence != field_presence::constant)
        {
            return utils::make_type_alias("value_type_tag", f.value_type_tag);
        }

        return {};
    }

    static std::string make_traits(const sbe::field& f)
    {
        return fmt::format(
            // clang-format off
R"(
template<>
class field_traits<{tag}>
{{
public:
    static constexpr const char* name() noexcept
    {{
        return "{name}";
    }}

    static constexpr member_id_t id() noexcept
    {{
        return {id};
    }}

    static constexpr const char* description() noexcept
    {{
        return "{description}";
    }}

    static constexpr field_presence presence() noexcept
    {{
        return {presence};
    }}
    
    {offset_impl}

    static constexpr version_t since_version() noexcept
    {{
        return {since_version};
    }}

    {deprecated_impl}
    {value_type}
    {value_type_tag}
}};
)",
            // clang-format on
            fmt::arg("tag", f.tag),
            fmt::arg("name", f.name),
            fmt::arg("id", f.id),
            fmt::arg("description", f.description),
            fmt::arg("presence", utils::presence_to_string(f.actual_presence)),
            fmt::arg("offset_impl", make_offset_impl(f.actual_offset)),
            fmt::arg("since_version", f.added_since),
            fmt::arg("value_type", make_field_value_type(f)),
            fmt::arg("value_type_tag", make_field_value_type_tag(f)),
            fmt::arg("deprecated_impl", make_deprecated(f.deprecated_since)));
    }

    template<typename T>
    std::string make_member_traits(const std::vector<T>& members) const
    {
        std::string res;
        for(const auto& m : members)
        {
            res += make_traits(m);
        }
        return res;
    }

    std::string make_traits(const sbe::group& g) const
    {
        const auto& dimension_type = types->get_as_or_throw<sbe::composite>(
            g.dimension_type,
            "{}: encoding `{}` doesn't exist or it's not a composite",
            g.location,
            g.dimension_type);
        return fmt::format(
            // clang-format off
R"(
template<>
class group_traits<{tag}>
{{
public:
    static constexpr const char* name() noexcept
    {{
        return "{name}";
    }}

    static constexpr const char* description() noexcept
    {{
        return "{description}";
    }}

    static constexpr member_id_t id() noexcept
    {{
        return {id};
    }}

    static constexpr block_length_t block_length() noexcept
    {{
        return {block_length};
    }}

    static constexpr const char* semantic_type() noexcept
    {{
        return "{semantic_type}";
    }}

    static constexpr version_t since_version() noexcept
    {{
        return {since_version};
    }}

    {deprecated_impl}
    {value_type}
    {dimension_type}
    {dimension_type_tag}
    {entry_type}
}};

{level_traits}
)",
            // clang-format on
            fmt::arg("tag", g.tag),
            fmt::arg("name", g.name),
            fmt::arg("description", g.description),
            fmt::arg("id", g.id),
            fmt::arg("block_length", g.actual_block_length),
            fmt::arg("semantic_type", g.semantic_type),
            fmt::arg("since_version", g.added_since),
            fmt::arg(
                "value_type",
                utils::make_alias_template("value_type", g.impl_type)),
            fmt::arg(
                "dimension_type",
                utils::make_alias_template(
                    "dimension_type", dimension_type.public_type)),
            fmt::arg(
                "dimension_type_tag",
                utils::make_type_alias(
                    "dimension_type_tag", dimension_type.tag)),
            fmt::arg(
                "entry_type",
                utils::make_alias_template("entry_type", g.entry_impl_type)),
            fmt::arg("level_traits", make_level_traits(g.members)),
            fmt::arg("deprecated_impl", make_deprecated(g.deprecated_since)));
    }

    static std::string make_traits(const sbe::data& d)
    {
        return fmt::format(
            // clang-format off
R"(
template<>
class data_traits<{tag}>
{{
public:
    static constexpr const char* name() noexcept
    {{
        return "{name}";
    }}

    static constexpr member_id_t id() noexcept
    {{
        return {id};
    }}

    static constexpr const char* description() noexcept
    {{
        return "{description}";
    }}

    static constexpr version_t since_version() noexcept
    {{
        return {since_version};
    }}

    {deprecated_impl}
    template<typename Byte>
    using value_type = {value_type};

    {length_type}
    {length_type_tag}
}};
)",
            // clang-format on
            fmt::arg("tag", d.tag),
            fmt::arg("name", d.name),
            fmt::arg("description", d.description),
            fmt::arg("id", d.id),
            fmt::arg("since_version", d.added_since),
            fmt::arg("value_type", d.impl_type),
            fmt::arg(
                "length_type",
                utils::make_type_alias(
                    "length_type", d.length_type->public_type)),
            fmt::arg(
                "length_type_tag",
                utils::make_type_alias("length_type_tag", d.length_type->tag)),
            fmt::arg("deprecated_impl", make_deprecated(d.deprecated_since)));
    }

    std::string make_level_traits(const sbe::level_members& members) const
    {
        std::string res;
        res += make_member_traits(members.fields);
        res += make_member_traits(members.groups);
        res += make_member_traits(members.data);
        return res;
    }
};
} // namespace sbepp::sbeppc
