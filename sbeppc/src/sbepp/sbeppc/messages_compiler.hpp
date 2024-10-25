// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/type_manager.hpp>
#include <sbepp/sbeppc/message_manager.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/traits_generator.hpp>
#include <sbepp/sbeppc/normal_accessors.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <unordered_set>
#include <variant>
#include <cassert>

namespace sbepp::sbeppc
{
class messages_compiler
{
public:
    using on_message_cb_t = std::function<void(
        const std::string_view name,
        const std::string_view implementation,
        const std::string_view alias,
        const std::unordered_set<std::string>& dependencies,
        const std::string_view traits)>;

    messages_compiler(
        const sbe::message_schema& schema,
        const type_manager& types,
        message_manager& messages,
        traits_generator& traits_gen)
        : schema{&schema},
          types{&types},
          messages{&messages},
          traits_gen{&traits_gen}
    {
    }

    void compile(on_message_cb_t cb)
    {
        on_message_cb = std::move(cb);

        messages->for_each(
            [this](auto& m)
            {
                compile_message(m);
            });
    }

private:
    const sbe::message_schema* schema;
    const type_manager* types;
    message_manager* messages;
    traits_generator* traits_gen;
    on_message_cb_t on_message_cb;
    std::size_t group_entry_index{};
    std::unordered_set<std::string> dependencies;

    bool is_const_field(const sbe::field& f)
    {
        if(!utils::is_primitive_type(f.type))
        {
            const auto& enc = types->get_or_throw(
                f.type, "{}: encoding `{}` doesn't exist", f.location, f.type);
            if(auto t = std::get_if<sbe::type>(&enc))
            {
                // inherits presence from user-provided type
                return (t->presence == field_presence::constant);
            }
            else if(std::holds_alternative<sbe::enumeration>(enc))
            {
                assert(
                    (f.presence != field_presence::constant)
                    || f.value_ref.has_value());
                return (f.presence == field_presence::constant);
            }
        }
        else if(f.presence == field_presence::constant)
        {
            assert(f.value_ref.has_value());
            return true;
        }

        return false;
    }

    std::string value_ref_to_enumerator(
        const std::string_view value_ref, const source_location& location) const
    {
        const auto parsed = utils::parse_value_ref(value_ref);
        if(parsed.enum_name.empty() || parsed.enumerator.empty())
        {
            throw_error(
                "{}: `{}` is not a valid valueRef", location, value_ref);
        }

        const auto& e = types->get_as_or_throw<sbe::enumeration>(
            parsed.enum_name,
            "{}: encoding `{}` doesn't exist or it's not an enum",
            location,
            parsed.enum_name);

        return fmt::format("{}::{}", e.impl_type, parsed.enumerator);
    }

    std::string value_ref_to_enum_value(
        const std::string_view value_ref, const source_location& location) const
    {
        return fmt::format(
            "::sbepp::to_underlying({})",
            value_ref_to_enumerator(value_ref, location));
    }

    std::string get_const_value(const sbe::type& t)
    {
        // the only difference from `types_compiler::get_const_value` is that
        // `value_ref_to_enum_value` never compiles enum type
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

    static std::string get_const_type(const sbe::type& t)
    {
        if(t.length != 1)
        {
            return t.public_type;
        }
        return t.underlying_type;
    }

    static void throw_if_has_no_value_ref(const sbe::field& f)
    {
        if(!f.value_ref)
        {
            throw_error("{}: required `valueRef` doesn't exist", f.location);
        }
    }

    std::string make_const_field_accessor(sbe::field& f)
    {
        if(!utils::is_primitive_type(f.type))
        {
            const auto& enc = types->get_or_throw(
                f.type, "{}: encoding `{}` doesn't exist", f.location, f.type);
            dependencies.emplace(utils::get_encoding_name(enc));
            if(auto t = std::get_if<sbe::type>(&enc))
            {
                f.value_type = get_const_type(*t);
                return normal_accessors::make_constant_accessor(
                    f.name, f.value_type, get_const_value(*t));
            }
            else if(auto e = std::get_if<sbe::enumeration>(&enc))
            {
                throw_if_has_no_value_ref(f);
                f.value_type = e->public_type;
                return normal_accessors::make_constant_accessor(
                    f.name,
                    f.value_type,
                    value_ref_to_enumerator(*f.value_ref, f.location));
            }

            throw_error(
                "{}: only types and enums can represent field constants",
                f.location);
        }

        throw_if_has_no_value_ref(f);
        f.value_type = utils::primitive_type_to_cpp_type(f.type);

        return normal_accessors::make_constant_accessor(
            f.name,
            f.value_type,
            value_ref_to_enum_value(*f.value_ref, f.location));
    }

    static field_presence get_actual_presence(
        const sbe::encoding& enc, field_presence schema_presence)
    {
        return std::visit(
            utils::overloaded{
                [](const sbe::type& t)
                {
                    return t.presence;
                },
                [schema_presence](const sbe::composite&)
                {
                    // optional composite don't have a special meaning in
                    // `sbepp` but we still should return correct value
                    return schema_presence;
                },
                [](const auto& /*enum or set*/)
                {
                    return field_presence::required;
                }},
            enc);
    }

    std::string make_field_accessors(
        std::vector<sbe::field>& fields, const std::size_t header_size)
    {
        std::string res;
        offset_t offset{};

        for(auto& f : fields)
        {
            f.is_template = false;
            if(is_const_field(f))
            {
                f.actual_presence = field_presence::constant;
                res += make_const_field_accessor(f);
                continue;
            }

            offset = utils::get_valid_offset(f.offset, offset, f.location);
            f.actual_offset = offset;
            if(utils::is_primitive_type(f.type))
            {
                f.actual_presence = f.presence;
                f.value_type =
                    utils::primitive_type_to_wrapper_type(f.type, f.presence);
                f.value_type_tag = f.value_type;
                f.size = utils::get_underlying_size(
                    utils::primitive_type_to_cpp_type(f.type));
                res += normal_accessors::make_type_accessor(
                    f.name,
                    f.value_type,
                    offset + header_size,
                    schema->byte_order);
            }
            else
            {
                const auto& enc = types->get_or_throw(
                    f.type, "{}: type `{}` doesn't exist", f.location, f.type);
                f.actual_presence = get_actual_presence(enc, f.presence);
                if(const auto t = std::get_if<sbe::type>(&enc);
                   (t && t->length != 1)
                   || std::holds_alternative<sbe::composite>(enc))
                {
                    f.is_template = true;
                }

                res += std::visit(
                    [&offset, &f, this, header_size](const auto& enc)
                    {
                        f.value_type = enc.public_type;
                        f.value_type_tag = enc.tag;
                        f.size = enc.size;
                        dependencies.emplace(enc.name);
                        return normal_accessors::make_accessor(
                            enc,
                            offset + header_size,
                            f.name,
                            schema->byte_order,
                            true);
                    },
                    enc);
            }
            offset += f.size;
        }

        return res;
    }

    std::string make_next_group_entry_name(const sbe::level_members& members)
    {
        group_entry_index++;
        auto name = fmt::format("entry_{}", group_entry_index);

        const auto member_names = get_member_names(members);
        std::size_t minor_index{};
        while(member_names.count(name))
        {
            minor_index++;
            name = fmt::format("entry_{}_{}", group_entry_index, minor_index);
        }

        return name;
    }

    static std::string make_level_size_bytes_impl(
        const bool is_flat,
        const std::string_view last_member,
        const std::size_t header_size)
    {
        if(is_flat)
        {
            return fmt::format(
                // clang-format off
R"(
    SBEPP_CPP20_CONSTEXPR std::size_t operator()(
        ::sbepp::detail::size_bytes_tag) const noexcept
    {{
        return {header_size} + (*this)(::sbepp::detail::get_block_length_tag{{}});
    }}
)",
                // clang-format on
                fmt::arg("header_size", header_size));
        }

        return fmt::format(
            // clang-format off
R"(
    SBEPP_CPP20_CONSTEXPR std::size_t operator()(
        ::sbepp::detail::size_bytes_tag) const noexcept
    {{
        const auto last = {last_member}();
        return ::sbepp::addressof(last) + ::sbepp::size_bytes(last)
               - (*this)(::sbepp::detail::addressof_tag{{}});
    }}
)",
            // clang-format on
            fmt::arg("last_member", last_member));
    }

    static std::string_view get_last_member(const sbe::level_members& members)
    {
        if(!members.data.empty())
        {
            return members.data.back().name;
        }
        else if(!members.groups.empty())
        {
            return members.groups.back().name;
        }
        else if(!members.fields.empty())
        {
            return members.fields.back().name;
        }
        return {};
    }

    static std::string_view get_block_length_type(const sbe::composite& c)
    {
        const auto t = std::get_if<sbe::type>(
            utils::find_composite_element(c, "blockLength"));
        if(t)
        {
            return t->underlying_type;
        }

        throw_error(
            "{}: `blockLength` is not found or it's not a type", c.location);
    }

    static std::string make_entry_cursor_constructor(
        const sbe::level_members& members,
        const std::string_view class_name,
        const std::string_view block_length_type,
        const std::string_view base_class)
    {
        // for empty group entries we generate a special cursor constructor to
        // advance cursor to `block_length` because there are no other fields
        // to do this. Default constructor is declared explicitly because old
        // compilers don't support inheriting it from the base class.
        if(members.fields.empty() && members.groups.empty()
           && members.data.empty())
        {
            return fmt::format(
                // clang-format off
R"(
    {class_name}() = default;

    template<typename Byte2,
        typename = ::sbepp::detail::enable_if_convertible_t<Byte2, Byte>>
    SBEPP_CPP14_CONSTEXPR {class_name}(
        ::sbepp::cursor<Byte2>& c,
        Byte* end_ptr,
        {block_length_type} block_length) noexcept
        : {base_class}{{
            c.pointer(),
            end_ptr,
            block_length}}
    {{
        SBEPP_SIZE_CHECK(
            (*this)(::sbepp::detail::addressof_tag{{}}),
            (*this)(::sbepp::detail::end_ptr_tag{{}}),
            0,
            block_length);
        c.pointer() += block_length;
    }}
)",
                // clang-format on
                fmt::arg("class_name", class_name),
                fmt::arg("block_length_type", block_length_type),
                fmt::arg("base_class", base_class));
        }

        return {};
    }

    std::string make_group_entry(sbe::group& g)
    {
        const auto& dimension_type = types->get_as_or_throw<sbe::composite>(
            g.dimension_type,
            "{}: type `{}` doesn't exist or it's not a composite",
            g.location,
            g.dimension_type);
        const auto groups_impl = make_groups(g.members.groups);
        // entry should not take header size into account
        const auto accessors = make_level_accessors(g.members, 0);

        const auto class_name = make_next_group_entry_name(g.members);
        g.entry_impl_type =
            fmt::format("::{}::detail::messages::{}", schema->name, class_name);
        const auto size_bytes_impl = make_level_size_bytes_impl(
            is_flat_level(g.members), get_last_member(g.members), 0);
        const auto block_length_type = get_block_length_type(dimension_type);
        const auto base_class = fmt::format(
            "::sbepp::detail::entry_base<Byte, {}>", block_length_type);

        return fmt::format(
            // clang-format off
R"(
{groups_impl}

template<typename Byte>
class {name} : public {base_class}
{{
public:
    using {base_class}::entry_base;
    using {base_class}::operator();

    {cursor_constructor}
    {accessors}
    {size_bytes_impl}

    template<typename Visitor, typename Cursor>
    constexpr bool operator()(
        ::sbepp::detail::visit_tag, Visitor& v, Cursor& c) const
    {{
        return v.on_entry(*this, c);
    }}

    {visit_children_impl}
}};
)",
            // clang-format on
            fmt::arg("name", class_name),
            fmt::arg("dimension", dimension_type.public_type),
            fmt::arg("accessors", accessors),
            fmt::arg("size_bytes_impl", size_bytes_impl),
            fmt::arg("base_class", base_class),
            fmt::arg(
                "cursor_constructor",
                make_entry_cursor_constructor(
                    g.members, class_name, block_length_type, base_class)),
            fmt::arg("groups_impl", groups_impl),
            fmt::arg("visit_children_impl", make_visit_children(g.members)));
    }

    static bool is_flat_level(const sbe::level_members& members)
    {
        return members.groups.empty() && members.data.empty();
    }

    static bool is_flat_group(const sbe::group& g)
    {
        return is_flat_level(g.members);
    }

    static std::string get_group_base_class(const bool is_flat)
    {
        if(is_flat)
        {
            return "flat_group_base";
        }
        return "nested_group_base";
    }

    static std::string_view get_num_in_group_type(const sbe::composite& c)
    {
        const auto t = std::get_if<sbe::type>(
            utils::find_composite_element(c, "numInGroup"));
        if(t)
        {
            return t->impl_type;
        }

        throw_error(
            "{}: `numInGroup` is not found or it's not a type", c.location);
    }

    static block_length_t
        calculate_block_length(const std::vector<sbe::field>& fields)
    {
        const auto last_non_const = std::find_if(
            std::rbegin(fields),
            std::rend(fields),
            [](const auto& f)
            {
                return (f.actual_presence != field_presence::constant);
            });

        if(last_non_const != std::rend(fields))
        {
            return last_non_const->actual_offset + last_non_const->size;
        }

        return {};
    }

    std::string make_group_header_filler(sbe::group& g)
    {
        const auto& header = types->get_as_or_throw<sbe::composite>(
            g.dimension_type,
            "{}: encoding `{}` doesn't exist or it's not a composite");
        dependencies.emplace(header.name);
        const auto calculated_block_length =
            calculate_block_length(g.members.fields);

        const auto block_length =
            g.block_length.value_or(calculated_block_length);
        if(block_length < calculated_block_length)
        {
            throw_error(
                "{}: provided `blockLength` ({}) is less than the calculated "
                "one ({})",
                g.location,
                block_length,
                calculated_block_length);
        }
        g.actual_block_length = block_length;

        return fmt::format(
            // clang-format off
R"(
    // this is implementation detail, don't use it directly
    template<
        typename T = void,
        typename = ::sbepp::detail::enable_if_writable_t<Byte, T>>
    SBEPP_CPP14_CONSTEXPR {header_type}<Byte>
        operator()(
            ::sbepp::detail::fill_group_header_tag,
            {size_type} num_in_group) const noexcept
    {{
        auto header = operator()(::sbepp::detail::get_header_tag{{}});
        header.blockLength({{{block_length}}});
        header.numInGroup(num_in_group);
        {num_groups_setter}
        {num_var_data_fields_setter}
        return header;
    }}
)",
            // clang-format on
            fmt::arg("header_type", header.public_type),
            fmt::arg("size_type", get_num_in_group_type(header)),
            fmt::arg("block_length", block_length),
            fmt::arg(
                "num_groups_setter",
                make_num_groups_setter(header, g.members.groups.size())),
            fmt::arg(
                "num_var_data_fields_setter",
                make_num_var_data_fields_setter(
                    header, g.members.data.size())));
    }

    std::string make_group(sbe::group& g)
    {
        const auto entry_impl = make_group_entry(g);

        g.impl_type = fmt::format(
            "::{}::detail::messages::{}", schema->name, g.impl_name);
        const auto& dimension_encoding = types->get_as_or_throw<sbe::composite>(
            g.dimension_type,
            "{}: encoding `{}` doesn't exist or it's not a composite",
            g.location,
            g.dimension_type);
        const auto& dimension_type = dimension_encoding.public_type;
        const auto base_class = get_group_base_class(is_flat_group(g));

        return fmt::format(
            // clang-format off
R"(
{entry_impl}

template<typename Byte>
class {name} : public ::sbepp::detail::{base_class}<
                  Byte,
                  {entry}<Byte>,
                  {dimension}<Byte>>
{{
public:
        using ::sbepp::detail::{base_class}<
                Byte,
                {entry}<Byte>,
                {dimension}<Byte>>::{base_class};
        using ::sbepp::detail::{base_class}<
                Byte,
                {entry}<Byte>,
                {dimension}<Byte>>::operator();
    {header_filler}

    template<typename Visitor, typename Cursor>
    constexpr bool operator()(
        ::sbepp::detail::visit_tag, Visitor& v, Cursor& c) const
    {{
        return v.on_group(*this, c, "{public_name}");
    }}
}};
)",
            // clang-format on
            fmt::arg("name", g.impl_name),
            fmt::arg("dimension", dimension_type),
            fmt::arg("entry", g.entry_impl_type),
            fmt::arg("base_class", base_class),
            fmt::arg("header_filler", make_group_header_filler(g)),
            fmt::arg("entry_impl", entry_impl),
            fmt::arg("public_name", g.name));
    }

    static std::string make_group_accessors(std::vector<sbe::group>& groups)
    {
        std::string res;
        bool is_first = true;
        std::string_view prev_group_name;

        for(auto& g : groups)
        {
            if(is_first)
            {
                is_first = false;
                res += fmt::format(
                    // clang-format off
R"(
    constexpr {type}<Byte> {name}() const noexcept
    {{
        return ::sbepp::detail::get_first_dynamic_field_view<{type}<Byte>>(
            *this);
    }}
)",
                    // clang-format on
                    fmt::arg("name", g.name),
                    fmt::arg("type", g.impl_type));
            }
            else
            {
                res += fmt::format(
                    // clang-format off
R"(
    constexpr {type}<Byte> {name}() const noexcept
    {{
        return ::sbepp::detail::get_dynamic_field_view<{type}<Byte>>(*this, {prev_group}());
    }}
)",
                    // clang-format on
                    fmt::arg("name", g.name),
                    fmt::arg("type", g.impl_type),
                    fmt::arg("prev_group", prev_group_name));
            }

            prev_group_name = g.name;
        }

        return res;
    }

    const sbe::type& find_data_length_type(const sbe::data& d)
    {
        const auto& c = types->get_as_or_throw<sbe::composite>(
            d.type,
            "{}: length encoding `{}` doesn't exist or it's not a composite",
            d.location,
            d.type);
        dependencies.emplace(c.name);

        const auto t =
            std::get_if<sbe::type>(utils::find_composite_element(c, "length"));
        if(t)
        {
            return *t;
        }

        throw_error("{}: `length` is not found or it's not a type", c.location);
    }

    std::string_view get_data_value_type(const sbe::data& d)
    {
        const auto& c = types->get_as_or_throw<sbe::composite>(
            d.type,
            "{}: length encoding `{}` doesn't exist or it's not a composite",
            d.location,
            d.type);

        const auto t =
            std::get_if<sbe::type>(utils::find_composite_element(c, "varData"));
        if(t)
        {
            return t->underlying_type;
        }

        throw_error("{}: no `varData` element or it's not a type", c.location);
    }

    static std::string make_first_data_accessor(
        const sbe::data& d, const std::vector<sbe::group>& groups)
    {
        const auto has_groups = !groups.empty();
        if(has_groups)
        {
            return fmt::format(
                // clang-format off
R"(
    constexpr {impl_type} {name}() const noexcept
    {{
        return ::sbepp::detail::get_dynamic_field_view<{impl_type}>(
            *this, {last_group}());
    }}
)",
                // clang-format on
                fmt::arg("name", d.name),
                fmt::arg("last_group", groups.back().name),
                fmt::arg("impl_type", d.impl_type));
        }
        else
        {
            return fmt::format(
                // clang-format off
R"(
    constexpr {impl_type} {name}() const noexcept
    {{
        return ::sbepp::detail::get_first_dynamic_field_view<{impl_type}>(
            *this);
    }}
)",
                // clang-format on
                fmt::arg("name", d.name),
                fmt::arg("impl_type", d.impl_type));
        }
    }

    static std::string make_data_accessor(
        const sbe::data& d, const std::string_view prev_data_member)
    {
        return fmt::format(
            // clang-format off
R"(
    constexpr {impl_type} {name}() const noexcept
    {{
        return ::sbepp::detail::get_dynamic_field_view<{impl_type}>(
            *this, {prev_data}());
    }}
)",
            // clang-format on
            fmt::arg("name", d.name),
            fmt::arg("prev_data", prev_data_member),
            fmt::arg("impl_type", d.impl_type));
    }

    std::string make_data_impl_type(const sbe::data& d)
    {
        return fmt::format(
            // clang-format off
R"(::sbepp::detail::dynamic_array_ref<
    Byte, {value_type}, {length_type}, {endian}>)",
            // clang-format on
            fmt::arg("value_type", get_data_value_type(d)),
            fmt::arg("endian", utils::byte_order_to_endian(schema->byte_order)),
            fmt::arg("length_type", d.length_type->public_type));
    }

    std::string make_data_accessors(sbe::level_members& members)
    {
        std::string res;
        bool is_first = true;
        std::string prev_data_member;

        for(auto& d : members.data)
        {
            d.length_type = &find_data_length_type(d);
            d.impl_type = make_data_impl_type(d);
            if(is_first)
            {
                is_first = false;
                res += make_first_data_accessor(d, members.groups);
            }
            else
            {
                res += make_data_accessor(d, prev_data_member);
            }
            prev_data_member = d.name;
        }

        return res;
    }

    std::string make_primitive_cursor_accessors(
        const std::string_view name,
        const std::string_view type,
        const offset_t offset,
        const std::size_t size,
        const offset_t absolute_offset) const
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR auto
        {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor, {type}>
    {{
        return c.template get_value<{type}, {type}, {endian}>(
            *this, {offset}, {absolute_offset});
    }}

    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_writeable_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR void {name}(
        {type} v, Cursor&& c) const noexcept
    {{
        return c.template set_value<{endian}>(
            *this, {offset}, {absolute_offset}, v);
    }}
)",
            // clang-format on
            fmt::arg("name", name),
            fmt::arg("type", type),
            fmt::arg("offset", offset),
            fmt::arg("size", size),
            fmt::arg("absolute_offset", absolute_offset),
            fmt::arg(
                "endian", utils::byte_order_to_endian(schema->byte_order)));
    }

    static std::string make_array_cursor_accessors(
        const std::string_view name,
        const std::string_view type,
        const offset_t offset,
        const offset_t absolute_offset)
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR auto
        {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor, {type}<
            ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    {{
        return c.template get_static_field_view<{type}<
            ::sbepp::detail::cursor_byte_type_t<Cursor>>>(
                *this, {offset}, {absolute_offset});
    }}
)",
            // clang-format on
            fmt::arg("name", name),
            fmt::arg("type", type),
            fmt::arg("offset", offset),
            fmt::arg("absolute_offset", absolute_offset));
    }

    std::string make_type_cursor_accessors(
        const std::string_view name,
        const std::string_view public_type,
        const offset_t offset,
        const std::size_t size,
        const offset_t absolute_offset,
        const std::string_view underlying_type) const
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR auto
        {name}(Cursor&& c) const noexcept
            -> ::sbepp::detail::cursor_result_type_t<Cursor, {type}>
    {{
        return c.template get_value<{type}, {underlying_type}, {endian}>(
            *this, {offset}, {absolute_offset});
    }}

    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_writeable_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR void {name}(
        {type} v, Cursor&& c) const noexcept
    {{
        return c.template set_value<{endian}>(
            *this, {offset}, {absolute_offset}, v.value());
    }}
)",
            // clang-format on
            fmt::arg("name", name),
            fmt::arg("type", public_type),
            fmt::arg("offset", offset),
            fmt::arg("size", size),
            fmt::arg("absolute_offset", absolute_offset),
            fmt::arg("underlying_type", underlying_type),
            fmt::arg(
                "endian", utils::byte_order_to_endian(schema->byte_order)));
    }

    std::string make_cursor_accessors(
        const sbe::type& enc,
        const std::string_view name,
        const offset_t offset,
        const offset_t absolute_offset) const
    {
        if(enc.length == 1)
        {
            return make_type_cursor_accessors(
                name,
                enc.public_type,
                offset,
                enc.size,
                absolute_offset,
                enc.underlying_type);
        }
        return make_array_cursor_accessors(
            name, enc.public_type, offset, absolute_offset);
    }

    std::string make_cursor_accessors(
        const sbe::enumeration& enc,
        const std::string_view name,
        const offset_t offset,
        const offset_t absolute_offset) const
    {
        return make_primitive_cursor_accessors(
            name, enc.public_type, offset, enc.size, absolute_offset);
    }

    std::string make_cursor_accessors(
        const sbe::set& s,
        const std::string_view name,
        const offset_t offset,
        const offset_t absolute_offset) const
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR auto
        {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor, {type}>
    {{
        return c.template get_value<{type}, {underlying_type}, {endian}>(
            *this, {offset}, {absolute_offset});
    }}

    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_writeable_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR void {name}(
        {type} v, Cursor&& c) const noexcept
    {{
        c.template set_value<{endian}>(*this, {offset}, {absolute_offset}, *v);
    }}
)",
            // clang-format on
            fmt::arg("name", name),
            fmt::arg("type", s.public_type),
            fmt::arg("offset", offset),
            fmt::arg("size", s.size),
            fmt::arg("absolute_offset", absolute_offset),
            fmt::arg("underlying_type", s.underlying_type),
            fmt::arg(
                "endian", utils::byte_order_to_endian(schema->byte_order)));
    }

    static std::string make_cursor_accessors(
        const sbe::composite& c,
        const std::string_view name,
        const offset_t offset,
        const offset_t absolute_offset)
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR auto {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor, 
            {type}<::sbepp::detail::cursor_byte_type_t<Cursor>>>
    {{
        return c.template get_static_field_view<{type}<::sbepp::detail::cursor_byte_type_t<Cursor>>>(
            *this, {offset}, {absolute_offset});
    }}
)",
            // clang-format on
            fmt::arg("name", name),
            fmt::arg("type", c.public_type),
            fmt::arg("offset", offset),
            fmt::arg("absolute_offset", absolute_offset));
    }

    std::string make_last_primitive_cursor_accessors(
        const std::string_view name,
        const std::string_view type,
        const offset_t offset,
        const std::size_t header_size,
        const offset_t absolute_offset) const
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR auto {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor, {type}>
    {{
        return c.template get_last_value<{type}, {type}, {endian}>(
            *this, {offset}, {absolute_offset});
    }}

    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_writeable_t<Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR void {name}({type} v, Cursor&& c) const noexcept
    {{
        return c.template set_last_value<{endian}>(*this, {offset}, {absolute_offset}, v);
    }}
)",
            // clang-format on
            fmt::arg("name", name),
            fmt::arg("type", type),
            fmt::arg("offset", offset),
            fmt::arg("header_size", header_size),
            fmt::arg("absolute_offset", absolute_offset),
            fmt::arg(
                "endian", utils::byte_order_to_endian(schema->byte_order)));
    }

    static std::string make_last_array_cursor_accessor(
        const std::string_view name,
        const std::string_view type,
        const sbepp::offset_t offset,
        const std::size_t header_size,
        const sbepp::offset_t absolute_offset)
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR auto {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor,
            {type}<::sbepp::detail::cursor_byte_type_t<Cursor>>>
    {{
        return c.template get_last_static_field_view<{type}<::sbepp::detail::cursor_byte_type_t<Cursor>>>(
            *this, {offset}, {absolute_offset});
    }}
)",
            // clang-format on
            fmt::arg("name", name),
            fmt::arg("type", type),
            fmt::arg("offset", offset),
            fmt::arg("header_size", header_size),
            fmt::arg("absolute_offset", absolute_offset));
    }

    std::string make_last_type_cursor_accessors(
        const std::string_view name,
        const offset_t offset,
        const std::string_view public_type,
        const std::size_t header_size,
        const offset_t absolute_offset,
        const std::string_view underlying_type) const
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR auto {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor, {type}>
    {{
        return c.template get_last_value<{type}, {underlying_type}, {endian}>(
            *this, {offset}, {absolute_offset});
    }}

    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_writeable_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR void {name}({type} v, Cursor&& c) const noexcept
    {{
        return c.template set_last_value<{endian}>(
            *this, {offset}, {absolute_offset}, v.value());
    }}
)",
            // clang-format on
            fmt::arg("name", name),
            fmt::arg("type", public_type),
            fmt::arg("offset", offset),
            fmt::arg("header_size", header_size),
            fmt::arg("absolute_offset", absolute_offset),
            fmt::arg("underlying_type", underlying_type),
            fmt::arg(
                "endian", utils::byte_order_to_endian(schema->byte_order)));
    }

    std::string make_last_cursor_accessors(
        const sbe::type& enc,
        const std::string_view name,
        const offset_t offset,
        const std::size_t header_size,
        const offset_t absolute_offset) const
    {
        if(enc.length == 1)
        {
            return make_last_type_cursor_accessors(
                name,
                offset,
                enc.public_type,
                header_size,
                absolute_offset,
                enc.underlying_type);
        }
        return make_last_array_cursor_accessor(
            name, enc.public_type, offset, header_size, absolute_offset);
    }

    std::string make_last_cursor_accessors(
        const sbe::enumeration& enc,
        const std::string_view name,
        const offset_t offset,
        const std::size_t header_size,
        const offset_t absolute_offset) const
    {
        return make_last_primitive_cursor_accessors(
            name, enc.public_type, offset, header_size, absolute_offset);
    }

    std::string make_last_cursor_accessors(
        const sbe::set& s,
        const std::string_view name,
        const offset_t offset,
        const std::size_t header_size,
        const offset_t absolute_offset) const
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR auto {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor, {type}>
    {{
        return c.template get_last_value<{type}, {underlying_type}, {endian}>(
            *this, {offset}, {absolute_offset});
    }}

    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_writeable_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR void {name}({type} v, Cursor&& c) const noexcept
    {{
        c.template set_last_value<{endian}>(
            *this, {offset}, {absolute_offset}, *v);
    }}
)",
            // clang-format on
            fmt::arg("name", name),
            fmt::arg("type", s.public_type),
            fmt::arg("offset", offset),
            fmt::arg("header_size", header_size),
            fmt::arg("absolute_offset", absolute_offset),
            fmt::arg("underlying_type", s.underlying_type),
            fmt::arg(
                "endian", utils::byte_order_to_endian(schema->byte_order)));
    }

    static std::string make_last_cursor_accessors(
        const sbe::composite& c,
        const std::string_view name,
        const offset_t offset,
        const std::size_t header_size,
        const offset_t absolute_offset)
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    SBEPP_CPP20_CONSTEXPR auto {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor,
            {type}<::sbepp::detail::cursor_byte_type_t<Cursor>>>
    {{
        return c.template get_last_static_field_view<{type}<
            ::sbepp::detail::cursor_byte_type_t<Cursor>>>(
                *this, {offset}, {absolute_offset});
    }}
)",
            // clang-format on
            fmt::arg("name", name),
            fmt::arg("type", c.public_type),
            fmt::arg("offset", offset),
            fmt::arg("header_size", header_size),
            fmt::arg("absolute_offset", absolute_offset));
    }

    std::string make_fields_cursor_accessors(
        const std::vector<sbe::field>& fields, const std::size_t header_size)
    {
        if(fields.empty())
        {
            return {};
        }

        std::string res;
        std::size_t absolute_offset{};

        std::for_each(
            std::cbegin(fields),
            std::cend(fields) - 1,
            [&res, this, &absolute_offset, header_size](const auto& f)
            {
                if(f.actual_presence == field_presence::constant)
                {
                    return;
                }

                // TODO: this can be simplified if use `get_level_tag` and
                // offsets which don't include header size
                const auto prev_field_end = absolute_offset;
                absolute_offset = utils::get_valid_offset(
                    f.offset, absolute_offset, f.location);
                const auto relative_offset = absolute_offset - prev_field_end;
                if(utils::is_primitive_type(f.type))
                {
                    const auto type = utils::primitive_type_to_cpp_type(f.type);
                    const auto size = utils::get_underlying_size(type);
                    res += make_type_cursor_accessors(
                        f.name,
                        utils::primitive_type_to_wrapper_type(
                            f.type, f.presence),
                        relative_offset,
                        size,
                        absolute_offset + header_size,
                        type);
                    absolute_offset += size;
                }
                else
                {
                    res += std::visit(
                        [relative_offset,
                         &f,
                         &absolute_offset,
                         this,
                         header_size](const auto& enc)
                        {
                            auto res = this->make_cursor_accessors(
                                enc,
                                f.name,
                                relative_offset,
                                absolute_offset + header_size);
                            absolute_offset += enc.size;
                            return res;
                        },
                        types->get_or_throw(
                            f.type,
                            "{}: encoding `{}` doesn't exist",
                            f.location,
                            f.type));
                }
            });

        const auto& last = fields.back();

        if(last.actual_presence == field_presence::constant)
        {
            return res;
        }

        const auto prev_field_end = absolute_offset;
        absolute_offset = utils::get_valid_offset(
            last.offset, absolute_offset, last.location);
        const auto relative_offset = absolute_offset - prev_field_end;
        if(utils::is_primitive_type(last.type))
        {
            const auto type = utils::primitive_type_to_cpp_type(last.type);
            res += make_last_type_cursor_accessors(
                last.name,
                relative_offset,
                utils::primitive_type_to_wrapper_type(last.type, last.presence),
                header_size,
                absolute_offset + header_size,
                type);
        }
        else
        {
            res += std::visit(
                [relative_offset, &last, header_size, absolute_offset, this](
                    const auto& enc)
                {
                    auto res = this->make_last_cursor_accessors(
                        enc,
                        last.name,
                        relative_offset,
                        header_size,
                        absolute_offset + header_size);
                    return res;
                },
                types->get_or_throw(
                    last.type,
                    "{}: encoding `{}` doesn't exist",
                    last.location,
                    last.type));
        }

        return res;
    }

    std::string make_first_group_cursor_accessor(const sbe::group& g)
    {
        const auto group_header_size =
            types
                ->get_as_or_throw<sbe::composite>(
                    g.dimension_type,
                    "{}: group encoding `{}` doesn't exist or it's not a "
                    "composite",
                    g.location,
                    g.dimension_type)
                .size;

        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    constexpr auto {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor,
            {type}<::sbepp::detail::cursor_byte_type_t<Cursor>>>
    {{
        return c.template get_first_group_view<
            {type}<::sbepp::detail::cursor_byte_type_t<Cursor>>>(*this);
    }}
)",
            // clang-format on
            fmt::arg("name", g.name),
            fmt::arg("type", g.impl_type),
            fmt::arg("group_header_size", group_header_size));
    }

    std::string make_group_cursor_accessor(const sbe::group& g)
    {
        const auto group_header_size =
            types
                ->get_as_or_throw<sbe::composite>(
                    g.dimension_type,
                    "{}: group encoding `{}` doesn't exist or it's not a "
                    "composite",
                    g.location,
                    g.dimension_type)
                .size;

        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    constexpr auto {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor,
            {type}<::sbepp::detail::cursor_byte_type_t<Cursor>>>
    {{
        return c.template get_group_view<
            {type}<::sbepp::detail::cursor_byte_type_t<Cursor>>>(
                *this, [this](){{return this->{name}();}});
    }}
)",
            // clang-format on
            fmt::arg("name", g.name),
            fmt::arg("type", g.impl_type),
            fmt::arg("group_header_size", group_header_size));
    }

    std::string
        make_groups_cursor_accessors(const std::vector<sbe::group>& groups)
    {
        if(groups.empty())
        {
            return {};
        }

        std::string res;

        const auto& first = groups.front();
        res += make_first_group_cursor_accessor(first);

        std::for_each(
            std::cbegin(groups) + 1,
            std::cend(groups),
            [&res, this](const auto& g)
            {
                res += make_group_cursor_accessor(g);
            });

        return res;
    }

    std::string make_first_data_cursor_accessor(const sbe::data& d)
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    constexpr auto {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor,
            ::sbepp::detail::dynamic_array_ref<
                ::sbepp::detail::cursor_byte_type_t<Cursor>,
                    {value_type}, {length_type}, {endian}>>
    {{
        return c.template get_first_data_view<
            ::sbepp::detail::dynamic_array_ref<
                ::sbepp::detail::cursor_byte_type_t<Cursor>,
                    {value_type}, {length_type}, {endian}>>(
                        *this);
    }}
)",
            // clang-format on
            fmt::arg("name", d.name),
            fmt::arg("length_type", d.length_type->public_type),
            fmt::arg("value_type", get_data_value_type(d)),
            fmt::arg(
                "endian", utils::byte_order_to_endian(schema->byte_order)));
    }

    std::string make_data_cursor_accessor(const sbe::data& d)
    {
        return fmt::format(
            // clang-format off
R"(
    template<
        typename Cursor,
        typename = ::sbepp::detail::enable_if_cursor_compatible_t<
            Byte, ::sbepp::detail::cursor_byte_type_t<Cursor>>>
    constexpr auto {name}(Cursor&& c) const noexcept
        -> ::sbepp::detail::cursor_result_type_t<Cursor,
            ::sbepp::detail::dynamic_array_ref<
                ::sbepp::detail::cursor_byte_type_t<Cursor>,
                    {value_type}, {length_type}, {endian}>>
    {{
        return c.template get_data_view<
            ::sbepp::detail::dynamic_array_ref<
                ::sbepp::detail::cursor_byte_type_t<Cursor>, {value_type},
                    {length_type}, {endian}>>(
                        *this, [this](){{return this->{name}();}});
    }}
)",
            // clang-format on
            fmt::arg("name", d.name),
            fmt::arg("length_type", d.length_type->public_type),
            fmt::arg("value_type", get_data_value_type(d)),
            fmt::arg(
                "endian", utils::byte_order_to_endian(schema->byte_order)));
    }

    std::string make_data_cursor_accessors(const sbe::level_members& members)
    {
        if(members.data.empty())
        {
            return {};
        }

        std::string res;
        const auto has_groups = !members.groups.empty();
        const auto& first = members.data.front();

        if(has_groups)
        {
            res += make_data_cursor_accessor(first);
        }
        else
        {
            res += make_first_data_cursor_accessor(first);
        }

        std::for_each(
            std::cbegin(members.data) + 1,
            std::cend(members.data),
            [&res, this](const auto& d)
            {
                res += make_data_cursor_accessor(d);
            });

        return res;
    }

    std::string make_level_accessors(
        sbe::level_members& members, const std::size_t header_size)
    {
        std::string res;

        res += make_field_accessors(members.fields, header_size);
        res += make_group_accessors(members.groups);
        res += make_data_accessors(members);

        res += make_fields_cursor_accessors(members.fields, header_size);
        res += make_groups_cursor_accessors(members.groups);
        res += make_data_cursor_accessors(members);

        return res;
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

    static bool has_num_groups(const sbe::composite& c)
    {
        return utils::find_composite_element(c, "numGroups");
    }

    static bool has_num_var_data_fields(const sbe::composite& c)
    {
        return utils::find_composite_element(c, "numVarDataFields");
    }

    static std::string make_num_groups_setter(
        const sbe::composite& header, std::size_t groups_count)
    {
        if(has_num_groups(header))
        {
            return fmt::format(
                // clang-format off
R"(header.numGroups({{{}}});
)",
                // clang-format on
                groups_count);
        }

        return {};
    }

    static std::string make_num_var_data_fields_setter(
        const sbe::composite& header, std::size_t var_data_count)
    {
        if(has_num_var_data_fields(header))
        {
            return fmt::format(
                // clang-format off
R"(header.numVarDataFields({{{}}});
)",
                // clang-format on
                var_data_count);
        }

        return {};
    }

    std::string make_message_header_filler(sbe::message& m)
    {
        const auto& header = types->get_as_or_throw<sbe::composite>(
            schema->header_type,
            "{}: message header type `{}` doesn't exist or it's not a "
            "composite",
            m.location,
            schema->header_type);
        dependencies.emplace(header.name);
        const auto calculated_block_length =
            calculate_block_length(m.members.fields);

        const auto block_length =
            m.block_length.value_or(calculated_block_length);
        if(block_length < calculated_block_length)
        {
            throw_error(
                "{}: custom blockLength `{}` is less than the minimal one `{}`",
                m.location,
                block_length,
                calculated_block_length);
        }
        m.actual_block_length = block_length;

        return fmt::format(
            // clang-format off
R"(
    template<
        typename T = void,
        typename = ::sbepp::detail::enable_if_writable_t<Byte, T>>
    SBEPP_CPP14_CONSTEXPR {header_type}<Byte>
        operator()(::sbepp::detail::fill_message_header_tag) const noexcept
    {{
        auto header = operator()(::sbepp::detail::get_header_tag{{}});
        header.schemaId({{{schema_id}}});
        header.templateId({{{template_id}}});
        header.version({{{schema_version}}});
        header.blockLength({{{block_length}}});
        {num_groups_setter}
        {num_var_data_fields_setter}
        return header;
    }}
)",
            // clang-format on
            fmt::arg("header_type", header.public_type),
            fmt::arg("schema_id", schema->id),
            fmt::arg("template_id", m.id),
            fmt::arg("schema_version", schema->version),
            fmt::arg("block_length", block_length),
            fmt::arg(
                "num_groups_setter",
                make_num_groups_setter(header, m.members.groups.size())),
            fmt::arg(
                "num_var_data_fields_setter",
                make_num_var_data_fields_setter(
                    header, m.members.data.size())));
    }

    std::string make_groups(std::vector<sbe::group>& groups)
    {
        std::string res;

        for(auto& g : groups)
        {
            res += make_group(g);
        }
        return res;
    }

    static std::string make_alias(const sbe::message& m)
    {
        return utils::make_alias_template(m.name, m.impl_type);
    }

    static std::vector<std::string>
        make_member_visit_calls(const sbe::level_members& members)
    {
        std::vector<std::string> res;
        res.reserve(
            members.fields.size() + members.groups.size()
            + members.data.size());

        for(const auto& f : members.fields)
        {
            if(f.actual_presence == field_presence::constant)
            {
                continue;
            }

            res.push_back(fmt::format(
                "v.on_field(this->{name}(c), {tag}{{}})",
                fmt::arg("name", f.name),
                fmt::arg("tag", f.tag)));
        }

        for(const auto& g : members.groups)
        {
            res.push_back(fmt::format(
                "v.on_group(this->{name}(c), c, {tag}{{}})",
                fmt::arg("name", g.name),
                fmt::arg("tag", g.tag)));
        }

        for(const auto& d : members.data)
        {
            res.push_back(fmt::format(
                "v.on_data(this->{name}(c), {tag}{{}})",
                fmt::arg("name", d.name),
                fmt::arg("tag", d.tag)));
        }

        return res;
    }

    static std::string make_visit_children(const sbe::level_members& members)
    {
        std::string res;
        auto member_visit_calls = make_member_visit_calls(members);
        if(member_visit_calls.empty())
        {
            member_visit_calls.emplace_back("false");
        }

        res += fmt::format(
            // clang-format off
R"(
    template<typename Visitor, typename Cursor>
    constexpr bool operator()(
        ::sbepp::detail::visit_children_tag, Visitor& v, Cursor& c) const
    {{
        return {member_visitors};
    }}
)",
            // clang-format on
            fmt::arg("member_visitors", fmt::join(member_visit_calls, "\n||")));

        return res;
    }

    void compile_message(sbe::message& m)
    {
        dependencies.clear();

        const auto& header = types->get_as_or_throw<sbe::composite>(
            schema->header_type,
            "{}: message header type `{}` doesn't exist or it's not a "
            "composite",
            m.location,
            schema->header_type);

        const auto groups = make_groups(m.members.groups);
        const auto accessors = make_level_accessors(m.members, header.size);

        m.impl_type = fmt::format(
            "::{}::detail::messages::{}", schema->name, m.impl_name);

        const auto& header_type = header.public_type;
        const auto size_bytes_impl = make_level_size_bytes_impl(
            is_flat_level(m.members), get_last_member(m.members), header.size);
        const auto visit_children_impl = make_visit_children(m.members);

        // it's not possible to make `operator()(visit_tag)` `constexpr` because
        // C++11 doesn't allow such function to return `void`
        const auto implementation = fmt::format(
            // clang-format off
R"(
{groups}

template<typename Byte>
class {name} : public ::sbepp::detail::message_base<
    Byte, {header_type}<Byte>>
{{
public:
    using ::sbepp::detail::message_base<
        Byte, {header_type}<Byte>>::message_base;
    using ::sbepp::detail::message_base<
        Byte, {header_type}<Byte>>::operator();

{accessors}
{header_filler}
{size_getter}

    template<typename Visitor, typename Cursor>
    SBEPP_CPP14_CONSTEXPR void operator()(
        ::sbepp::detail::visit_tag, Visitor& v, Cursor& c)
    {{
        v.on_message(*this, c, {tag}{{}});
    }}

{visit_children_impl}
}};
)",
            // clang-format on
            fmt::arg("name", m.impl_name),
            fmt::arg("header_type", header_type),
            fmt::arg("accessors", accessors),
            fmt::arg("size_getter", size_bytes_impl),
            fmt::arg("header_filler", make_message_header_filler(m)),
            fmt::arg("groups", groups),
            fmt::arg("visit_children_impl", visit_children_impl),
            fmt::arg("tag", m.tag));

        m.public_type = fmt::format(
            "::{schema}::messages::{name}",
            fmt::arg("schema", schema->name),
            fmt::arg("name", m.name));

        const auto alias = make_alias(m);
        const auto traits = traits_gen->make_message_traits(m);
        on_message_cb(m.name, implementation, alias, dependencies, traits);
    }
};
} // namespace sbepp::sbeppc
