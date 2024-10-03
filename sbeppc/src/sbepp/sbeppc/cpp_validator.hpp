// SPDX-License-Identifier: MIT
// Copyright (c) 2024, Oleksandr Koval

#pragma once

#include "sbepp/sbeppc/source_location.hpp"
#include "sbepp/sbeppc/throw_error.hpp"
#include <sbepp/sbeppc/ireporter.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/context_manager.hpp>

#include <string_view>
#include <unordered_set>
#include <variant>

namespace sbepp::sbeppc
{
class cpp_validator
{
public:
    cpp_validator(ireporter& reporter, context_manager& ctx_manager)
        : reporter{&reporter}, ctx_manager{&ctx_manager}
    {
    }

    // TODO: test
    void validate(const sbe::message_schema& schema)
    {
        this->schema = &schema;

        // at this point all names except schema one satisfy
        // `utils::is_sbe_symbolic_name` so they have valid C++ name format but
        // we still need to check them against reserved C++ names or keywords
        validate_schema_name();
        validate_type_names();
        validate_message_names();
    }

private:
    ireporter* reporter{};
    context_manager* ctx_manager{};
    const sbe::message_schema* schema{};

    void validate_level_members(const sbe::level_members& members)
    {
        for(const auto& f : members.fields)
        {
            validate_name(f);
        }

        for(const auto& g : members.groups)
        {
            validate_name(g);
            validate_level_members(g.members);
        }

        for(const auto& d : members.data)
        {
            validate_name(d);
        }
    }

    void validate_message(const sbe::message& m)
    {
        validate_name(m);
        validate_level_members(m.members);
    }

    void validate_message_names()
    {
        for(const auto& m : schema->messages)
        {
            validate_message(m);
        }
    }

    void validate_encoding(const sbe::type& t)
    {
        validate_name(t);
    }

    void validate_encoding(const sbe::enumeration& e)
    {
        validate_name(e);

        for(const auto& value : e.valid_values)
        {
            validate_name(value);
        }
    }

    void validate_encoding(const sbe::set& s)
    {
        validate_name(s);

        for(const auto& choice : s.choices)
        {
            validate_name(choice);
        }
    }

    void validate_encoding(const sbe::composite& c)
    {
        validate_name(c);

        for(const auto& element : c.elements)
        {
            std::visit(
                [this](const auto& enc)
                {
                    validate_encoding(enc);
                },
                element);
        }
    }

    void validate_encoding(const sbe::ref& r)
    {
        validate_name(r);
        // referred type will be checked during main loop over public types
    }

    void validate_type_names()
    {
        // iterate over all public/private types and their children
        for(const auto& [name, enc] : schema->types)
        {
            std::visit(
                [this](const auto& enc)
                {
                    validate_encoding(enc);
                },
                enc);
        }
    }

    void validate_schema_name()
    {
        // TODO: should we provide different error message for custom or
        // XML name?
        const auto& name = ctx_manager->get(*schema).name;
        if(!utils::is_sbe_symbolic_name(name) || !is_cpp_keyword(name)
           || is_reserved_cpp_namespace(name))
        {
            throw_error(
                "{}: schema name `{}` is not a valid C++ namespace. Change "
                "`messageSchema.package` attribute or provide a custom name "
                "using `--schema-name` sbeppc option",
                schema->location,
                name);
        }

        warn_about_reserved_identifier(name, schema->location);
    }

    static bool is_reserved_cpp_namespace(const std::string_view str)
    {
        return (str == "std") || (str == "posix");
    }

    static bool is_cpp_keyword(const std::string_view str)
    {
        static const std::unordered_set<std::string_view> cpp_keywords{
            "alignas",       "alignof",     "and",
            "and_eq",        "asm",         "auto",
            "bitand",        "bitor",       "bool",
            "break",         "case",        "catch",
            "char",          "char8_t",     "char16_t",
            "char32_t",      "class",       "compl",
            "concept",       "const",       "consteval",
            "constexpr",     "constinit",   "const_cast",
            "continue",      "co_await",    "co_return",
            "co_yield",      "decltype",    "default",
            "delete",        "do",          "double",
            "dynamic_cast",  "else",        "enum",
            "explicit",      "export",      "extern",
            "false",         "float",       "for",
            "friend",        "goto",        "if",
            "inline",        "int",         "long",
            "mutable",       "namespace",   "new",
            "noexcept",      "not",         "not_eq",
            "nullptr",       "operator",    "or",
            "or_eq",         "private",     "protected",
            "public",        "register",    "reinterpret_cast",
            "requires",      "return",      "short",
            "signed",        "sizeof",      "static",
            "static_assert", "static_cast", "struct",
            "switch",        "template",    "this",
            "thread_local",  "throw",       "true",
            "try",           "typedef",     "typeid",
            "typename",      "union",       "unsigned",
            "using",         "virtual",     "void",
            "volatile",      "wchar_t",     "while",
            "xor",           "xor_eq"};

        return cpp_keywords.count(str);
    }

    static bool is_reserved_cpp_identifier(const std::string_view str)
    {
        if(str.find("__") != std::string_view::npos)
        {
            return true;
        }

        if((str.size() > 1) && (str[0] == '_')
           && (std::isupper(static_cast<unsigned char>(str[1]))))
        {
            return true;
        }

        return false;
    }

    void warn_about_reserved_identifier(
        const std::string_view name, const source_location& location)
    {
        if(is_reserved_cpp_identifier(name))
        {
            reporter->warning(
                "{}: name `{}` is a reserved C++ identifier\n", location, name);
        }
    }

    void validate_name(
        const std::string_view name, const source_location& location)
    {
        if(!is_cpp_keyword(name))
        {
            throw_error("{}: `{}` is not a valid C++ name", location, name);
        }

        warn_about_reserved_identifier(name, location);
    }

    template<typename T>
    void validate_name(const T& entity)
    {
        validate_name(entity.name, entity.location);
    }
};
} // namespace sbepp::sbeppc