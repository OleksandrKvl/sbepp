// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/source_location.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/traits_generator.hpp>
#include <sbepp/sbeppc/normal_accessors.hpp>
#include <sbepp/sbeppc/context_manager.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <functional>
#include <cassert>

namespace sbepp::sbeppc
{
class types_compiler
{
public:
    using on_public_type_cb_t = std::function<void(
        const std::string_view name,
        const std::string_view implementation,
        const std::string_view alias,
        const std::unordered_set<std::string>& dependencies,
        const std::string_view traits)>;

    types_compiler(
        const sbe::message_schema& schema,
        traits_generator& traits_gen,
        context_manager& ctx_manager)
        : schema{&schema},
          traits_gen{&traits_gen},
          ctx_manager{&ctx_manager},
          schema_name{ctx_manager.get(schema).name},
          byte_order{schema.byte_order}
    {
    }

    void compile(on_public_type_cb_t cb)
    {
        on_public_type_cb = std::move(cb);

        for(const auto& [name, enc] : schema->types)
        {
            dependencies.emplace_back();
            compile_public_encoding(enc);
            dependencies.pop_back();
        }
    }

private:
    const sbe::message_schema* schema{};
    traits_generator* traits_gen{};
    context_manager* ctx_manager{};

    std::string_view schema_name;
    sbe::byte_order_kind byte_order;
    on_public_type_cb_t on_public_type_cb;
    std::unordered_map<std::string_view, bool> compiled_types;
    std::vector<std::unordered_set<std::string>> dependencies;

    static std::string get_min_value(const sbe::type& t)
    {
        static const std::unordered_map<std::string_view, std::string>
            built_in_min_values{
                {"char", "0x20"},
                {"int8", "-127"},
                {"int16", "-32767"},
                {"int32", "-2147483647"},
                {"int64", "-9223372036854775807"},
                {"uint8", "0"},
                {"uint16", "0"},
                {"uint32", "0"},
                {"uint64", "0"},
                {"float", "::std::numeric_limits<float>::min()"},
                {"double", "::std::numeric_limits<double>::min()"}};

        if(t.min_value)
        {
            return utils::numeric_literal_to_value(
                *t.min_value, t.primitive_type);
        }
        return built_in_min_values.at(t.primitive_type);
    }

    static std::string get_max_value(const sbe::type& t)
    {
        static const std::unordered_map<std::string_view, std::string>
            built_in_max_values{
                {"char", "0x7e"},
                {"int8", "127"},
                {"int16", "32767"},
                {"int32", "2147483647"},
                {"int64", "9223372036854775807"},
                {"uint8", "254"},
                {"uint16", "65534"},
                {"uint32", "4294967294"},
                {"uint64", "18446744073709551614UL"},
                {"float", "::std::numeric_limits<float>::max()"},
                {"double", "::std::numeric_limits<double>::max()"}};

        if(t.max_value)
        {
            return utils::numeric_literal_to_value(
                *t.max_value, t.primitive_type);
        }
        return built_in_max_values.at(t.primitive_type);
    }

    static std::string get_null_value(const sbe::type& t)
    {
        static const std::unordered_map<std::string_view, std::string>
            built_in_null_values{
                {"char", "0"},
                {"int8", "-128"},
                {"int16", "-327678"},
                {"int32", "-2147483648"},
                {"int64", "-9223372036854775807 - 1"},
                {"uint8", "255"},
                {"uint16", "65535"},
                {"uint32", "4294967295"},
                {"uint64", "18446744073709551615UL"},
                {"float", "::std::numeric_limits<float>::quiet_NaN()"},
                {"double", "::std::numeric_limits<double>::quiet_NaN()"}};

        if(t.null_value)
        {
            return utils::numeric_literal_to_value(
                *t.null_value, t.primitive_type);
        }
        return built_in_null_values.at(t.primitive_type);
    }

    std::string make_constant_type(const sbe::type& t) const
    {
        assert(t.presence == field_presence::constant);
        const auto& context = ctx_manager->get(t);

        if(t.length != 1)
        {
            return fmt::format(
                // clang-format off
R"(
using {type_name} = ::sbepp::detail::static_array_ref<
    const char, {element_type}, {length}, {tag}>;
)",
                // clang-format on
                fmt::arg("type_name", context.impl_name),
                fmt::arg("element_type", context.underlying_type),
                fmt::arg("length", t.length),
                fmt::arg("tag", context.tag));
        }
        else
        {
            return fmt::format(
                "using {} = {};\n", context.impl_name, context.underlying_type);
        }
    }

    std::string make_array_type(const sbe::type& t) const
    {
        assert((t.length != 1) && (t.presence != field_presence::constant));
        const auto& context = ctx_manager->get(t);

        return fmt::format(
            // clang-format off
R"(
template<typename Byte>
using {type_name} = ::sbepp::detail::static_array_ref<
    Byte, {element_type}, {length}, {tag}>;
)",
            // clang-format on
            fmt::arg("type_name", context.impl_name),
            fmt::arg("element_type", context.underlying_type),
            fmt::arg("length", t.length),
            fmt::arg("tag", context.tag));
    }

    std::string make_required_type(const sbe::type& t) const
    {
        assert((t.presence == field_presence::required) && (t.length == 1));
        const auto& context = ctx_manager->get(t);

        return fmt::format(
            // clang-format off
R"(
class {type_name}
    : public ::sbepp::detail::required_base<{underlying_type}, {type_name}>
{{
public:
    using ::sbepp::detail::required_base<
        {underlying_type}, {type_name}>::required_base;

    static constexpr value_type min_value() noexcept
    {{
        return {{{min_value}}};
    }}

    static constexpr value_type max_value() noexcept
    {{
        return {{{max_value}}};
    }}
}};
)",
            // clang-format on
            fmt::arg("type_name", context.impl_name),
            fmt::arg("underlying_type", context.underlying_type),
            fmt::arg("min_value", get_min_value(t)),
            fmt::arg("max_value", get_max_value(t)));
    }

    std::string make_optional_type(const sbe::type& t) const
    {
        assert((t.presence == field_presence::optional) && (t.length == 1));
        const auto& context = ctx_manager->get(t);

        return fmt::format(
            // clang-format off
R"(
class {type_name}
    : public ::sbepp::detail::optional_base<{underlying_type}, {type_name}>
{{
public:
    using ::sbepp::detail::optional_base<
        {underlying_type}, {type_name}>::optional_base;

    static constexpr value_type min_value() noexcept
    {{
        return {{{min_value}}};
    }}

    static constexpr value_type max_value() noexcept
    {{
        return {{{max_value}}};
    }}

    static constexpr value_type null_value() noexcept
    {{
        return {{{null_value}}};
    }}
}};
)",
            // clang-format on
            fmt::arg("type_name", context.impl_name),
            fmt::arg("underlying_type", context.underlying_type),
            fmt::arg("min_value", get_min_value(t)),
            fmt::arg("max_value", get_max_value(t)),
            fmt::arg("null_value", get_null_value(t)));
    }

    std::string compile_encoding(const sbe::type& t)
    {
        auto& context = ctx_manager->get(t);
        context.impl_type = fmt::format(
            "::{}::detail::types::{}", schema_name, context.impl_name);
        context.public_type = context.impl_type;
        context.is_template = false;
        context.underlying_type =
            utils::primitive_type_to_cpp_type(t.primitive_type);

        if(t.presence == field_presence::constant)
        {
            return make_constant_type(t);
        }
        else if(t.length != 1)
        {
            context.is_template = true;
            return make_array_type(t);
        }
        else if(t.presence == field_presence::required)
        {
            return make_required_type(t);
        }
        else
        {
            return make_optional_type(t);
        }
    }

    std::string make_enumerators(const sbe::enumeration& e) const
    {
        std::vector<std::string> enumerators;
        enumerators.reserve(e.valid_values.size());

        const auto& context = ctx_manager->get(e);
        const auto is_char_type = (context.underlying_type == "char");

        for(const auto& valid_value : e.valid_values)
        {
            if(is_char_type)
            {
                enumerators.push_back(fmt::format(
                    "    {} = '{}'", valid_value.name, valid_value.value));
            }
            else
            {
                enumerators.push_back(fmt::format(
                    "    {} = {}",
                    valid_value.name,
                    utils::to_integer_literal(
                        valid_value.value, context.primitive_type)));
            }
        }

        return fmt::format("{}", fmt::join(enumerators, ",\n"));
    }

    std::string make_enum_visit_impl(const sbe::enumeration& e) const
    {
        std::vector<std::string> switch_cases;
        switch_cases.reserve(e.valid_values.size());
        const auto& context = ctx_manager->get(e);

        for(const auto& valid_value : e.valid_values)
        {
            switch_cases.push_back(fmt::format(
                // clang-format off
R"(case {enum_type}::{enumerator}:
        visitor.on_enum_value(e, {tag}{{}});
        break;)",
                // clang-format on
                fmt::arg("enum_type", context.impl_name),
                fmt::arg("enumerator", valid_value.name),
                fmt::arg("tag", ctx_manager->get(valid_value).tag)));
        }

        return fmt::format(
            // clang-format off
R"(template<typename Visitor>
SBEPP_CPP14_CONSTEXPR void tag_invoke(
    ::sbepp::detail::visit_tag, {enum} e, Visitor& visitor) noexcept
{{
    switch(e)
    {{
    {switch_cases}
    default:
        visitor.on_enum_value(e, ::sbepp::unknown_enum_value_tag{{}});
    }}
}}
)",
            // clang-format on
            fmt::arg("enum", context.impl_name),
            fmt::arg("switch_cases", fmt::join(switch_cases, "\n    ")));
    }

    std::string compile_encoding(const sbe::enumeration& e)
    {
        auto& context = ctx_manager->get(e);
        context.impl_type = fmt::format(
            "::{}::detail::types::{}", schema_name, context.impl_name);
        context.public_type = context.impl_type;
        context.underlying_type =
            utils::primitive_type_to_cpp_type(context.primitive_type);

        const auto enumerators = make_enumerators(e);

        return fmt::format(
            // clang-format off
R"(
enum class {name} : {type}
{{
{enumerators}
}};

{enum_visit_impl}
)",
            // clang-format on
            fmt::arg("name", context.impl_name),
            fmt::arg("type", context.underlying_type),
            fmt::arg("enumerators", enumerators),
            fmt::arg("enum_visit_impl", make_enum_visit_impl(e)));
    }

    static bool is_unsigned(const std::string_view type)
    {
        static const std::unordered_set<std::string_view> unsigned_types{
            "::std::uint8_t",
            "::std::uint16_t",
            "::std::uint32_t",
            "::std::uint64_t"};
        return unsigned_types.count(type);
    }

    std::string make_set_accessors(const sbe::set& s) const
    {
        const auto& context = ctx_manager->get(s);
        std::string res;

        for(const auto& choice : s.choices)
        {
            res += fmt::format(
                // clang-format off
R"(
    constexpr bool {name}() const noexcept
    {{
        return (*this)(::sbepp::detail::get_bit_tag{{}}, {index});
    }}

    SBEPP_CPP14_CONSTEXPR {class_name}& {name}(const bool v) noexcept
    {{
        (*this)(::sbepp::detail::set_bit_tag{{}}, {index}, v);
        return *this;
    }}
)",
                // clang-format on
                fmt::arg("name", choice.name),
                fmt::arg("index", choice.value),
                fmt::arg("class_name", context.impl_name));
        }

        return res;
    }

    static std::string make_visit_set_impl(const sbe::set& s)
    {
        std::vector<std::string> visitors;
        visitors.reserve(s.choices.size());

        for(const auto& choice : s.choices)
        {
            visitors.push_back(fmt::format(
                "visitor(this->{choice_name}(), \"{choice_name}\");",
                fmt::arg("choice_name", choice.name)));
        }

        return fmt::format(
            // clang-format off
R"(template<typename Visitor>
SBEPP_CPP14_CONSTEXPR Visitor&& operator()(
    ::sbepp::detail::visit_set_tag, Visitor&& visitor) const noexcept
{{
    {visitors}
    return std::forward<Visitor>(visitor);
}}
)",
            // clang-format on
            fmt::arg("visitors", fmt::join(visitors, "\n    ")));
    }

    std::string make_visit_set_impl2(const sbe::set& s) const
    {
        std::vector<std::string> visitors;
        visitors.reserve(s.choices.size());

        for(const auto& choice : s.choices)
        {
            visitors.push_back(fmt::format(
                "visitor.on_set_choice(this->{choice_name}(), {tag}{{}});",
                fmt::arg("choice_name", choice.name),
                fmt::arg("tag", ctx_manager->get(choice).tag)));
        }

        return fmt::format(
            // clang-format off
R"(
template<typename Visitor>
SBEPP_CPP14_CONSTEXPR void operator()(
    ::sbepp::detail::visit_tag, Visitor& visitor) const noexcept
{{
    {visitors}
}}
)",
            // clang-format on
            fmt::arg("visitors", fmt::join(visitors, "\n    ")));
    }

    std::string compile_encoding(const sbe::set& s)
    {
        auto& context = ctx_manager->get(s);
        context.impl_type = fmt::format(
            "::{}::detail::types::{}", schema_name, context.impl_name);
        context.public_type = context.impl_type;
        context.underlying_type =
            utils::primitive_type_to_cpp_type(context.primitive_type);

        return fmt::format(
            // clang-format off
R"(
class {name} : public ::sbepp::detail::bitset_base<{type}>
{{
public:
    using ::sbepp::detail::bitset_base<{type}>::bitset_base;
    using ::sbepp::detail::bitset_base<{type}>::operator();

    {accessors}
    {visit_set_impl}
    {visit_set_impl2}
}};
)",
            // clang-format on
            fmt::arg("name", context.impl_name),
            fmt::arg("type", context.underlying_type),
            fmt::arg("accessors", make_set_accessors(s)),
            fmt::arg("visit_set_impl", make_visit_set_impl(s)),
            fmt::arg("visit_set_impl2", make_visit_set_impl2(s)));
    }

    std::string get_const_impl_type(const sbe::type& t) const
    {
        const auto& context = ctx_manager->get(t);

        if(t.length != 1)
        {
            return context.impl_type;
        }
        return context.underlying_type;
    }

    std::string value_ref_to_enum_value(const std::string_view value_ref)
    {
        const auto parsed = utils::parse_value_ref(value_ref);
        const auto& enc = utils::get_schema_encoding(*schema, parsed.enum_name);
        const auto& e = std::get<sbe::enumeration>(enc);

        compile_public_encoding(enc);

        return fmt::format(
            "::sbepp::to_underlying({}::{})",
            ctx_manager->get(e).impl_type,
            parsed.enumerator);
    }

    std::string get_const_value(const sbe::type& t)
    {
        assert(t.presence == field_presence::constant);

        assert(t.value_ref || t.constant_value);
        if(t.value_ref)
        {
            return value_ref_to_enum_value(*t.value_ref);
        }

        if(t.primitive_type == "char")
        {
            return utils::make_char_constant(
                *t.constant_value, t.length, t.location);
        }

        return utils::numeric_literal_to_value(
            *t.constant_value, t.primitive_type);
    }

    std::vector<std::string> make_children_visit_calls(
        const std::vector<sbe::composite_element>& elements) const
    {
        std::vector<std::string> res;
        using string_view_triple =
            std::tuple<std::string_view, std::string_view, std::string_view>;

        const auto types_visitor = utils::overloaded{
            [this](const sbe::type& t) -> string_view_triple
            {
                if(t.presence == field_presence::constant)
                {
                    return {};
                }

                return {"on_type", t.name, ctx_manager->get(t).tag};
            },
            [this](const sbe::enumeration& e) -> string_view_triple
            {
                return {"on_enum", e.name, ctx_manager->get(e).tag};
            },
            [this](const sbe::set& s) -> string_view_triple
            {
                return {"on_set", s.name, ctx_manager->get(s).tag};
            },
            [this](const sbe::composite& c) -> string_view_triple
            {
                return {"on_composite", c.name, ctx_manager->get(c).tag};
            }};

        for(const auto& e : elements)
        {
            const auto visit_info = std::visit(
                utils::overloaded{
                    [this,
                     &types_visitor](const sbe::ref& r) -> string_view_triple
                    {
                        const auto& enc =
                            utils::get_schema_encoding(*schema, r.type);
                        auto enc_visit_info = std::visit(types_visitor, enc);
                        return {
                            std::get<0>(enc_visit_info),
                            r.name,
                            ctx_manager->get(r).tag};
                    },
                    types_visitor},
                e);

            if(!std::get<0>(visit_info).empty())
            {
                res.push_back(fmt::format(
                    "v.{visitor}(this->{name}(), {tag}{{}})",
                    fmt::arg("visitor", std::get<0>(visit_info)),
                    fmt::arg("name", std::get<1>(visit_info)),
                    fmt::arg("tag", std::get<2>(visit_info))));
            }
        }

        return res;
    }

    std::string
        make_visit_children(const std::vector<sbe::composite_element>& elements)
    {
        std::string res;
        auto children_visit_calls = make_children_visit_calls(elements);
        if(children_visit_calls.empty())
        {
            children_visit_calls.emplace_back("false");
        }

        res += fmt::format(
            // clang-format off
R"(
    template<typename Visitor, typename Cursor>
    constexpr bool operator()(
        ::sbepp::detail::visit_children_tag, Visitor& v, Cursor&) const
    {{
        return {children_visitors};
    }}
)",
            // clang-format on
            fmt::arg(
                "children_visitors", fmt::join(children_visit_calls, "\n||")));

        return res;
    }

    std::pair<std::string, std::string>
        make_element_accessors(const sbe::composite& c)
    {
        std::string accessors;
        std::string inline_types_impl;

        for(auto& e : c.elements)
        {
            accessors += std::visit(
                utils::overloaded{
                    [this](const sbe::ref& r)
                    {
                        auto& enc = utils::get_schema_encoding(*schema, r.type);
                        compile_public_encoding(enc);

                        if(const auto t = std::get_if<sbe::type>(&enc);
                           t && (t->presence == field_presence::constant))
                        {
                            return normal_accessors::make_constant_accessor(
                                r.name,
                                get_const_impl_type(*t),
                                get_const_value(*t));
                        }

                        return std::visit(
                            [&r, this](auto& enc)
                            {
                                return normal_accessors::make_accessor(
                                    enc,
                                    ctx_manager->get(r).offset_in_composite,
                                    r.name,
                                    byte_order,
                                    ctx_manager->get(enc));
                            },
                            enc);
                    },
                    [this, &inline_types_impl](const sbe::type& t)
                    {
                        inline_types_impl += compile_encoding(t);
                        if(t.presence == field_presence::constant)
                        {
                            return normal_accessors::make_constant_accessor(
                                t.name,
                                get_const_impl_type(t),
                                get_const_value(t));
                        }

                        const auto& context = ctx_manager->get(t);
                        return normal_accessors::make_accessor(
                            t,
                            *context.offset_in_composite,
                            t.name,
                            byte_order,
                            context);
                    },
                    [this, &inline_types_impl](auto& enc)
                    {
                        inline_types_impl += compile_encoding(enc);

                        const auto& context = ctx_manager->get(enc);
                        return normal_accessors::make_accessor(
                            enc,
                            *context.offset_in_composite,
                            enc.name,
                            byte_order,
                            context);
                    }},
                e);
        }

        return {accessors, inline_types_impl};
    }

    std::string compile_encoding(const sbe::composite& c)
    {
        auto& context = ctx_manager->get(c);
        context.impl_type = fmt::format(
            "::{}::detail::types::{}", schema_name, context.impl_name);
        context.public_type = context.impl_type;

        const auto accessors = make_element_accessors(c);

        return fmt::format(
            // clang-format off
R"(
{inline_types_impl}

template<typename Byte>
class {name} : public ::sbepp::detail::composite_base<Byte>
{{
public:
    using ::sbepp::detail::composite_base<Byte>::composite_base;
    using ::sbepp::detail::composite_base<Byte>::operator();

{accessors}

    constexpr std::size_t
        operator()(::sbepp::detail::size_bytes_tag) const noexcept
    {{
        return {size};
    }}

    template<typename Visitor, typename Cursor>
    constexpr bool operator()(
        ::sbepp::detail::visit_tag, Visitor& v, Cursor&) const
    {{
        return v.on_composite(*this, {tag}{{}});
    }}

{visit_children_impl}
}};
)",
            // clang-format on
            fmt::arg("name", context.impl_name),
            fmt::arg("accessors", accessors.first),
            fmt::arg("size", context.size),
            fmt::arg("inline_types_impl", accessors.second),
            fmt::arg("visit_children_impl", make_visit_children(c.elements)),
            fmt::arg("tag", context.tag));
    }

    std::string compile_encoding(const sbe::encoding& enc)
    {
        return std::visit(
            [this](auto& enc)
            {
                return compile_encoding(enc);
            },
            enc);
    }

    std::string make_public_type(const std::string_view name)
    {
        return fmt::format("::{}::types::{}", schema_name, name);
    }

    std::string make_alias(const sbe::encoding& enc)
    {
        return std::visit(
            utils::overloaded{
                [this](const sbe::composite& c)
                {
                    auto& context = ctx_manager->get(c);
                    context.public_type = make_public_type(c.name);
                    return utils::make_alias_template(
                        c.name, context.impl_type);
                },
                [this](const sbe::type& t)
                {
                    auto& context = ctx_manager->get(t);
                    context.public_type = make_public_type(t.name);
                    if(context.is_template)
                    {
                        return utils::make_alias_template(
                            t.name, context.impl_type);
                    }
                    else
                    {
                        return utils::make_type_alias(
                            t.name, context.impl_type);
                    }
                },
                [this](const auto& e)
                {
                    auto& context = ctx_manager->get(e);
                    context.public_type = make_public_type(e.name);
                    return utils::make_type_alias(e.name, context.impl_type);
                }},
            enc);
    }

    void compile_public_encoding(const sbe::encoding& enc)
    {
        const auto name = utils::get_encoding_name(enc);
        assert(!dependencies.empty());
        dependencies.back().emplace(name);

        if(!compiled_types[name])
        {
            dependencies.emplace_back();
            const auto implementation = compile_encoding(enc);
            compiled_types[name] = true;
            const auto alias = make_alias(enc);
            const auto traits = traits_gen->make_type_traits(enc);
            on_public_type_cb(
                name, implementation, alias, dependencies.back(), traits);
            dependencies.pop_back();
        }
    }
};
} // namespace sbepp::sbeppc
