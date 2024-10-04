// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/source_location.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/context_manager.hpp>

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
        const sbe::message_schema& schema, context_manager& ctx_manager)
        : schema{&schema}, ctx_manager{&ctx_manager}
    {
        generated = generate();
    }

    const std::string& get() const
    {
        return generated;
    }

private:
    const sbe::message_schema* schema{};
    context_manager* ctx_manager{};
    std::string generated;
    std::unordered_map<std::string_view, bool> processed_types;
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
            "::{}::schema::{}::{}",
            ctx_manager->get(*schema).name,
            fmt::join(path, "::"),
            last);
    }

    std::string make_impl_path(const std::string_view impl_name) const
    {
        // no need for `types/messages` here because in `detail` we have only
        // `impl_name`s which cannot conflict with each other
        return fmt::format(
            "::{}::detail::schema::{}",
            ctx_manager->get(*schema).name,
            impl_name);
    }

    std::string make_tag(const sbe::type& t, std::vector<std::string>& path)
    {
        auto& context = ctx_manager->get(t);
        context.impl_name = make_next_type_name();
        context.tag = make_public_tag(path, t.name);

        return fmt::format("struct {}{{}};\n", context.impl_name);
    }

    std::string make_enum_value_tags(
        const sbe::enumeration& e, std::vector<std::string>& path)
    {
        std::string res;
        for(auto& value : e.valid_values)
        {
            res += fmt::format("    struct {}{{}};\n", value.name);
            ctx_manager->get(value).tag = make_public_tag(path, value.name);
        }
        return res;
    }

    std::string
        make_tag(const sbe::enumeration& e, std::vector<std::string>& path)
    {
        auto& context = ctx_manager->get(e);
        context.impl_name = make_next_enum_name(e);
        context.tag = make_public_tag(path, e.name);

        path.push_back(e.name);

        auto res = fmt::format(
            // clang-format off
R"(struct {name}
{{
{enum_value_tags}
}};
)",
            // clang-format on
            fmt::arg("name", context.impl_name),
            fmt::arg("enum_value_tags", make_enum_value_tags(e, path)));

        path.pop_back();

        return res;
    }

    std::string
        make_set_choice_tags(const sbe::set& s, std::vector<std::string>& path)
    {
        std::string res;
        for(auto& choice : s.choices)
        {
            res += fmt::format("    struct {}{{}};\n", choice.name);
            ctx_manager->get(choice).tag = make_public_tag(path, choice.name);
        }

        return res;
    }

    std::string make_tag(const sbe::set& s, std::vector<std::string>& path)
    {
        auto& context = ctx_manager->get(s);
        context.impl_name = make_next_set_name(s);
        context.tag = make_public_tag(path, s.name);
        path.push_back(s.name);

        auto res = fmt::format(
            // clang-format off
R"(struct {name}
{{
{choice_tags}
}};
)",
            // clang-format on
            fmt::arg("name", context.impl_name),
            fmt::arg("choice_tags", make_set_choice_tags(s, path)));

        path.pop_back();

        return res;
    }

    std::string make_composite_element_tags(
        const sbe::composite& c, std::vector<std::string>& path)
    {
        std::string res;

        for(auto& member : c.elements)
        {
            res += std::visit(
                utils::overloaded{
                    [&path, this](const sbe::ref& r)
                    {
                        return make_tag(r, path);
                    },
                    [&path, this](auto& enc)
                    {
                        detail += make_tag(enc, path);
                        return fmt::format(
                            "using {} = {};\n",
                            enc.name,
                            make_impl_path(ctx_manager->get(enc).impl_name));
                    }},
                member);
        }

        return res;
    }

    std::string
        make_tag(const sbe::composite& c, std::vector<std::string>& path)
    {
        auto& context = ctx_manager->get(c);
        context.impl_name = make_next_composite_name(c);
        context.tag = make_public_tag(path, c.name);

        path.push_back(c.name);

        auto res = fmt::format(
            // clang-format off
R"(struct {name}
{{
{element_tags}
}};
)",
            // clang-format on
            fmt::arg("name", context.impl_name),
            fmt::arg("element_tags", make_composite_element_tags(c, path)));

        path.pop_back();

        return res;
    }

    std::string_view get_impl_name(const sbe::encoding& enc) const
    {
        return std::visit(
            [this](const auto& e) -> std::string_view
            {
                return ctx_manager->get(e).impl_name;
            },
            enc);
    }

    std::string make_tag(const sbe::ref& r, std::vector<std::string>& path)
    {
        auto& enc = utils::get_schema_encoding(*schema, r.type);
        handle_public_encoding(enc);

        ctx_manager->get(r).tag = make_public_tag(path, r.name);

        return fmt::format(
            "struct {name} : {type}{{}};\n",
            fmt::arg("name", r.name),
            fmt::arg("type", get_impl_name(enc)));
    }

    void handle_public_encoding(const sbe::encoding& encoding)
    {
        std::vector<std::string> path;
        path.emplace_back("types");

        const auto name = utils::get_encoding_name(encoding);
        if(!processed_types[name])
        {
            detail += std::visit(
                [&path, this](const auto& enc)
                {
                    return make_tag(enc, path);
                },
                encoding);

            processed_types[name] = true;
        }
    }

    void make_type_tags_impl()
    {
        for(const auto& [name, enc] : schema->types)
        {
            handle_public_encoding(enc);
        }
    }

    const sbe::composite* try_get_composite(const std::string& type_name) const
    {
        if(!utils::is_primitive_type(type_name))
        {
            return std::get_if<sbe::composite>(
                &utils::get_schema_encoding(*schema, type_name));
        }
        return {};
    }

    std::string make_field_tags(
        const std::vector<sbe::field>& fields, std::vector<std::string>& path)
    {
        std::string res;

        for(auto& field : fields)
        {
            ctx_manager->get(field).tag = make_public_tag(path, field.name);

            if(const auto c = try_get_composite(field.type))
            {
                res += fmt::format(
                    "    struct {name} : {impl}{{}};\n",
                    fmt::arg("name", field.name),
                    fmt::arg(
                        "impl",
                        make_impl_path(ctx_manager->get(*c).impl_name)));
            }
            else
            {
                res += fmt::format("    struct {}{{}};\n", field.name);
            }
        }

        return res;
    }

    std::string make_group_tags(
        const std::vector<sbe::group>& groups, std::vector<std::string>& path)
    {
        std::string res;

        for(auto& g : groups)
        {
            auto& context = ctx_manager->get(g);
            context.impl_name = make_next_group_name(g.members);
            context.tag = make_public_tag(path, g.name);
            path.push_back(g.name);

            detail += fmt::format(
                // clang-format off
R"(struct {group}
{{
{members}
}};
)",
                // clang-format on
                fmt::arg("group", context.impl_name),
                fmt::arg("members", make_member_tags(g.members, path)));

            res += fmt::format(
                "    using {} = {};\n",
                g.name,
                make_impl_path(context.impl_name));
            path.pop_back();
        }

        return res;
    }

    std::string make_data_tags(
        const std::vector<sbe::data>& data_members,
        const std::vector<std::string>& path)
    {
        std::string res;

        for(auto& data : data_members)
        {
            ctx_manager->get(data).tag = make_public_tag(path, data.name);
            res += fmt::format("    struct {}{{}};\n", data.name);
        }

        return res;
    }

    std::string make_member_tags(
        const sbe::level_members& members, std::vector<std::string>& path)
    {
        std::string res;

        res += make_field_tags(members.fields, path);
        res += make_group_tags(members.groups, path);
        res += make_data_tags(members.data, path);

        return res;
    }

    std::string
        make_message_tag(const sbe::message& m, std::vector<std::string>& path)
    {
        auto& context = ctx_manager->get(m);
        context.impl_name = make_next_message_name(m.members);
        context.tag = make_public_tag(path, m.name);
        path.push_back(m.name);

        auto res = fmt::format(
            // clang-format off
R"(struct {name}
{{
{members}
}};
)",
            // clang-format on
            fmt::arg("name", context.impl_name),
            fmt::arg("members", make_member_tags(m.members, path)));

        path.pop_back();

        return res;
    }

    void make_message_tags_impl()
    {
        std::vector<std::string> path;
        path.emplace_back("messages");

        for(const auto& m : schema->messages)
        {
            detail += make_message_tag(m, path);
        }
    }

    std::string make_type_aliases()
    {
        std::string res;
        for(const auto& [name, enc] : schema->types)
        {
            res += std::visit(
                [this](const auto& enc)
                {
                    return fmt::format(
                        "    using {} = {};\n",
                        enc.name,
                        make_impl_path(ctx_manager->get(enc).impl_name));
                },
                enc);
        }

        return res;
    }

    std::string make_message_aliases()
    {
        std::string res;
        for(const auto& m : schema->messages)
        {
            res += fmt::format(
                "    using {} = {};\n",
                m.name,
                make_impl_path(ctx_manager->get(m).impl_name));
        }

        return res;
    }

    std::string make_types_struct_name() const
    {
        std::size_t index{};

        auto name = fmt::format("types_{}", index);
        while(schema->types.count(name))
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
        // TODO: refactor
        const auto contains = [this](const auto& name)
        {
            const auto search = std::find_if(
                std::begin(schema->messages),
                std::end(schema->messages),
                [&name](const auto& m)
                {
                    return m.name == name;
                });

            return search != std::end(schema->messages);
        };

        while(contains(name))
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

        auto& context = ctx_manager->get(*schema);
        context.tag = fmt::format("::{}::schema", context.name);

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
