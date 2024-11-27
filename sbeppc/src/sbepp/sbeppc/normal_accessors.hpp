// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/context_manager.hpp>

#include <fmt/core.h>

#include <string>
#include <string_view>

namespace sbepp::sbeppc
{
class normal_accessors
{
public:
    static std::string make_constant_accessor(
        const std::string_view name,
        const std::string_view type,
        const std::string_view value)
    {
        return fmt::format(
            // clang-format off
R"(    static constexpr {type} {name}() noexcept
    {{
        return {type}{{{value}}};
    }}
)",
            // clang-format on
            fmt::arg("name", name),
            fmt::arg("type", type),
            fmt::arg("value", value));
    }

    static std::string make_accessor(
        const sbe::type& t,
        const offset_t offset,
        const std::string_view name,
        const sbe::byte_order_kind byte_order,
        const type_context& context)
    {
        if(t.length == 1)
        {
            return make_type_accessor(
                name, context.public_type, offset, byte_order);
        }
        return make_array_accessor(name, context.public_type, offset);
    }

    static std::string make_type_accessor(
        const std::string_view name,
        const std::string_view type,
        const offset_t offset,
        const sbe::byte_order_kind byte_order)
    {
        return fmt::format(
            // clang-format off
R"(    SBEPP_CPP20_CONSTEXPR {type} {name}() const noexcept
    {{
        return ::sbepp::detail::get_value<
            {type}, {type}::value_type, {endian}>(*this, {offset});
    }}

    template<
        typename T = void,
        typename = ::sbepp::detail::enable_if_writable_t<Byte, T>>
    SBEPP_CPP20_CONSTEXPR void {name}({type} v) const noexcept
    {{
        ::sbepp::detail::set_value<{endian}>(*this, {offset}, v.value());
    }}
)",
            // clang-format on
            fmt::arg("type", type),
            fmt::arg("name", name),
            fmt::arg("offset", offset),
            fmt::arg("endian", utils::byte_order_to_endian(byte_order)));
    }

    static std::string make_accessor(
        const sbe::enumeration&,
        const offset_t offset,
        const std::string_view name,
        const sbe::byte_order_kind byte_order,
        const enumeration_context& context)
    {
        return fmt::format(
            // clang-format off
R"(    SBEPP_CPP20_CONSTEXPR {type} {name}() const noexcept
    {{
        return ::sbepp::detail::get_value<
            {type}, {type}, {endian}>(*this, {offset});
    }}

    template<
        typename T = void,
        typename = ::sbepp::detail::enable_if_writable_t<Byte, T>>
    SBEPP_CPP20_CONSTEXPR void {name}({type} v) const noexcept
    {{
        ::sbepp::detail::set_value<{endian}>(*this, {offset}, v);
    }}
)",
            // clang-format on
            fmt::arg("type", context.public_type),
            fmt::arg("name", name),
            fmt::arg("offset", offset),
            fmt::arg("endian", utils::byte_order_to_endian(byte_order)));
    }

    static std::string make_accessor(
        const sbe::set&,
        const offset_t offset,
        const std::string_view name,
        const sbe::byte_order_kind byte_order,
        const set_context& context)
    {
        return fmt::format(
            // clang-format off
R"(    SBEPP_CPP20_CONSTEXPR {type} {name}() const noexcept
    {{
        return ::sbepp::detail::get_value<
            {type}, {underlying_type}, {endian}>(*this, {offset});
    }}

    template<
        typename T = void,
        typename = ::sbepp::detail::enable_if_writable_t<Byte, T>>
    SBEPP_CPP20_CONSTEXPR void {name}({type} v) const noexcept
    {{
        ::sbepp::detail::set_value<{endian}>(*this, {offset}, *v);
    }}
)",
            // clang-format on
            fmt::arg("type", context.public_type),
            fmt::arg("name", name),
            fmt::arg("offset", offset),
            fmt::arg("underlying_type", context.underlying_type),
            fmt::arg("endian", utils::byte_order_to_endian(byte_order)));
    }

    static std::string make_accessor(
        const sbe::composite&,
        const offset_t offset,
        const std::string_view name,
        // to make all signatures equivalent, useful with `std::visit`
        const sbe::byte_order_kind,
        const composite_context& context)
    {
        return fmt::format(
            // clang-format off
R"(    constexpr {type}<Byte> {name}() const noexcept
    {{
        return ::sbepp::detail::get_static_field_view<{type}<Byte>>(
            *this, {offset});
    }}
)",
            // clang-format on
            fmt::arg("type", context.public_type),
            fmt::arg("name", name),
            fmt::arg("offset", offset));
    }

private:
    static std::string make_array_accessor(
        const std::string_view name,
        const std::string_view type,
        const offset_t offset)
    {
        return fmt::format(
            // clang-format off
R"(    constexpr {type}<Byte> {name}() const noexcept
    {{
        return ::sbepp::detail::get_static_field_view<{type}<Byte>>(
            *this, {offset});
    }}
)",
            // clang-format on
            fmt::arg("type", type),
            fmt::arg("name", name),
            fmt::arg("offset", offset));
    }
};
} // namespace sbepp::sbeppc
