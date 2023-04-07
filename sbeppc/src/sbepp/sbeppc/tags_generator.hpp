// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/source_location.hpp>
#include <sbepp/sbeppc/type_manager.hpp>
#include <sbepp/sbeppc/message_manager.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <vector>
#include <string>
#include <unordered_map>

namespace sbepp::sbeppc
{
class tags_generator
{
public:
    tags_generator(
        sbe::message_schema& schema,
        type_manager& types,
        message_manager& messages)
        : schema{&schema}, types{&types}, messages{&messages}
    {
        generated = generate();
    }

    const std::string& get() const
    {
        return generated;
    }

private:
    sbe::message_schema* schema;
    type_manager* types;
    message_manager* messages;
    std::string generated;
    // keeps processing state of a type, `false` means "in-progress", `true`
    // means "processed"
    std::unordered_map<std::string, bool> processed_types;
    std::string detail;
    std::size_t type_index{};
    std::size_t enum_index{};
    std::size_t set_index{};
    std::size_t composite_index{};
    std::size_t message_index{};
    std::size_t group_index{};

    std::string make_next_type_name()
    {
        type_index++;
        return fmt::format("type_{}", type_index);
    }

    std::string make_next_enum_name(const sbe::enumeration& e)
    {
        enum_index++;
        auto name = fmt::format("enum_{}", enum_index);

        // in C++ `enum class` can contain enumerator with the same name as enum
        // itself but `struct` cannot, e.g. `struct S{ struct S{}; };` is not
        // allowed. `struct`s are used for tags generation so enumerator names
        // should be taken into account
        std::unordered_set<std::string> names;
        for(const auto& value : e.valid_values)
        {
            names.emplace(value.name);
        }

        std::size_t minor_index{};
        while(names.count(name))
        {
            minor_index++;
            name = fmt::format("enum_{}_{}", enum_index, minor_index);
        }
        return name;
    }

    std::string make_next_set_name(const sbe::set& s)
    {
        set_index++;
        auto name = fmt::format("set_{}", set_index);

        std::unordered_set<std::string> names;
        for(const auto& choice : s.choices)
        {
            names.emplace(choice.name);
        }

        std::size_t minor_index{};
        while(names.count(name))
        {
            minor_index++;
            name = fmt::format("set_{}_{}", set_index, minor_index);
        }
        return name;
    }

    std::string make_next_composite_name(const sbe::composite& c)
    {
        composite_index++;
        auto name = fmt::format("composite_{}", composite_index);

        std::unordered_set<std::string> names;
        for(const auto& e : c.elements)
        {
            names.emplace(utils::get_encoding_name(e));
        }

        std::size_t minor_index{};
        while(names.count(name))
        {
            minor_index++;
            name = fmt::format("composite_{}_{}", composite_index, minor_index);
        }
        return name;
    }

    static std::unordered_set<std::string>
        get_member_names(const sbe::level_members& members)
    {
        std::unordered_set<std::string> res;

        for(const auto& f : members.fields)
        {
            res.insert(f.name);
        }

        for(const auto& g : members.groups)
        {
            res.insert(g.name);
        }

        for(const auto& d : members.data)
        {
            res.insert(d.name);
        }

        return res;
    }

    std::string make_next_group_name(const sbe::level_members& members)
    {
        group_index++;
        auto name = fmt::format("group_{}", group_index);

        const auto member_names = get_member_names(members);
        std::size_t minor_index{};
        while(member_names.count(name))
        {
            minor_index++;
            name = fmt::format("group_{}_{}", group_index, minor_index);
        }

        return name;
    }

    std::string make_next_message_name(const sbe::level_members& members)
    {
        message_index++;
        auto name = fmt::format("message_{}", message_index);

        const auto member_names = get_member_names(members);
        std::size_t minor_index{};
        while(member_names.count(name))
        {
            minor_index++;
            name = fmt::format("message_{}_{}", message_index, minor_index);
        }

        return name;
    }

    std::string make_public_tag(
        const std::vector<std::string>& path, const std::string_view last) const
    {
        return fmt::format(
            "::{}::schema::{}::{}", schema->name, fmt::join(path, "::"), last);
    }

    std::string make_impl_path(const std::string_view impl_name) const
    {
        // no need for `types/messages` here because in `detail` we have only
        // `impl_name`s which cannot conflict with each other
        return fmt::format("::{}::detail::schema::{}", schema->name, impl_name);
    }

    std::string make_tag(sbe::type& t, std::vector<std::string>& path)
    {
        t.impl_name = make_next_type_name();
        t.tag = make_public_tag(path, t.name);

        return fmt::format("struct {}{{}};\n", t.impl_name);
    }

    std::string make_enum_value_tags(
        sbe::enumeration& e, std::vector<std::string>& path)
    {
        std::string res;
        for(auto& value : e.valid_values)
        {
            res += fmt::format("    struct {}{{}};\n", value.name);
            value.tag = make_public_tag(path, value.name);
        }
        return res;
    }

    std::string make_tag(sbe::enumeration& e, std::vector<std::string>& path)
    {
        e.impl_name = make_next_enum_name(e);
        e.tag = make_public_tag(path, e.name);

        path.push_back(e.name);

        auto res = fmt::format(
            // clang-format off
R"(struct {name}
{{
{enum_value_tags}
}};
)",
            // clang-format on
            fmt::arg("name", e.impl_name),
            fmt::arg("enum_value_tags", make_enum_value_tags(e, path)));

        path.pop_back();

        return res;
    }

    std::string
        make_set_choice_tags(sbe::set& s, std::vector<std::string>& path)
    {
        std::string res;
        for(auto& choice : s.choices)
        {
            res += fmt::format("    struct {}{{}};\n", choice.name);
            choice.tag = make_public_tag(path, choice.name);
        }

        return res;
    }

    std::string make_tag(sbe::set& s, std::vector<std::string>& path)
    {
        s.impl_name = make_next_set_name(s);
        s.tag = make_public_tag(path, s.name);
        path.push_back(s.name);

        auto res = fmt::format(
            // clang-format off
R"(struct {name}
{{
{choice_tags}
}};
)",
            // clang-format on
            fmt::arg("name", s.impl_name),
            fmt::arg("choice_tags", make_set_choice_tags(s, path)));

        path.pop_back();

        return res;
    }

    std::string make_composite_element_tags(
        sbe::composite& c, std::vector<std::string>& path)
    {
        std::string res;

        for(auto& member : c.elements)
        {
            res += std::visit(
                utils::overloaded{
                    [&path, this](sbe::ref& r)
                    {
                        return make_tag(r, path);
                    },
                    [&path, this](auto& enc)
                    {
                        detail += make_tag(enc, path);
                        return fmt::format(
                            "using {} = {};\n",
                            enc.name,
                            make_impl_path(enc.impl_name));
                    }},
                member);
        }

        return res;
    }

    std::string make_tag(sbe::composite& c, std::vector<std::string>& path)
    {
        c.impl_name = make_next_composite_name(c);
        c.tag = make_public_tag(path, c.name);

        path.push_back(c.name);

        auto res = fmt::format(
            // clang-format off
R"(struct {name}
{{
{element_tags}
}};
)",
            // clang-format on
            fmt::arg("name", c.impl_name),
            fmt::arg("element_tags", make_composite_element_tags(c, path)));

        path.pop_back();

        return res;
    }

    static std::string_view get_impl_name(const sbe::encoding& enc)
    {
        return std::visit(
            [](const auto& e) -> std::string_view
            {
                return e.impl_name;
            },
            enc);
    }

    std::string make_tag(sbe::ref& r, std::vector<std::string>& path)
    {
        auto& type = types->get_or_throw(
            r.type, "{}: encoding `{}` doesn't exist", r.location, r.type);
        handle_public_encoding(type);

        r.tag = make_public_tag(path, r.name);

        return fmt::format(
            "struct {name} : {type}{{}};\n",
            fmt::arg("name", r.name),
            fmt::arg("type", get_impl_name(type)));
    }

    void handle_public_encoding(sbe::encoding& encoding)
    {
        std::vector<std::string> path;
        path.emplace_back("types");

        const std::string name{utils::get_encoding_name(encoding)};
        if(!processed_types.count(name))
        {
            processed_types[name] = false;

            detail += std::visit(
                [&path, this](auto& enc)
                {
                    return make_tag(enc, path);
                },
                encoding);

            processed_types[name] = true;
        }
        else if(!processed_types[name])
        {
            throw_error(
                "{}: cyclic reference detected while processing encoding `{}`",
                utils::get_location(encoding),
                utils::get_encoding_name(encoding));
        }
    }

    void make_type_tags_impl()
    {
        types->for_each(
            [this](auto& enc)
            {
                handle_public_encoding(enc);
            });
    }

    const sbe::composite* try_get_composite(
        const std::string& type_name, const source_location& location) const
    {
        if(!utils::is_primitive_type(type_name))
        {
            return std::get_if<sbe::composite>(&types->get_or_throw(
                type_name,
                "{}: encoding `{}` doesn't exist",
                location,
                type_name));
        }
        return {};
    }

    std::string make_field_tags(
        std::vector<sbe::field>& fields, std::vector<std::string>& path)
    {
        std::string res;

        for(auto& field : fields)
        {
            field.tag = make_public_tag(path, field.name);

            if(const auto c = try_get_composite(field.type, field.location))
            {
                res += fmt::format(
                    "    struct {name} : {impl}{{}};\n",
                    fmt::arg("name", field.name),
                    fmt::arg("impl", make_impl_path(c->impl_name)));
            }
            else
            {
                res += fmt::format("    struct {}{{}};\n", field.name);
            }
        }

        return res;
    }

    std::string make_group_tags(
        std::vector<sbe::group>& groups, std::vector<std::string>& path)
    {
        std::string res;

        for(auto& g : groups)
        {
            g.impl_name = make_next_group_name(g.members);
            g.tag = make_public_tag(path, g.name);
            path.push_back(g.name);

            detail += fmt::format(
                // clang-format off
R"(struct {group}
{{
{members}
}};
)",
                // clang-format on
                fmt::arg("group", g.impl_name),
                fmt::arg("members", make_member_tags(g.members, path)));

            res += fmt::format(
                "    using {} = {};\n", g.name, make_impl_path(g.impl_name));
            path.pop_back();
        }

        return res;
    }

    std::string make_data_tags(
        std::vector<sbe::data>& data_members,
        const std::vector<std::string>& path)
    {
        std::string res;

        for(auto& data : data_members)
        {
            data.tag = make_public_tag(path, data.name);
            res += fmt::format("    struct {}{{}};\n", data.name);
        }

        return res;
    }

    std::string make_member_tags(
        sbe::level_members& members, std::vector<std::string>& path)
    {
        std::string res;

        res += make_field_tags(members.fields, path);
        res += make_group_tags(members.groups, path);
        res += make_data_tags(members.data, path);

        return res;
    }

    std::string
        make_message_tag(sbe::message& m, std::vector<std::string>& path)
    {
        m.impl_name = make_next_message_name(m.members);
        m.tag = make_public_tag(path, m.name);
        path.push_back(m.name);

        auto res = fmt::format(
            // clang-format off
R"(struct {name}
{{
{members}
}};
)",
            // clang-format on
            fmt::arg("name", m.impl_name),
            fmt::arg("members", make_member_tags(m.members, path)));

        path.pop_back();

        return res;
    }

    void make_message_tags_impl()
    {
        std::vector<std::string> path;
        path.emplace_back("messages");

        messages->for_each(
            [this, &path](auto& m)
            {
                detail += make_message_tag(m, path);
            });
    }

    std::string make_type_aliases()
    {
        std::string res;
        types->for_each(
            [this, &res](const auto& enc)
            {
                res += std::visit(
                    [this](const auto& enc)
                    {
                        return fmt::format(
                            "    using {} = {};\n",
                            enc.name,
                            make_impl_path(enc.impl_name));
                    },
                    enc);
            });

        return res;
    }

    std::string make_message_aliases()
    {
        std::string res;
        messages->for_each(
            [this, &res](const auto& m)
            {
                res += fmt::format(
                    "    using {} = {};\n",
                    m.name,
                    make_impl_path(m.impl_name));
            });

        return res;
    }

    std::string make_types_struct_name() const
    {
        std::size_t index{};

        auto name = fmt::format("types_{}", index);
        while(types->contains(name))
        {
            index++;
            name = fmt::format("types_{}", index);
        }

        return name;
    }

    std::string make_messages_struct_name() const
    {
        std::size_t index{};

        auto name = fmt::format("messages_{}", index);
        while(messages->contains(name))
        {
            index++;
            name = fmt::format("messages_{}", index);
        }

        return name;
    }

    std::string generate()
    {
        make_type_tags_impl();
        make_message_tags_impl();

        // names `types/messages` cannot be just hard-coded here because
        // there could be type or message with the same name.
        const auto types_struct_name = make_types_struct_name();
        const auto messages_struct_name = make_messages_struct_name();

        schema->tag = fmt::format("::{}::schema", schema->name);

        return fmt::format(
            // clang-format off
R"(namespace detail
{{
namespace schema
{{
{detail}

struct {types_struct_name}
{{
{type_aliases}
}};

struct {messages_struct_name}
{{
{message_aliases}
}};
}} // namespace schema
}} // namespace detail

struct schema
{{
    using types = {types_struct_path};
    using messages = {messages_struct_path};
}};
)",
            // clang-format on
            fmt::arg("detail", detail),
            fmt::arg("types_struct_name", types_struct_name),
            fmt::arg("messages_struct_name", messages_struct_name),
            fmt::arg("types_struct_path", make_impl_path(types_struct_name)),
            fmt::arg(
                "messages_struct_path", make_impl_path(messages_struct_name)),
            fmt::arg("type_aliases", make_type_aliases()),
            fmt::arg("message_aliases", make_message_aliases()));
    }
};
} // namespace sbepp::sbeppc
