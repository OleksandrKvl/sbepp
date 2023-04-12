// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/source_location.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/type_manager.hpp>
#include <sbepp/sbeppc/traits_generator.hpp>
#include <sbepp/sbeppc/normal_accessors.hpp>

#include <fmt/core.h>

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
        const std::string_view schema_name,
        const sbe::byte_order_kind byte_order,
        type_manager& types,
        traits_generator& traits_gen)
        : schema_name{schema_name},
          byte_order{byte_order},
          types{&types},
          traits_gen{&traits_gen}
    {
    }

    void compile(on_public_type_cb_t cb)
    {
        on_public_type_cb = std::move(cb);

        types->for_each(
            [this](auto& enc)
            {
                dependencies.emplace_back();
                compile_public_encoding(enc);
                dependencies.pop_back();
            });
    }

private:
    std::string_view schema_name;
    sbe::byte_order_kind byte_order;
    type_manager* types;
    traits_generator* traits_gen;
    on_public_type_cb_t on_public_type_cb;
    std::unordered_map<std::string_view, bool> compiled;
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

        return utils::numeric_literal_to_value(
                   t.min_value, t.primitive_type, t.location)
            .value_or(built_in_min_values.at(t.primitive_type));
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

        return utils::numeric_literal_to_value(
                   t.max_value, t.primitive_type, t.location)
            .value_or(built_in_max_values.at(t.primitive_type));
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

        return utils::numeric_literal_to_value(
                   t.null_value, t.primitive_type, t.location)
            .value_or(built_in_null_values.at(t.primitive_type));
    }

    static std::string make_constant_type(const sbe::type& t)
    {
        assert(t.presence == field_presence::constant);

        if(t.length != 1)
        {
            if(t.underlying_type != "char")
            {
                throw_error(
                    "{}: array constants must have type `char`", t.location);
            }

            return fmt::format(
                // clang-format off
R"(
using {type_name} = ::sbepp::detail::static_array_ref<const char, {element_type}, {length}>;
)",
                // clang-format on
                fmt::arg("type_name", t.impl_name),
                fmt::arg("element_type", t.underlying_type),
                fmt::arg("length", t.length));
        }
        else
        {
            return fmt::format(
                "using {} = {};\n", t.impl_name, t.underlying_type);
        }
    }

    static std::string make_array_type(sbe::type& t)
    {
        assert((t.length != 1) && (t.presence != field_presence::constant));

        return fmt::format(
            // clang-format off
R"(
template<typename Byte>
using {type_name} = ::sbepp::detail::static_array_ref<
    Byte, {element_type}, {length}>;
)",
            // clang-format on
            fmt::arg("type_name", t.impl_name),
            fmt::arg("element_type", t.underlying_type),
            fmt::arg("length", t.length));
    }

    static std::string make_required_type(const sbe::type& t)
    {
        assert((t.presence == field_presence::required) && (t.length == 1));

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
            fmt::arg("type_name", t.impl_name),
            fmt::arg("underlying_type", t.underlying_type),
            fmt::arg("min_value", get_min_value(t)),
            fmt::arg("max_value", get_max_value(t)));
    }

    static std::string make_optional_type(const sbe::type& t)
    {
        assert((t.presence == field_presence::optional) && (t.length == 1));

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
            fmt::arg("type_name", t.impl_name),
            fmt::arg("underlying_type", t.underlying_type),
            fmt::arg("min_value", get_min_value(t)),
            fmt::arg("max_value", get_max_value(t)),
            fmt::arg("null_value", get_null_value(t)));
    }

    std::string compile_encoding(sbe::type& t) const
    {
        t.impl_type =
            fmt::format("::{}::detail::types::{}", schema_name, t.impl_name);
        t.public_type = t.impl_type;
        t.is_template = false;
        t.underlying_type = utils::primitive_type_to_cpp_type(t.primitive_type);
        t.size = utils::get_underlying_size(t.underlying_type) * t.length;

        if(t.presence == field_presence::constant)
        {
            return make_constant_type(t);
        }
        else if(t.length != 1)
        {
            t.is_template = true;
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

    template<typename EnumOrSet>
    std::string_view get_underlying_type(const EnumOrSet& enc) const
    {
        if(utils::is_primitive_type(enc.type))
        {
            return utils::primitive_type_to_cpp_type(enc.type);
        }

        return utils::primitive_type_to_cpp_type(
            types
                ->get_as_or_throw<sbe::type>(
                    enc.type,
                    "{}: encoding `{}` doesn't exist or it's not a type",
                    enc.location,
                    enc.type)
                .primitive_type);
    }

    static std::string make_enumerators(const sbe::enumeration& e)
    {
        std::vector<std::string> enumerators;
        enumerators.reserve(e.valid_values.size());

        const auto is_char_type = (e.underlying_type == "char");
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
                        valid_value.value, valid_value.location)));
            }
        }

        return fmt::format("{}", fmt::join(enumerators, ",\n"));
    }

    static std::string make_enum_to_string(sbe::enumeration& e)
    {
        std::string switch_cases;
        for(const auto& valid_value : e.valid_values)
        {
            switch_cases.append(fmt::format(
                // clang-format off
R"(
    case {enum_type}::{enumerator}:
        return "{enumerator}";
)",
                // clang-format on
                fmt::arg("enum_type", e.impl_name),
                fmt::arg("enum_name", e.name),
                fmt::arg("enumerator", valid_value.name)));
        }

        return fmt::format(
            // clang-format off
R"(
inline SBEPP_CPP14_CONSTEXPR const char*
    tag_invoke(
        ::sbepp::detail::enum_to_str_tag,
        {enum} e) noexcept
{{
    switch(e)
    {{
    {switch_cases}
    default:
        return nullptr;
    }}
}}
)",
            // clang-format on
            fmt::arg("enum", e.impl_name),
            fmt::arg("switch_cases", switch_cases));
    }

    std::string compile_encoding(sbe::enumeration& e)
    {
        e.impl_type =
            fmt::format("::{}::detail::types::{}", schema_name, e.impl_name);
        e.public_type = e.impl_type;
        e.underlying_type = get_underlying_type(e);
        e.size = utils::get_underlying_size(e.underlying_type);

        const auto enumerators = make_enumerators(e);

        return fmt::format(
            // clang-format off
R"(
enum class {name} : {type}
{{
{enumerators}
}};

{enum_to_string_impl}
)",
            // clang-format on
            fmt::arg("name", e.impl_name),
            fmt::arg("type", e.underlying_type),
            fmt::arg("enumerators", enumerators),
            fmt::arg("enum_to_string_impl", make_enum_to_string(e)));
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

    static std::string make_set_accessors(const sbe::set& s)
    {
        std::string res;
        static constexpr auto bits_per_byte = 8;
        const auto bit_length = s.size * bits_per_byte - 1;

        for(const auto& choice : s.choices)
        {
            if(choice.value > bit_length)
            {
                throw_error(
                    "{}: choice value is out of range", choice.location);
            }

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
                fmt::arg("class_name", s.impl_name));
        }

        return res;
    }

    static std::string make_visit_set_impl(sbe::set& s)
    {
        std::string choice_visitors;
        for(const auto& choice : s.choices)
        {
            choice_visitors.append(fmt::format(
                // clang-format off
R"(
    visitor(this->{choice_name}(), "{choice_name}");)",
                // clang-format on
                fmt::arg("set_name", s.name),
                fmt::arg("choice_name", choice.name)));
        }

        return fmt::format(
            // clang-format off
R"(
template<typename Visitor>
SBEPP_CPP14_CONSTEXPR Visitor&& operator()(
    ::sbepp::detail::visit_set_tag, Visitor&& visitor) const noexcept
{{
    {choice_visitors}
    return std::forward<Visitor>(visitor);
}}
)",
            // clang-format on
            fmt::arg("choice_visitors", choice_visitors));
    }

    std::string compile_encoding(sbe::set& s)
    {
        s.impl_type =
            fmt::format("::{}::detail::types::{}", schema_name, s.impl_name);
        s.public_type = s.impl_type;

        s.underlying_type = get_underlying_type(s);
        s.size = utils::get_underlying_size(s.underlying_type);
        if(!is_unsigned(s.underlying_type))
        {
            throw_error("{}: underlying types must be unsigned", s.location);
        }

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
}};
)",
            // clang-format on
            fmt::arg("name", s.impl_name),
            fmt::arg("type", s.underlying_type),
            fmt::arg("accessors", make_set_accessors(s)),
            fmt::arg("visit_set_impl", make_visit_set_impl(s)));
    }

    static std::string get_const_impl_type(const sbe::encoding& enc)
    {
        const auto& t = std::get<sbe::type>(enc);
        if(t.length != 1)
        {
            return t.impl_type;
        }
        return t.underlying_type;
    }

    std::string value_ref_to_enum_value(
        const std::string_view value_ref, const source_location& location)
    {
        const auto parsed = utils::parse_value_ref(value_ref);
        if(parsed.enum_name.empty() || parsed.enumerator.empty())
        {
            throw_error(
                "{}: `{}` is not a valid valueRef", location, value_ref);
        }

        auto& enc = types->get_or_throw(
            parsed.enum_name,
            "{}: no type named `{}`",
            location,
            parsed.enum_name);
        auto enum_enc = std::get_if<sbe::enumeration>(&enc);
        if(!enum_enc)
        {
            throw_error(
                "{}: `{}` is not an enumeration", location, parsed.enum_name);
        }

        compile_public_encoding(enc);

        return fmt::format(
            "::sbepp::to_underlying({}::{})",
            enum_enc->impl_type,
            parsed.enumerator);
    }

    std::string get_const_value(const sbe::encoding& enc)
    {
        assert(utils::is_constant(enc));
        auto& t = std::get<sbe::type>(enc);

        assert(t.value_ref || t.constant_value);
        if(t.value_ref)
        {
            return value_ref_to_enum_value(*t.value_ref, t.location);
        }

        if(t.primitive_type == "char")
        {
            return utils::make_char_constant(
                *t.constant_value, t.length, t.location);
        }

        return *utils::numeric_literal_to_value(
            t.constant_value, t.primitive_type, t.location);
    }

    std::vector<std::string> make_children_visit_calls(
        const std::vector<sbe::composite_element>& elements) const
    {
        std::vector<std::string> res;
        using string_view_triple =
            std::tuple<std::string_view, std::string_view, std::string_view>;

        const auto types_visitor = utils::overloaded{
            [](const sbe::type& t) -> string_view_triple
            {
                if(t.presence == field_presence::constant)
                {
                    return {};
                }

                return {"on_type", t.name, t.tag};
            },
            [](const sbe::enumeration& e) -> string_view_triple
            {
                return {"on_enum", e.name, e.tag};
            },
            [](const sbe::set& s) -> string_view_triple
            {
                return {"on_set", s.name, s.tag};
            },
            [](const sbe::composite& c) -> string_view_triple
            {
                return {"on_composite", c.name, c.tag};
            }};

        for(const auto& e : elements)
        {
            const auto visit_info = std::visit(
                utils::overloaded{
                    [this,
                     &types_visitor](const sbe::ref& r) -> string_view_triple
                    {
                        const auto& enc = types->get_or_throw(
                            r.type,
                            "{}: ref `{}` refers to an unknown type `{}`",
                            r.location,
                            r.name,
                            r.type);

                        auto enc_visit_info = std::visit(types_visitor, enc);
                        return {std::get<0>(enc_visit_info), r.name, r.tag};
                    },
                    types_visitor},
                e);

            if(!std::get<0>(visit_info).empty())
            {
                res.push_back(fmt::format(
                    "v.template {visitor}(this->{name}(), {tag}{{}})",
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
    SBEPP_CPP14_CONSTEXPR bool operator()(
        ::sbepp::detail::visit_children_tag, Visitor& v, Cursor&)
    {{
        (void)v;
        return {children_visitors};
    }}
)",
            // clang-format on
            fmt::arg(
                "children_visitors", fmt::join(children_visit_calls, "\n||")));

        return res;
    }

    std::pair<std::string, std::string>
        make_element_accessors(sbe::composite& c)
    {
        offset_t offset{};
        std::string accessors;
        std::string inline_types_impl;

        for(auto& e : c.elements)
        {
            accessors += std::visit(
                utils::overloaded{
                    [this, &offset](sbe::ref& r)
                    {
                        auto& enc = types->get_or_throw(
                            r.type,
                            "{}: `ref` `{}` refers to an unknown type `{}`",
                            r.location,
                            r.name,
                            r.type);

                        compile_public_encoding(enc);

                        if(utils::is_constant(enc))
                        {
                            return normal_accessors::make_constant_accessor(
                                r.name,
                                get_const_impl_type(enc),
                                get_const_value(enc));
                        }

                        // if ref has offset - use it
                        offset = utils::get_valid_offset(
                            r.offset, offset, r.location);
                        r.actual_offset = offset;

                        return std::visit(
                            [&offset, &r, this](auto& enc)
                            {
                                auto accessor = normal_accessors::make_accessor(
                                    enc, offset, r.name, byte_order, false);
                                offset += enc.size;
                                return accessor;
                            },
                            enc);
                    },
                    [this, &offset, &inline_types_impl](auto& enc)
                    {
                        inline_types_impl += compile_encoding(enc);
                        if(utils::is_constant(enc))
                        {
                            return normal_accessors::make_constant_accessor(
                                enc.name,
                                get_const_impl_type(enc),
                                get_const_value(enc));
                        }
                        offset = utils::get_valid_offset(
                            enc.offset, offset, enc.location);
                        enc.actual_offset = offset;
                        auto accessor = normal_accessors::make_accessor(
                            enc, offset, enc.name, byte_order, false);
                        offset += enc.size;
                        return accessor;
                    }},
                e);
        }
        c.size = offset;

        return {accessors, inline_types_impl};
    }

    std::string compile_encoding(sbe::composite& c)
    {
        c.impl_type =
            fmt::format("::{}::detail::types::{}", schema_name, c.impl_name);
        c.public_type = c.impl_type;

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

    SBEPP_CPP20_CONSTEXPR std::size_t
        operator()(::sbepp::detail::size_bytes_tag) const noexcept
    {{
        return {size};
    }}

    template<typename Visitor, typename Cursor>
    SBEPP_CPP14_CONSTEXPR bool operator()(
        ::sbepp::detail::visit_tag, Visitor& v, Cursor&)
    {{
        return v.template on_composite(*this, {tag}{{}});
    }}

{visit_children_impl}
}};
)",
            // clang-format on
            fmt::arg("name", c.impl_name),
            fmt::arg("accessors", accessors.first),
            fmt::arg("size", c.size),
            fmt::arg("inline_types_impl", accessors.second),
            fmt::arg("visit_children_impl", make_visit_children(c.elements)),
            fmt::arg("tag", c.tag));
    }

    std::string compile_encoding(sbe::encoding& enc)
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

    std::string make_alias(sbe::encoding& enc)
    {
        return std::visit(
            utils::overloaded{
                [this](sbe::composite& c)
                {
                    c.public_type = make_public_type(c.name);
                    return utils::make_alias_template(c.name, c.impl_type);
                },
                [this](sbe::type& t)
                {
                    t.public_type = make_public_type(t.name);
                    if(t.is_template)
                    {
                        return utils::make_alias_template(t.name, t.impl_type);
                    }
                    else
                    {
                        return utils::make_type_alias(t.name, t.impl_type);
                    }
                },
                [this](auto& e)
                {
                    e.public_type = make_public_type(e.name);
                    return utils::make_type_alias(e.name, e.impl_type);
                }},
            enc);
    }

    void compile_public_encoding(sbe::encoding& enc)
    {
        const auto name = utils::get_encoding_name(enc);
        assert(!dependencies.empty());
        dependencies.back().emplace(name);

        if(!compiled.count(name))
        {
            compiled[name] = false;
            dependencies.emplace_back();
            const auto implementation = compile_encoding(enc);
            compiled[name] = true;
            const auto alias = make_alias(enc);
            const auto traits = traits_gen->make_type_traits(enc);
            on_public_type_cb(
                name, implementation, alias, dependencies.back(), traits);
            dependencies.pop_back();
        }
        else if(!compiled[name])
        {
            throw_error(
                "{}: cyclic reference detected while processing encoding `{}`",
                utils::get_location(enc),
                utils::get_encoding_name(enc));
        }
    }
};
} // namespace sbepp::sbeppc
