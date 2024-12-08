#pragma once

#include <sbepp/sbeppc/source_location.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/context_manager.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>

#include <fmt/format.h>

#include <unordered_set>
#include <variant>
#include <limits>

namespace sbepp::sbeppc
{
// This class is responsible for checking and generating mangled or non-mangled
// (if possible) names for types and messages. If non-mangled name is not
// possible, it tries to preserve the original name by adding a numerical suffix
// to it. The main goal is to keep names in compiler error messages
// understandable for a user.
class names_generator
{
public:
    void generate(
        const sbe::message_schema& schema, context_manager& ctx_manager)
    {
        this->schema = &schema;
        this->ctx_manager = &ctx_manager;
        // TODO: clear all data members

        generate_type_names();
        generate_message_names();
    }

private:
    const sbe::message_schema* schema{};
    context_manager* ctx_manager{};
    // all public type names
    std::unordered_set<std::string> non_mangled_type_names;
    // mangled public and anonymous (defined within composites) type names
    std::unordered_set<std::string> mangled_type_names;
    // these are similar to the above one but are message-related
    std::unordered_set<std::string> non_mangled_message_names;
    std::unordered_set<std::string> mangled_message_names;

    static std::unordered_set<std::string> get_member_names(const sbe::type& t)
    {
        if((t.presence == field_presence::constant) || (t.length != 1))
        {
            // constants/arrays are represented using `static_array_ref` or
            // naked types, they have no direct members
            return {};
        }
        else if(t.presence == field_presence::required)
        {
            return {"min_value", "max_value"};
        }
        else
        {
            return {"min_value", "max_value", "null_value"};
        }
    }

    static std::unordered_set<std::string>
        get_member_names(const sbe::enumeration& e)
    {
        std::unordered_set<std::string> members;

        for(const auto& valid_value : e.valid_values)
        {
            members.insert(valid_value.name);
        }

        return members;
    }

    static std::unordered_set<std::string> get_member_names(const sbe::set& s)
    {
        std::unordered_set<std::string> members;

        for(const auto& choice : s.choices)
        {
            members.insert(choice.name);
        }

        return members;
    }

    static std::unordered_set<std::string>
        get_member_names(const sbe::composite& c)
    {
        std::unordered_set<std::string> members;

        for(const auto& element : c.elements)
        {
            std::visit(
                [&members](const auto& actual_element)
                {
                    members.insert(actual_element.name);
                },
                element);
        }

        return members;
    }

    template<typename... UnorderedSets>
    static std::string make_mangled_name(
        const std::string_view original_name,
        const source_location& location,
        const UnorderedSets&... reserved_names)
    {
        // TODO: should we start from `2`?
        for(std::size_t n = 0; n != std::numeric_limits<std::size_t>::max();
            n++)
        {
            const auto mangled_name = fmt::format("{}_{}", original_name, n);
            const auto is_reserved =
                (reserved_names.count(mangled_name) || ...);
            if(!is_reserved)
            {
                return mangled_name;
            }
        }

        throw_error(
            "{}: can't generate a mangled name for `{}`",
            location,
            original_name);
    }

    void collect_non_mangled_type_names()
    {
        for(const auto& [name, enc] : schema->types)
        {
            std::visit(
                [this](const auto& actual_enc)
                {
                    non_mangled_type_names.insert(actual_enc.name);
                },
                enc);
        }
    }

    void handle_composite_elements(
        const std::vector<sbe::composite_element>& elements)
    {
        for(const auto& e : elements)
        {
            // this is similar to public types handling with one difference: we
            // always add type name (mangled or not) to the `mangled_type_names`
            // since implementation types are in the same enclosing namespace

            std::visit(
                utils::overloaded{
                    [](const sbe::ref&)
                    {
                        // ref tag is always located directly inside composite
                        // and it doesn't have any implementation type
                    },
                    [this](const auto& actual_enc)
                    {
                        const auto members = get_member_names(actual_enc);
                        // composite elements (except ref-s) are implemented in
                        // `detail::types` namespace

                        // should not clash with its members
                        if(members.count(actual_enc.name)
                           // should not clash with other mangled types.
                           // Composite elements except ref-s are implemented in
                           // `detail::types` namespace. Although their tags may
                           // be put directly inside the composite's tag, we
                           // want tags and implementation types to have the
                           // same name.
                           || mangled_type_names.count(actual_enc.name))
                        {
                            const auto mangled_name = make_mangled_name(
                                actual_enc.name,
                                actual_enc.location,
                                members,
                                mangled_type_names,
                                non_mangled_type_names);
                            mangled_type_names.insert(mangled_name);
                            ctx_manager->get(actual_enc).mangled_name =
                                mangled_name;
                        }
                        else
                        {
                            mangled_type_names.insert(actual_enc.name);
                        }

                        if constexpr(std::is_same_v<
                                         decltype(actual_enc),
                                         const sbe::composite&>)
                        {
                            handle_composite_elements(actual_enc.elements);
                        }
                    }},
                e);
        }
    }

    void generate_type_names()
    {
        // collect all non-mangled names to avoid matching of mangled and
        // non-mangled names. Although such a match won't lead to an error
        // because those names will be in different namespaces, it will be just
        // confusing
        collect_non_mangled_type_names();

        for(const auto& [name, enc] : schema->types)
        {
            std::visit(
                [this](const auto& actual_enc)
                {
                    const auto members = get_member_names(actual_enc);
                    if(members.count(actual_enc.name))
                    {
                        // encoding's name clashes with one of its members
                        // (either in implementation class or in tag structure)
                        // so we need a mangled name for it
                        const auto mangled_name = make_mangled_name(
                            actual_enc.name,
                            actual_enc.location,
                            members,
                            mangled_type_names,
                            non_mangled_type_names);
                        mangled_type_names.insert(mangled_name);
                        ctx_manager->get(actual_enc).mangled_name =
                            mangled_name;
                    }

                    // TODO: refactor?
                    if constexpr(std::is_same_v<
                                     decltype(actual_enc),
                                     const sbe::composite&>)
                    {
                        handle_composite_elements(actual_enc.elements);
                    }
                },
                enc);
        }

        // implementation types are located inside `types` namespace but tags
        // are inside `types` structure and it's not possible to have a child
        // structure for a tag with the same name so we need a mangled name for
        // it
        if(non_mangled_type_names.count("types"))
        {
            const auto mangled_tag_types_name = make_mangled_name(
                "types", schema->location, non_mangled_type_names);
            ctx_manager->get(*schema).mangled_tag_types_name =
                mangled_tag_types_name;
        }
    }

    void collect_non_mangled_message_names()
    {
        for(const auto& m : schema->messages)
        {
            non_mangled_message_names.insert(m.name);
        }
    }

    static std::unordered_set<std::string>
        get_member_names(const sbe::level_members& members)
    {
        std::unordered_set<std::string> level_names;

        for(const auto& f : members.fields)
        {
            level_names.insert(f.name);
        }

        for(const auto& g : members.groups)
        {
            level_names.insert(g.name);
        }

        for(const auto& d : members.data)
        {
            level_names.insert(d.name);
        }

        return level_names;
    }

    static std::string make_group_entry_name(const std::string_view group_name)
    {
        return fmt::format("{}_entry", group_name);
    }

    struct mangled_group_info
    {
        std::string group_name;
        std::string entry_name;
    };

    template<typename... UnorderedSets>
    static mangled_group_info make_mangled_group_info(
        const std::string_view original_name,
        const source_location& location,
        const std::unordered_set<std::string>& entry_members,
        const UnorderedSets&... reserved_names)
    {
        // TODO: should we start from `2`?
        for(std::size_t n = 0; n != std::numeric_limits<std::size_t>::max();
            n++)
        {
            const auto mangled_group_name =
                fmt::format("{}_{}", original_name, n);
            const auto entry_name = make_group_entry_name(mangled_group_name);
            const auto is_reserved =
                entry_members.count(mangled_group_name)
                || (reserved_names.count(mangled_group_name) || ...)
                || entry_members.count(entry_name)
                || (reserved_names.count(entry_name) || ...);
            if(!is_reserved)
            {
                return {mangled_group_name, entry_name};
            }
        }

        throw_error(
            "{}: can't generate a mangled name for `{}`",
            location,
            original_name);
    }

    void handle_message_level(const sbe::level_members& members)
    {
        // field names are never mangled because they never introduce new types,
        // only refer to the existing ones. Their tags are never mangled too.

        for(const auto& g : members.groups)
        {
            // group introduces a new level, although its name is preserved on
            // the enclosing level, it and its entry needs a mangled name

            // group implementation type itself doesn't have any named members
            const auto entry_members = get_member_names(g.members);
            const auto entry_name = make_group_entry_name(g.name);

            // we want to preserve group name in both its implementation type
            // and its entry type

            // groups and their entries are implemented in the same namespace as
            // mangled messages so their names should not clash
            if(mangled_message_names.count(g.name)
               || mangled_message_names.count(entry_name)
               || entry_members.count(entry_name)
               // group name should not clash with entry members because their
               // tags are located directly within the group's one
               || entry_members.count(g.name))
            {
                const auto mangled_group_info = make_mangled_group_info(
                    g.name,
                    g.location,
                    entry_members,
                    mangled_message_names,
                    non_mangled_message_names);
                mangled_message_names.insert(mangled_group_info.group_name);
                ctx_manager->get(g).mangled_name =
                    mangled_group_info.group_name;
                mangled_message_names.insert(mangled_group_info.entry_name);
                ctx_manager->get(g).entry_name = mangled_group_info.entry_name;
            }
            else
            {
                mangled_message_names.insert(g.name);
                mangled_message_names.insert(entry_name);
                ctx_manager->get(g).entry_name = entry_name;
            }

            handle_message_level(g.members);
        }

        // similar to fields, data tags are never mangled. They don't have
        // implementation type name, it's always `dynamic_array_ref`.
    }

    void generate_message_names()
    {
        collect_non_mangled_message_names();

        for(const auto& m : schema->messages)
        {
            const auto members = get_member_names(m.members);
            if(members.count(m.name))
            {
                // message name clashes with its member so we need a mangled
                // name for it
                const auto mangled_name = make_mangled_name(
                    m.name,
                    m.location,
                    members,
                    mangled_message_names,
                    non_mangled_message_names);
                mangled_message_names.insert(mangled_name);
                ctx_manager->get(m).mangled_name = mangled_name;
            }

            // handle message levels recursively, their names always go into
            // mangled_message_names.
            handle_message_level(m.members);
        }

        // implementation types are located inside `messages` namespace but tags
        // are inside `messages` structure and it's not possible to have a child
        // structure for a tag with the same name so we need a mangled name for
        // it
        if(non_mangled_message_names.count("messages"))
        {
            const auto mangled_tag_messages_name = make_mangled_name(
                "messages", schema->location, non_mangled_message_names);
            ctx_manager->get(*schema).mangled_tag_messages_name =
                mangled_tag_messages_name;
        }
    }
};
} // namespace sbepp::sbeppc