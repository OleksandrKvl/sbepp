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
#include <iterator>
#include <vector>

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
        return make_level_traits(m.members) + make_message_root_traits(m);
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
    struct group_size_bytes_info
    {
        std::vector<std::string> param_names;
        std::vector<std::string> param_types;
        bool has_data_members;
    };

    struct level_size_bytes_info
    {
        std::vector<std::string> param_names;
        std::vector<std::string> param_types;
        // does not include trailing `total_data_size` term
        std::vector<std::string> sum_terms;
        bool has_data_members;
    };

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

    std::string get_num_in_group_underlying_type(const sbe::group& g) const
    {
        const auto& header = types->get_as_or_throw<sbe::composite>(
            g.dimension_type,
            "{}: encoding `{}` doesn't exist or it's not a composite");

        const auto t = std::get_if<sbe::type>(
            utils::find_composite_element(header, "numInGroup"));
        if(t)
        {
            return t->underlying_type;
        }

        throw_error(
            "{}: `numInGroup` is not found or it's not a type",
            header.location);
    }

    static bool
        vector_contains(const std::vector<std::string>& v, const std::string& s)
    {
        return (std::find(std::begin(v), std::end(v), s) != std::end(v));
    }

    static std::string make_unique_param_name(
        std::string desired_name,
        const std::vector<std::string>& existing_names1,
        const std::vector<std::string>& existing_names2,
        const std::size_t level_depth)
    {
        // names on the same level are always unique but it's not guaranteed for
        // names from different levels (which are concatenated to create group
        // size parameter name). By adding `_<depth>` suffix we ensure that
        // there will be no two identical parameter names because each level has
        // unique suffix which is used only in case of conflicts.
        if(vector_contains(existing_names1, desired_name)
           || vector_contains(existing_names2, desired_name))
        {
            return fmt::format("{}_{}", desired_name, level_depth);
        }

        return desired_name;
    }

    void get_group_size_bytes_info_impl(
        const sbe::group& g,
        const std::vector<std::string>& existing_names,
        std::vector<std::string>& path,
        group_size_bytes_info& info) const
    {
        path.push_back(g.name);

        info.param_names.push_back(make_unique_param_name(
            fmt::format("{}_num_in_group", fmt::join(path, "_")),
            existing_names,
            info.param_names,
            path.size() - 1));
        info.param_types.push_back(get_num_in_group_underlying_type(g));

        for(const auto& nested_group : g.members.groups)
        {
            get_group_size_bytes_info_impl(
                nested_group, existing_names, path, info);
        }

        info.has_data_members |= !g.members.data.empty();
        path.pop_back();
    }

    group_size_bytes_info get_group_size_bytes_info(
        const sbe::group& g,
        const std::vector<std::string>& existing_names,
        std::vector<std::string>& path) const
    {
        group_size_bytes_info info{};
        get_group_size_bytes_info_impl(g, existing_names, path, info);

        return info;
    }

    static std::string
        make_group_size_bytes_args(const group_size_bytes_info& info)
    {
        auto args = fmt::format("{}", fmt::join(info.param_names, ", "));
        if(info.has_data_members)
        {
            if(args.empty())
            {
                args = "0";
            }
            else
            {
                args += ", 0";
            }
        }

        return args;
    }

    static void append_vector(
        std::vector<std::string>& to, std::vector<std::string> from)
    {
        to.insert(
            std::end(to),
            std::make_move_iterator(std::begin(from)),
            std::make_move_iterator(std::end(from)));
    }

    level_size_bytes_info
        get_level_size_bytes_info(const sbe::level_members& members) const
    {
        level_size_bytes_info info{};
        std::vector<std::string> path;

        for(const auto& g : members.groups)
        {
            auto group_info =
                get_group_size_bytes_info(g, info.param_names, path);

            info.has_data_members |= group_info.has_data_members;

            info.sum_terms.push_back(fmt::format(
                "::sbepp::group_traits<{tag}>::size_bytes({args})",
                fmt::arg("tag", g.tag),
                fmt::arg("args", make_group_size_bytes_args(group_info))));
            append_vector(info.param_names, std::move(group_info.param_names));
            append_vector(info.param_types, std::move(group_info.param_types));
        }

        for(const auto& d : members.data)
        {
            info.has_data_members = true;
            info.sum_terms.push_back(
                fmt::format("::sbepp::data_traits<{}>::size_bytes(0)", d.tag));
        }

        return info;
    }

    static std::string make_size_bytes_params(const level_size_bytes_info& info)
    {
        std::string params;
        bool is_first_param{true};

        for(std::size_t i = 0; i != info.param_types.size(); i++)
        {
            if(is_first_param)
            {
                is_first_param = false;
            }
            else
            {
                params += ", ";
            }

            params += fmt::format(
                "const {} {}", info.param_types[i], info.param_names[i]);
        }

        if(info.has_data_members)
        {
            if(!is_first_param)
            {
                params += ", ";
            }
            params += "const ::std::size_t total_data_size";
        }

        return params;
    }

    std::string make_group_size_bytes_params(
        const sbe::group& g, const level_size_bytes_info& info) const
    {
        auto params = fmt::format(
            "const {} num_in_group", get_num_in_group_underlying_type(g));
        const auto optional_params = make_size_bytes_params(info);

        if(!optional_params.empty())
        {
            params += ", ";
            params += optional_params;
        }

        return params;
    }

    std::string make_group_size_bytes(const sbe::group& g) const
    {
        auto info = get_level_size_bytes_info(g.members);

        if(!info.sum_terms.empty())
        {
            // adding empty string just to have `+` prefix when it's joined
            info.sum_terms.emplace(std::begin(info.sum_terms), "");
        }

        auto body_size = fmt::format(
            "num_in_group * (block_length() {})",
            fmt::join(info.sum_terms, "\n+ "));

        if(info.has_data_members)
        {
            body_size += "\n+ total_data_size";
        }

        return fmt::format(
            // clang-format off
R"(static constexpr ::std::size_t size_bytes({params}) noexcept
    {{
        return ::sbepp::composite_traits<dimension_type_tag>::size_bytes()
            + {body_size};
    }}
)",
            // clang-format on
            fmt::arg("params", make_group_size_bytes_params(g, info)),
            fmt::arg("body_size", body_size));
    }

    std::string make_message_size_bytes(const sbe::message& m) const
    {
        auto info = get_level_size_bytes_info(m.members);
        if(info.has_data_members)
        {
            info.sum_terms.emplace_back("total_data_size");
        }

        if(!info.sum_terms.empty())
        {
            // adding empty string just to have `+` prefix when it's joined
            info.sum_terms.emplace(std::begin(info.sum_terms), "");
        }

        return fmt::format(
            // clang-format off
R"(static constexpr ::std::size_t size_bytes({params}) noexcept
    {{
        return ::sbepp::composite_traits<
            ::sbepp::schema_traits<schema_tag>::header_type_tag>::size_bytes()
        + block_length() {sum_terms};
    }}
)",
            // clang-format on
            fmt::arg("params", make_size_bytes_params(info)),
            fmt::arg("sum_terms", fmt::join(info.sum_terms, "\n+ ")));
    }

    std::string make_message_root_traits(const sbe::message& m) const
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
    {schema_tag}
    {size_bytes_impl}
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
            fmt::arg("deprecated_impl", make_deprecated(m.deprecated_since)),
            fmt::arg("size_bytes_impl", make_message_size_bytes(m)),
            fmt::arg(
                "schema_tag",
                utils::make_type_alias("schema_tag", schema->tag)));
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
{level_traits}

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
    {size_bytes_impl}
}};
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
            fmt::arg("deprecated_impl", make_deprecated(g.deprecated_since)),
            fmt::arg("size_bytes_impl", make_group_size_bytes(g)));
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

    static constexpr ::std::size_t size_bytes(
        const length_type::value_type size) noexcept
    {{
        return sizeof(size) + size;
    }}
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
