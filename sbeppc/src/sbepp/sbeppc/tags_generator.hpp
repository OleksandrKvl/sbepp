// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/source_location.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/context_manager.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <utility>
#include <vector>
#include <string>
#include <unordered_map>

namespace sbepp::sbeppc
{
class tags_generator
{
public:
    static std::string generate(
        const sbe::message_schema& schema, context_manager& ctx_manager)
    {
        tags_generator generator{schema, ctx_manager};
        return generator.generate();
    }

private:
    const sbe::message_schema* schema{};
    context_manager* ctx_manager{};
    std::unordered_map<std::string_view, bool> processed_types;
    std::string detail_types;
    std::string public_types;
    std::string detail_messages;
    std::string public_messages;
    std::vector<std::string> path;

    tags_generator(
        const sbe::message_schema& schema, context_manager& ctx_manager)
        : schema{&schema}, ctx_manager{&ctx_manager}
    {
    }

    std::string make_public_tag(const std::string_view last) const
    {
        return fmt::format(
            "::{}::schema::{}::{}",
            ctx_manager->get(*schema).name,
            fmt::join(path, "::"),
            last);
    }

    std::string make_type_impl_path(const std::string_view impl_name) const
    {
        return fmt::format(
            "::{}::detail::schema::types::{}",
            ctx_manager->get(*schema).name,
            impl_name);
    }

    std::string make_message_impl_path(const std::string_view impl_name) const
    {
        return fmt::format(
            "::{}::detail::schema::messages::{}",
            ctx_manager->get(*schema).name,
            impl_name);
    }

    std::string make_tag(const sbe::type& t)
    {
        auto& context = ctx_manager->get(t);
        context.tag = make_public_tag(t.name);

        return fmt::format(
            "struct {}{{}};\n", context.mangled_name.value_or(t.name));
    }

    std::string make_enum_value_tags(const sbe::enumeration& e)
    {
        std::string res;
        for(auto& value : e.valid_values)
        {
            res += fmt::format("    struct {}{{}};\n", value.name);
            ctx_manager->create(value).tag = make_public_tag(value.name);
        }
        return res;
    }

    std::string make_tag(const sbe::enumeration& e)
    {
        auto& context = ctx_manager->get(e);
        context.tag = make_public_tag(e.name);
        path.push_back(e.name);
        const auto valid_value_tags = make_enum_value_tags(e);
        path.pop_back();

        return fmt::format(
            // clang-format off
R"(struct {name}
{{
{valid_value_tags}
}};
)",
            // clang-format on
            fmt::arg("name", context.mangled_name.value_or(e.name)),
            fmt::arg("valid_value_tags", valid_value_tags));
    }

    std::string make_set_choice_tags(const sbe::set& s)
    {
        std::string res;
        for(auto& choice : s.choices)
        {
            res += fmt::format("    struct {}{{}};\n", choice.name);
            ctx_manager->create(choice).tag = make_public_tag(choice.name);
        }

        return res;
    }

    std::string make_tag(const sbe::set& s)
    {
        auto& context = ctx_manager->get(s);
        context.tag = make_public_tag(s.name);
        path.push_back(s.name);
        const auto choice_tags = make_set_choice_tags(s);
        path.pop_back();

        return fmt::format(
            // clang-format off
R"(struct {name}
{{
{choice_tags}
}};
)",
            // clang-format on
            fmt::arg("name", context.mangled_name.value_or(s.name)),
            fmt::arg("choice_tags", choice_tags));
    }

    std::string make_composite_element_tags(const sbe::composite& c)
    {
        std::string res;

        for(auto& member : c.elements)
        {
            std::visit(
                utils::overloaded{
                    [this, &res](const sbe::ref& r)
                    {
                        res += make_tag(r);
                    },
                    [this, &res](const auto& enc)
                    {
                        const auto& ctx = ctx_manager->get(enc);
                        const auto tag = make_tag(enc);
                        if(ctx.mangled_name)
                        {
                            detail_types += tag;
                            res += utils::make_type_alias(
                                enc.name,
                                make_type_impl_path(*ctx.mangled_name));
                        }
                        else
                        {
                            res += tag;
                        }
                    }},
                member);
            res += '\n';
        }

        return res;
    }

    std::string make_tag(const sbe::composite& c)
    {
        auto& context = ctx_manager->get(c);
        context.tag = make_public_tag(c.name);

        path.push_back(c.name);
        const auto element_tags = make_composite_element_tags(c);
        path.pop_back();

        return fmt::format(
            // clang-format off
R"(struct {name}
{{
{element_tags}
}};
)",
            // clang-format on
            fmt::arg("name", context.mangled_name.value_or(c.name)),
            fmt::arg("element_tags", element_tags));
    }

    std::optional<std::string> get_nested_type_name(const sbe::encoding& enc)
    {
        return std::visit(
            utils::overloaded{
                [](const sbe::type&) -> std::optional<std::string>
                {
                    return {};
                },
                [this](const auto& nested_type) -> std::optional<std::string>
                {
                    return ctx_manager->get(nested_type)
                        .mangled_name.value_or(nested_type.name);
                }},
            enc);
    }

    std::string make_tag(const sbe::ref& r)
    {
        auto& enc = utils::get_schema_encoding(*schema, r.type);
        handle_public_encoding(enc);

        ctx_manager->get(r).tag = make_public_tag(r.name);

        const auto referred_type_name = get_nested_type_name(enc);
        if(referred_type_name)
        {
            // referred type has nested elements so use inheritance
            return fmt::format(
                "struct {name} : {referred_type}{{}};\n",
                fmt::arg("name", r.name),
                fmt::arg(
                    "referred_type", make_type_impl_path(*referred_type_name)));
        }
        else
        {
            return fmt::format("struct {}{{}};\n", r.name);
        }
    }

    void handle_public_encoding(const sbe::encoding& encoding)
    {
        // always start with empty path
        auto prev_path = std::exchange(path, {});
        path.emplace_back("types");

        std::visit(
            [this](const auto& enc)
            {
                if(!processed_types[enc.name])
                {
                    const auto tag = make_tag(enc);
                    // public types always go into detail
                    detail_types += tag;
                    // add alias to public_types whether it's mangled or not
                    public_types += utils::make_type_alias(
                        enc.name,
                        make_type_impl_path(
                            ctx_manager->get(enc).mangled_name.value_or(
                                enc.name)));
                    public_types += '\n';

                    processed_types[enc.name] = true;
                }
            },
            encoding);

        std::swap(path, prev_path);
    }

    void make_type_tags_impl()
    {
        for(const auto& [name, enc] : schema->types)
        {
            handle_public_encoding(enc);
        }
    }

    std::optional<std::string>
        get_nested_type_tag(const std::string_view type_name)
    {
        if(!utils::is_primitive_type(type_name))
        {
            const auto& enc = utils::get_schema_encoding(*schema, type_name);
            return std::visit(
                utils::overloaded{
                    [](const sbe::type&) -> std::optional<std::string>
                    {
                        return {};
                    },
                    [this](const auto& actual_enc) -> std::optional<std::string>
                    {
                        return make_type_impl_path(
                            ctx_manager->get(actual_enc)
                                .mangled_name.value_or(actual_enc.name));
                    }},
                enc);
        }

        return {};
    }

    std::string make_field_tags(const std::vector<sbe::field>& fields)
    {
        std::string res;

        for(auto& field : fields)
        {
            ctx_manager->get(field).tag = make_public_tag(field.name);

            if(const auto type_tag = get_nested_type_tag(field.type))
            {
                res += fmt::format(
                    "    struct {name} : {type_tag}{{}};\n",
                    fmt::arg("name", field.name),
                    fmt::arg("type_tag", *type_tag));
            }
            else
            {
                res += fmt::format("    struct {}{{}};\n", field.name);
            }
        }

        return res;
    }

    std::string make_group_tags(const std::vector<sbe::group>& groups)
    {
        std::string res;

        for(auto& g : groups)
        {
            auto& context = ctx_manager->get(g);
            context.tag = make_public_tag(g.name);
            path.push_back(g.name);

            auto group_tag = fmt::format(
                // clang-format off
R"(struct {group}
{{
{members}
}};
)",
                // clang-format on
                fmt::arg("group", context.mangled_name.value_or(g.name)),
                fmt::arg("members", make_member_tags(g.members)));
            path.pop_back();

            if(context.mangled_name)
            {
                detail_messages += group_tag;
                res += fmt::format(
                    "    using {} = {};\n",
                    g.name,
                    make_message_impl_path(*context.mangled_name));
            }
            else
            {
                res += group_tag;
            }
        }

        return res;
    }

    std::string make_data_tags(const std::vector<sbe::data>& data_members)
    {
        std::string res;

        for(auto& data : data_members)
        {
            ctx_manager->create(data).tag = make_public_tag(data.name);
            res += fmt::format("    struct {}{{}};\n", data.name);
        }

        return res;
    }

    std::string make_member_tags(const sbe::level_members& members)
    {
        std::string res;

        res += make_field_tags(members.fields);
        res += make_group_tags(members.groups);
        res += make_data_tags(members.data);

        return res;
    }

    std::string make_message_tag(const sbe::message& m)
    {
        auto& context = ctx_manager->get(m);
        context.tag = make_public_tag(m.name);
        path.push_back(m.name);

        auto message_tag = fmt::format(
            // clang-format off
R"(struct {name}
{{
{members}
}};
)",
            // clang-format on
            fmt::arg("name", context.mangled_name.value_or(m.name)),
            fmt::arg("members", make_member_tags(m.members)));

        path.pop_back();

        if(context.mangled_name)
        {
            detail_messages += message_tag;
            return utils::make_type_alias(
                m.name, make_message_impl_path(*context.mangled_name));
        }
        else
        {
            return message_tag;
        }
    }

    void make_message_tags_impl()
    {
        path.emplace_back("messages");

        for(const auto& m : schema->messages)
        {
            public_messages += make_message_tag(m);
            public_messages += '\n';
        }

        path.pop_back();
    }

    std::string generate()
    {
        make_type_tags_impl();
        make_message_tags_impl();

        auto& context = ctx_manager->get(*schema);
        context.tag = fmt::format("::{}::schema", context.name);

        std::string res;

        if(!detail_types.empty() || !detail_messages.empty()
           || context.mangled_tag_types_name
           || context.mangled_tag_messages_name)
        {
            // clang-format off
            res +=
R"(namespace detail
{
namespace schema
{
)";
            // clang-format on

            if(!detail_types.empty())
            {
                res += fmt::format(
                    // clang-format off
R"(namespace types
{{
{detail_types}
}} // namespace types
)"
,
                    // clang-format on
                    fmt::arg("detail_types", detail_types));
            }

            if(!detail_messages.empty())
            {
                res += fmt::format(
                    // clang-format off
R"(namespace messages
{{
{detail_messages}
}} // namespace types
)"
,
                    // clang-format on
                    fmt::arg("detail_messages", detail_messages));
            }

            if(context.mangled_tag_types_name)
            {
                res += fmt::format(
                    // clang-format off
R"(struct {mangled_tag_types_name}
{{
{public_types}
}};
)",
                    // clang-format on
                    fmt::arg(
                        "mangled_tag_types_name",
                        *context.mangled_tag_types_name),
                    fmt::arg("public_types", public_types));
            }

            if(context.mangled_tag_messages_name)
            {
                res += fmt::format(
                    // clang-format off
R"(struct {mangled_tag_messages_name}
{{
{public_messages}
}};
)",
                    // clang-format on
                    fmt::arg(
                        "mangled_tag_messages_name",
                        *context.mangled_tag_messages_name),
                    fmt::arg("public_messages", public_messages));
            }

            // clang-format off
            res +=
R"(} //namespace schema
} // namespace detail
)";
            // clang-format on
        }

        // clang-format off
        res +=
R"(struct schema
{
)";
        // clang-format on

        if(context.mangled_tag_types_name)
        {
            res += fmt::format(
                "using types = ::{}::detail::schema::{};\n",
                context.name,
                *context.mangled_tag_types_name);
        }
        else
        {
            res += fmt::format(
                // clang-format off
R"(struct types
{{
{public_types}
}};
)",
                // clang-format on
                fmt::arg("public_types", public_types));
        }

        if(context.mangled_tag_messages_name)
        {
            res += fmt::format(
                "using messages = ::{}::detail::schema::{};\n",
                context.name,
                *context.mangled_tag_messages_name);
        }
        else
        {
            res += fmt::format(
                // clang-format off
R"(struct messages
{{
{public_messages}
}};
)",
                // clang-format on
                fmt::arg("public_messages", public_messages));
        }

        // clang-format off
        res +=
R"(};
)";
        // clang-format on

        return res;
    }
};
} // namespace sbepp::sbeppc
