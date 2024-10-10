// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/ireporter.hpp>
#include <sbepp/sbeppc/source_location.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/build_info.hpp>

#include <fmt/core.h>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <charconv>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <optional>

namespace sbepp::sbeppc::utils
{
inline bool is_primitive_type(const std::string_view type)
{
    static const std::unordered_set<std::string_view> primitive_types{
        "char",
        "int8",
        "int16",
        "int32",
        "int64",
        "uint8",
        "uint16",
        "uint32",
        "uint64",
        "float",
        "double"};

    return primitive_types.count(type);
}

inline std::string_view primitive_type_to_cpp_type(const std::string_view type)
{
    static const std::unordered_map<std::string_view, std::string_view> map{
        {"char", "char"},
        {"int8", "::std::int8_t"},
        {"int16", "::std::int16_t"},
        {"int32", "::std::int32_t"},
        {"int64", "::std::int64_t"},
        {"uint8", "::std::uint8_t"},
        {"uint16", "::std::uint16_t"},
        {"uint32", "::std::uint32_t"},
        {"uint64", "::std::uint64_t"},
        {"float", "float"},
        {"double", "double"}};

    return map.at(type);
}

inline std::string_view primitive_type_to_wrapper_type(
    const std::string_view type, const field_presence presence)
{
    assert(presence != field_presence::constant);

    static const std::unordered_map<std::string_view, std::string_view>
        required_types{
            {"char", "::sbepp::char_t"},
            {"int8", "::sbepp::int8_t"},
            {"int16", "::sbepp::int16_t"},
            {"int32", "::sbepp::int32_t"},
            {"int64", "::sbepp::int64_t"},
            {"uint8", "::sbepp::uint8_t"},
            {"uint16", "::sbepp::uint16_t"},
            {"uint32", "::sbepp::uint32_t"},
            {"uint64", "::sbepp::uint64_t"},
            {"float", "::sbepp::float_t"},
            {"double", "::sbepp::double_t"}};

    static const std::unordered_map<std::string_view, std::string_view>
        optional_types{
            {"char", "::sbepp::char_opt_t"},
            {"int8", "::sbepp::int8_opt_t"},
            {"int16", "::sbepp::int16_opt_t"},
            {"int32", "::sbepp::int32_opt_t"},
            {"int64", "::sbepp::int64_opt_t"},
            {"uint8", "::sbepp::uint8_opt_t"},
            {"uint16", "::sbepp::uint16_opt_t"},
            {"uint32", "::sbepp::uint32_opt_t"},
            {"uint64", "::sbepp::uint64_opt_t"},
            {"float", "::sbepp::float_opt_t"},
            {"double", "::sbepp::double_opt_t"}};

    if(presence == field_presence::required)
    {
        return required_types.at(type);
    }

    return optional_types.at(type);
}

inline std::size_t get_underlying_size(const std::string_view underlying_type)
{
    static const std::unordered_map<std::string_view, std::size_t> map{
        {"char", sizeof(char)},
        {"::std::int8_t", sizeof(std::int8_t)},
        {"::std::int16_t", sizeof(std::int16_t)},
        {"::std::int32_t", sizeof(std::int32_t)},
        {"::std::int64_t", sizeof(std::int64_t)},
        {"::std::uint8_t", sizeof(std::uint8_t)},
        {"::std::uint16_t", sizeof(std::uint16_t)},
        {"::std::uint32_t", sizeof(std::uint32_t)},
        {"::std::uint64_t", sizeof(std::uint64_t)},
        {"float", sizeof(float)},
        {"double", sizeof(double)}};

    return map.at(underlying_type);
}

template<typename T>
std::string_view get_encoding_name(const T& encoding)
{
    return std::visit(
        [](const auto& encoding) -> std::string_view
        {
            return encoding.name;
        },
        encoding);
}

template<typename Variant>
const source_location& get_location(const Variant& variant)
{
    return std::visit(
        [](const auto& variant) -> const source_location&
        {
            return variant.location;
        },
        variant);
}

template<typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

inline offset_t get_valid_offset(
    const std::optional<offset_t>& offset,
    const offset_t min_offset,
    const source_location& location)
{
    if(offset)
    {
        if(*offset >= min_offset)
        {
            return *offset;
        }
        throw_error(
            "{}: custom offset ({}) is less than minimal ({})",
            location,
            *offset,
            min_offset);
    }
    return min_offset;
}

inline std::string_view byte_order_to_endian(const sbe::byte_order_kind order)
{
    if(order == sbe::byte_order_kind::big_endian)
    {
        return "::sbepp::endian::big";
    }
    return "::sbepp::endian::little";
}

inline std::string_view presence_to_string(const field_presence presence)
{
    if(presence == field_presence::constant)
    {
        return "::sbepp::field_presence::constant";
    }
    else if(presence == field_presence::optional)
    {
        return "::sbepp::field_presence::optional";
    }
    else
    {
        return "::sbepp::field_presence::required";
    }
}

inline std::string to_lower(const std::string_view str)
{
    std::string res;
    res.reserve(str.size());
    std::transform(
        std::begin(str),
        std::end(str),
        std::back_inserter(res),
        [](const auto ch)
        {
            return std::tolower(ch);
        });
    return res;
}

template<typename T>
std::optional<T> string_to_number(const std::string_view str)
{
    if(!str.empty())
    {
        T value{};
        const auto str_end = str.data() + str.size();
        auto res = std::from_chars(str.data(), str_end, value);
        if((res.ec == std::errc{}) && (res.ptr == str_end))
        {
            return value;
        }
    }

    return {};
}

inline std::string to_integer_literal(
    const std::string_view value, const std::string_view type)
{
    assert(!value.empty() && (type != "float") && (type != "double"));

    if((type == "int64") && (value[0] == '-'))
    {
        const auto v = string_to_number<std::int64_t>(value);
        assert(v);
        static constexpr auto min_signed_literal = -9223372036854775807;
        if(*v < min_signed_literal)
        {
            // the difference is going to be negative, no need for a sign in the
            // format string
            return fmt::format(
                "{} {}", min_signed_literal, (*v - min_signed_literal));
        }
    }
    else if(type == "uint64")
    {
        // this part is not strictly required since type will be deduced
        // correctly but most compilers produce warning when literal
        // representing large number is used without `UL` prefix
        const auto v = string_to_number<std::uint64_t>(value);
        assert(v);
        static constexpr auto max_signed_literal = 9223372036854775807;
        if(*v > max_signed_literal)
        {
            return fmt::format("{}UL", value);
        }
    }

    return std::string{value};
}

inline std::string get_compiled_header_top_comment()
{
    static std::string str = fmt::format(
        // clang-format off
R"(// SPDX-License-Identifier: MIT
// Copyright (c) 2024, Oleksandr Koval

// This file was generated by sbeppc {})",
        // clang-format on
        build_info::get_version());

    return str;
}

struct parsed_value_ref
{
    std::string_view enum_name;
    std::string_view enumerator;
};

inline parsed_value_ref parse_value_ref(const std::string_view value_ref)
{
    const auto dot_pos = value_ref.find('.');
    if(dot_pos == std::string_view::npos)
    {
        return {};
    }

    parsed_value_ref res{};
    res.enum_name = value_ref.substr(0, dot_pos);
    res.enumerator = value_ref.substr(dot_pos + 1);

    return res;
}

inline std::string
    make_type_alias(const std::string_view alias, const std::string_view type)
{
    return fmt::format("using {} = {};", alias, type);
}

inline std::string make_alias_template(
    const std::string_view alias, const std::string_view type)
{
    return fmt::format(
        // clang-format off
R"(
    template<typename Byte>
    using {} = {}<Byte>;
)",
        // clang-format on
        alias,
        type);
}

inline std::string make_string_constant(
    const std::string& const_value,
    const length_t type_length,
    const source_location& location)
{
    if(type_length < const_value.size())
    {
        throw_error("{}: constant value doesn't fit into `length`", location);
    }

    std::string value;
    value.append("\"").append(const_value);
    // add padding if necessary
    const auto padding_length = type_length - const_value.size();
    for(std::size_t i = 0; i != padding_length; i++)
    {
        value.append("\\0");
    }
    value.append("\"");

    // string constants are represented using `static_array_ref`
    // which requires pointer and size to be constructed
    return fmt::format("{}, {}", value, const_value.size() + padding_length);
}

inline std::string make_char_constant(
    const std::string& constant_value,
    const length_t type_length,
    const source_location& location)
{
    if(constant_value.size() > 1)
    {
        return utils::make_string_constant(
            constant_value, type_length, location);
    }

    return fmt::format("'{}'", constant_value);
}

inline std::string numeric_literal_to_value(
    const std::string_view value, const std::string_view type)
{
    assert(!value.empty());

    if((type == "float") || (type == "double"))
    {
        if(value == "NaN")
        {
            return fmt::format("::std::numeric_limits<{}>::quiet_NaN()", type);
        }
        else if((value == "INF") || (value == "+INF"))
        {
            return fmt::format("::std::numeric_limits<{}>::infinity()", type);
        }
        else if(value == "-INF")
        {
            return fmt::format("-::std::numeric_limits<{}>::infinity()", type);
        }

        return std::string{value};
    }

    return utils::to_integer_literal(value, type);
}

inline const sbe::composite_element*
    find_composite_element(const sbe::composite& c, const std::string_view name)
{
    const auto search = std::find_if(
        std::begin(c.elements),
        std::end(c.elements),
        [name](const auto& element)
        {
            return utils::get_encoding_name(element) == name;
        });

    if(search != std::end(c.elements))
    {
        return &*search;
    }

    return {};
}

// returns existing encoding using case-insensitive search
inline const sbe::encoding& get_schema_encoding(
    const sbe::message_schema& schema, std::string_view name)
{
    const auto lowered_name = utils::to_lower(name);
    return schema.types.at(lowered_name);
}

template<typename T>
const T& get_schema_encoding_as(
    const sbe::message_schema& schema, std::string_view name)
{
    return std::get<T>(get_schema_encoding(schema, name));
}
} // namespace sbepp::sbeppc::utils
