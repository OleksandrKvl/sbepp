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
#include <fmt/ranges.h>

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

{traits_tag}
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
            fmt::arg("deprecated_impl", make_deprecated(t.deprecated_since)),
            fmt::arg("traits_tag", make_traits_tag(t)));
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

{traits_tag}
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
            fmt::arg("deprecated_impl", make_deprecated(e.deprecated_since)),
            fmt::arg("traits_tag", make_traits_tag(e.public_type, e.tag)));
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

{traits_tag}
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
            fmt::arg("deprecated_impl", make_deprecated(s.deprecated_since)),
            fmt::arg("traits_tag", make_traits_tag(s.public_type, s.tag)));
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

{traits_tag}
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
            fmt::arg("deprecated_impl", make_deprecated(c.deprecated_since)),
            fmt::arg(
                "traits_tag", make_templated_traits_tag(c.public_type, c.tag)));
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

    static std::string make_unique_param_name(
        std::string desired_name,
        const std::vector<std::string>& existing_names,
        const std::size_t level_depth)
    {
        // names on the same level are always unique but it's not guaranteed for
        // names from different levels (which are concatenated to create group
        // size parameter name). By adding `_<depth>` suffix we ensure that
        // there will be no two identical parameter names because each level has
        // unique suffix which is used only in case of conflicts.
        if(std::find(
               std::begin(existing_names),
               std::end(existing_names),
               desired_name)
           != std::end(existing_names))
        {
            return fmt::format("{}_{}", desired_name, level_depth);
        }

        return desired_name;
    }

    static std::string get_group_payload_size(
        const sbe::group& g, std::vector<std::string>& param_names)
    {
        std::vector<std::string> headers_sum;
        headers_sum.reserve(g.members.groups.size() + g.members.data.size());

        for(const auto& nested_group : g.members.groups)
        {
            headers_sum.push_back(fmt::format(
                "::sbepp::composite_traits<::sbepp::group_traits<{}"
                ">::dimension_type_tag>::size_bytes()",
                nested_group.tag));
        }

        for(const auto& d : g.members.data)
        {
            headers_sum.push_back(
                fmt::format("::sbepp::data_traits<{}>::size_bytes(0)", d.tag));
        }

        if(!headers_sum.empty())
        {
            headers_sum.emplace(std::begin(headers_sum), "");
        }

        return fmt::format(
            "{num_in_group_param} * "
            "(::sbepp::group_traits<{tag}>::block_length() {headers_sum})",
            fmt::arg("num_in_group_param", param_names.back()),
            fmt::arg("tag", g.tag),
            fmt::arg("headers_sum", fmt::join(headers_sum, "\n+ ")));
    }

    void make_group_size_bytes_impl(
        const sbe::group& g,
        std::vector<std::string>& path,
        std::vector<std::string>& param_names,
        std::vector<std::string>& param_types,
        std::vector<std::string>& sum_terms,
        bool& has_data_members) const
    {
        if(path.empty())
        {
            // it's a top-level group
            param_names.emplace_back("num_in_group");
        }
        else
        {
            param_names.push_back(make_unique_param_name(
                fmt::format("{}_num_in_group", fmt::join(path, "_")),
                param_names,
                path.size()));
        }

        param_types.push_back(get_num_in_group_underlying_type(g));
        has_data_members |= !g.members.data.empty();
        sum_terms.push_back(get_group_payload_size(g, param_names));

        for(const auto& nested_group : g.members.groups)
        {
            path.push_back(nested_group.name);
            make_group_size_bytes_impl(
                nested_group,
                path,
                param_names,
                param_types,
                sum_terms,
                has_data_members);
            path.pop_back();
        }
    }

    static std::string make_size_bytes_params(
        const std::vector<std::string>& param_names,
        const std::vector<std::string>& param_types)
    {
        assert(param_names.size() == param_types.size());

        std::string params;
        bool is_first{true};
        for(std::size_t i = 0; i != param_names.size(); i++)
        {
            if(is_first)
            {
                is_first = false;
            }
            else
            {
                params += ", ";
            }
            params += fmt::format(
                "\n        const {} {}", param_types[i], param_names[i]);
        }

        return params;
    }

    std::string make_group_size_bytes(const sbe::group& g) const
    {
        bool has_data_members{};
        std::vector<std::string> path;
        std::vector<std::string> param_names;
        std::vector<std::string> param_types;
        std::vector<std::string> sum_terms;

        make_group_size_bytes_impl(
            g, path, param_names, param_types, sum_terms, has_data_members);

        if(has_data_members)
        {
            param_names.emplace_back("total_data_size");
            param_types.emplace_back("::std::size_t");
            sum_terms.emplace_back("total_data_size");
        }

        return fmt::format(
            // clang-format off
R"(static constexpr ::std::size_t size_bytes({params}) noexcept
    {{
        return ::sbepp::composite_traits<dimension_type_tag>::size_bytes()
            + {sum_terms};
    }}
)",
            // clang-format on
            fmt::arg(
                "params", make_size_bytes_params(param_names, param_types)),
            fmt::arg("sum_terms", fmt::join(sum_terms, "\n+ ")));
    }

    void get_group_size_bytes_params(
        const sbe::group& g,
        std::vector<std::string>& path,
        std::vector<std::string>& param_names,
        std::vector<std::string>& param_types,
        bool& has_data_members) const
    {
        path.push_back(g.name);

        param_names.push_back(make_unique_param_name(
            fmt::format("{}_num_in_group", fmt::join(path, "_")),
            param_names,
            path.size() - 1));
        param_types.push_back(get_num_in_group_underlying_type(g));
        has_data_members |= !g.members.data.empty();

        for(const auto& nested_group : g.members.groups)
        {
            get_group_size_bytes_params(
                nested_group, path, param_names, param_types, has_data_members);
        }

        path.pop_back();
    }

    static std::string make_group_size_bytes_args(
        const std::vector<std::string>& param_names,
        const std::size_t params_to_use,
        const bool has_data_members)
    {
        auto params = fmt::format(
            "{}",
            fmt::join(
                // NOLINTNEXTLINE: this is guaranteed to be safe
                std::end(param_names) - params_to_use,
                std::end(param_names),
                ", "));
        if(has_data_members)
        {
            if(params.empty())
            {
                return "0";
            }
            else
            {
                params += ", 0";
            }
        }

        return params;
    }

    void make_message_size_bytes_impl(
        const sbe::level_members& members,
        std::vector<std::string>& path,
        std::vector<std::string>& param_names,
        std::vector<std::string>& param_types,
        std::vector<std::string>& sum_terms,
        bool& has_data_members) const
    {
        for(const auto& g : members.groups)
        {
            const auto prev_size = param_names.size();
            bool has_nested_data_members{};
            get_group_size_bytes_params(
                g, path, param_names, param_types, has_nested_data_members);
            const auto params_added = param_names.size() - prev_size;

            sum_terms.push_back(fmt::format(
                "::sbepp::group_traits<{tag}>::size_bytes({params})",
                fmt::arg("tag", g.tag),
                fmt::arg(
                    "params",
                    make_group_size_bytes_args(
                        param_names, params_added, has_nested_data_members))));
            has_data_members |= has_nested_data_members;
        }

        has_data_members |= !members.data.empty();
        for(const auto& d : members.data)
        {
            sum_terms.push_back(
                fmt::format("::sbepp::data_traits<{}>::size_bytes(0)", d.tag));
        }
    }

    std::string make_message_size_bytes(const sbe::message& m) const
    {
        std::vector<std::string> sum_terms;
        std::vector<std::string> param_names;
        std::vector<std::string> param_types;
        std::vector<std::string> path;
        bool has_data_members{};

        sum_terms.emplace_back("block_length()");

        make_message_size_bytes_impl(
            m.members,
            path,
            param_names,
            param_types,
            sum_terms,
            has_data_members);

        if(has_data_members)
        {
            param_names.emplace_back("total_data_size");
            param_types.emplace_back("::std::size_t");
            sum_terms.emplace_back("total_data_size");
        }

        return fmt::format(
            // clang-format off
R"(static constexpr ::std::size_t size_bytes({params}) noexcept
    {{
        return ::sbepp::composite_traits<
            ::sbepp::schema_traits<schema_tag>::header_type_tag>::size_bytes()
            + {sum_terms};
    }}
)",
            // clang-format on
            fmt::arg(
                "params", make_size_bytes_params(param_names, param_types)),
            fmt::arg("sum_terms", fmt::join(sum_terms, "\n+ ")));
    }

    static std::string make_traits_tag(const sbe::type& t)
    {
        // `traits_tag` specialization for array types exists in sbepp.hpp. For
        // numeric constants we don't generate this mapping because they are
        // represented using raw types
        if(!t.is_template && (t.presence != field_presence::constant))
        {
            return make_traits_tag(t.public_type, t.tag);
        }

        return "";
    }

    static std::string make_templated_traits_tag(
        const std::string_view type, const std::string_view tag)
    {
        return fmt::format(
            // clang-format off
R"(template<typename Byte>
struct traits_tag<{type}<Byte>>
{{
    using type = {tag};
}};
)",
            // clang-format on
            fmt::arg("type", type),
            fmt::arg("tag", tag));
    }

    static std::string
        make_traits_tag(const std::string_view type, const std::string_view tag)
    {
        return fmt::format(
            // clang-format off
R"(template<>
struct traits_tag<{type}>
{{
    using type = {tag};
}};
)",
            // clang-format on
            fmt::arg("type", type),
            fmt::arg("tag", tag));
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

{traits_tag}
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
                utils::make_type_alias("schema_tag", schema->tag)),
            fmt::arg(
                "traits_tag", make_templated_traits_tag(m.public_type, m.tag)));
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

{traits_tag}
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
            fmt::arg("size_bytes_impl", make_group_size_bytes(g)),
            fmt::arg(
                "traits_tag", make_templated_traits_tag(g.impl_type, g.tag)));
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
