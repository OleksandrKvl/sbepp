// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/traits_generator.hpp>
#include <sbepp/sbeppc/normal_accessors.hpp>
#include <sbepp/sbeppc/context_manager.hpp>

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
        traits_generator& traits_gen,
        context_manager& ctx_manager)
        : schema{&schema}, traits_gen{&traits_gen}, ctx_manager{&ctx_manager}
    {
    }

    void compile(on_message_cb_t cb)
    {
        on_message_cb = std::move(cb);

        for(const auto& m : schema->messages)
        {
            compile_message(m);
        }
    }

private:
    const sbe::message_schema* schema{};
    traits_generator* traits_gen{};
    context_manager* ctx_manager{};
    on_message_cb_t on_message_cb;
    std::size_t group_entry_index{};
    std::unordered_set<std::string> dependencies;

    bool is_const_field(const sbe::field& f)
    {
        // TODO: use context.actual_presence here
        if(!utils::is_primitive_type(f.type))
        {
            const auto& enc = utils::get_schema_encoding(*schema, f.type);
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

    std::string value_ref_to_enumerator(const std::string_view value_ref) const
    {
        const auto parsed = utils::parse_value_ref(value_ref);
        const auto& e = utils::get_schema_encoding_as<sbe::enumeration>(
            *schema, parsed.enum_name);

        return fmt::format(
            "{}::{}", ctx_manager->get(e).impl_type, parsed.enumerator);
    }

    std::string value_ref_to_enum_value(const std::string_view value_ref) const
    {
        return fmt::format(
            "::sbepp::to_underlying({})", value_ref_to_enumerator(value_ref));
    }

    std::string get_const_value(const sbe::type& t)
    {
        // the only difference from `types_compiler::get_const_value` is that
        // `value_ref_to_enum_value` never compiles enum type
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

    std::string get_const_type(const sbe::type& t) const
    {
        const auto& context = ctx_manager->get(t);
        if(t.length != 1)
        {
            return context.public_type;
        }
        return context.underlying_type;
    }

    std::string make_const_field_accessor(const sbe::field& f)
    {
        auto& context = ctx_manager->get(f);

        if(!utils::is_primitive_type(f.type))
        {
            const auto& enc = utils::get_schema_encoding(*schema, f.type);
            dependencies.emplace(utils::get_encoding_name(enc));
            if(auto t = std::get_if<sbe::type>(&enc))
            {
                context.value_type = get_const_type(*t);
                return normal_accessors::make_constant_accessor(
                    f.name, context.value_type, get_const_value(*t));
            }
            else if(auto e = std::get_if<sbe::enumeration>(&enc))
            {
                context.value_type = ctx_manager->get(*e).public_type;
                return normal_accessors::make_constant_accessor(
                    f.name,
                    context.value_type,
                    value_ref_to_enumerator(*f.value_ref));
            }

            // TODO:  check if it's possible now
            // throw_error(
            //     "{}: only types and enums can represent field constants",
            //     f.location);
        }

        context.value_type = utils::primitive_type_to_cpp_type(f.type);

        return normal_accessors::make_constant_accessor(
            f.name, context.value_type, value_ref_to_enum_value(*f.value_ref));
    }

    std::string make_field_accessors(
        const std::vector<sbe::field>& fields, const std::size_t header_size)
    {
        std::string res;

        for(const auto& f : fields)
        {
            auto& context = ctx_manager->get(f);
            context.is_template = false;
            if(context.actual_presence == field_presence::constant)
            {
                res += make_const_field_accessor(f);
                continue;
            }

            if(utils::is_primitive_type(f.type))
            {
                context.value_type =
                    utils::primitive_type_to_wrapper_type(f.type, f.presence);
                context.value_type_tag = context.value_type;

                res += normal_accessors::make_type_accessor(
                    f.name,
                    context.value_type,
                    context.level_offset + header_size,
                    schema->byte_order);
            }
            else
            {
                const auto& enc = utils::get_schema_encoding(*schema, f.type);
                if(const auto t = std::get_if<sbe::type>(&enc);
                   (t && t->length != 1)
                   || std::holds_alternative<sbe::composite>(enc))
                {
                    context.is_template = true;
                }

                res += std::visit(
                    [&f, &context, this, header_size](const auto& enc)
                    {
                        const auto& enc_context = ctx_manager->get(enc);
                        context.value_type = enc_context.public_type;
                        context.value_type_tag = enc_context.tag;
                        dependencies.emplace(enc.name);
                        return normal_accessors::make_accessor(
                            enc,
                            context.level_offset + header_size,
                            f.name,
                            schema->byte_order,
                            true,
                            enc_context);
                    },
                    enc);
            }
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

    std::string_view get_block_length_type(const sbe::composite& c) const
    {
        // TODO: support `ref`, see also
        // `sbe_checker::validate_message_header_element`
        const auto t = std::get_if<sbe::type>(
            utils::find_composite_element(c, "blockLength"));
        assert(t);

        return ctx_manager->get(*t).underlying_type;
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

    std::string make_group_entry(const sbe::group& g)
    {
        const auto& dimension_type =
            utils::get_schema_encoding_as<sbe::composite>(
                *schema, g.dimension_type);
        const auto groups_impl = make_groups(g.members.groups);
        // entry should not take header size into account
        const auto accessors = make_level_accessors(g.members, 0);

        const auto class_name = make_next_group_entry_name(g.members);
        ctx_manager->get(g).entry_impl_type = fmt::format(
            "::{}::detail::messages::{}",
            ctx_manager->get(*schema).name,
            class_name);
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
            fmt::arg("dimension", ctx_manager->get(dimension_type).public_type),
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

    std::string_view get_num_in_group_type(const sbe::composite& c) const
    {
        const auto t = std::get_if<sbe::type>(
            utils::find_composite_element(c, "numInGroup"));
        assert(t);

        return ctx_manager->get(*t).impl_type;
    }

    std::string make_group_header_filler(const sbe::group& g)
    {
        const auto& header = utils::get_schema_encoding_as<sbe::composite>(
            *schema, g.dimension_type);
        dependencies.emplace(header.name);

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
            fmt::arg("header_type", ctx_manager->get(header).public_type),
            fmt::arg("size_type", get_num_in_group_type(header)),
            fmt::arg("block_length", ctx_manager->get(g).actual_block_length),
            fmt::arg(
                "num_groups_setter",
                make_num_groups_setter(header, g.members.groups.size())),
            fmt::arg(
                "num_var_data_fields_setter",
                make_num_var_data_fields_setter(
                    header, g.members.data.size())));
    }

    std::string make_group(const sbe::group& g)
    {
        const auto entry_impl = make_group_entry(g);
        auto& context = ctx_manager->get(g);

        context.impl_type = fmt::format(
            "::{}::detail::messages::{}",
            ctx_manager->get(*schema).name,
            context.impl_name);
        const auto& dimension_encoding =
            utils::get_schema_encoding_as<sbe::composite>(
                *schema, g.dimension_type);
        const auto& dimension_type =
            ctx_manager->get(dimension_encoding).public_type;
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
            fmt::arg("name", context.impl_name),
            fmt::arg("dimension", dimension_type),
            fmt::arg("entry", context.entry_impl_type),
            fmt::arg("base_class", base_class),
            fmt::arg("header_filler", make_group_header_filler(g)),
            fmt::arg("entry_impl", entry_impl),
            fmt::arg("public_name", g.name));
    }

    std::string make_group_accessors(const std::vector<sbe::group>& groups)
    {
        std::string res;
        bool is_first = true;
        std::string_view prev_group_name;

        for(auto& g : groups)
        {
            const auto& context = ctx_manager->get(g);
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
                    fmt::arg("type", context.impl_type));
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
                    fmt::arg("type", context.impl_type),
                    fmt::arg("prev_group", prev_group_name));
            }

            prev_group_name = g.name;
        }

        return res;
    }

    const sbe::type& find_data_length_type(const sbe::data& d)
    {
        const auto& c =
            utils::get_schema_encoding_as<sbe::composite>(*schema, d.type);
        // TODO: a weird place to insert a dependency
        dependencies.emplace(c.name);

        const auto& t =
            std::get<sbe::type>(*utils::find_composite_element(c, "length"));

        return t;
    }

    std::string_view get_data_value_type(const sbe::data& d)
    {
        const auto& c =
            utils::get_schema_encoding_as<sbe::composite>(*schema, d.type);
        const auto& t =
            std::get<sbe::type>(*utils::find_composite_element(c, "varData"));

        return ctx_manager->get(t).underlying_type;
    }

    std::string make_first_data_accessor(
        const sbe::data& d, const std::vector<sbe::group>& groups) const
    {
        const auto& context = ctx_manager->get(d);
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
                fmt::arg("impl_type", context.impl_type));
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
                fmt::arg("impl_type", context.impl_type));
        }
    }

    std::string make_data_accessor(
        const sbe::data& d, const std::string_view prev_data_member) const
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
            fmt::arg("impl_type", ctx_manager->get(d).impl_type));
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
            fmt::arg(
                "length_type",
                // TODO: refactor
                ctx_manager->get(*ctx_manager->get(d).length_type)
                    .public_type));
    }

    std::string make_data_accessors(const sbe::level_members& members)
    {
        std::string res;
        bool is_first = true;
        std::string prev_data_member;

        for(auto& d : members.data)
        {
            auto& context = ctx_manager->get(d);
            context.length_type = &find_data_length_type(d);
            context.impl_type = make_data_impl_type(d);
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
        const sbe::type& t,
        const std::string_view name,
        const offset_t offset,
        const offset_t absolute_offset) const
    {
        const auto& context = ctx_manager->get(t);

        if(t.length == 1)
        {
            return make_type_cursor_accessors(
                name,
                context.public_type,
                offset,
                context.size,
                absolute_offset,
                context.underlying_type);
        }
        return make_array_cursor_accessors(
            name, context.public_type, offset, absolute_offset);
    }

    std::string make_cursor_accessors(
        const sbe::enumeration& e,
        const std::string_view name,
        const offset_t offset,
        const offset_t absolute_offset) const
    {
        const auto& context = ctx_manager->get(e);

        return make_primitive_cursor_accessors(
            name, context.public_type, offset, context.size, absolute_offset);
    }

    std::string make_cursor_accessors(
        const sbe::set& s,
        const std::string_view name,
        const offset_t offset,
        const offset_t absolute_offset) const
    {
        const auto& context = ctx_manager->get(s);

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
            fmt::arg("type", context.public_type),
            fmt::arg("offset", offset),
            fmt::arg("size", context.size),
            fmt::arg("absolute_offset", absolute_offset),
            fmt::arg("underlying_type", context.underlying_type),
            fmt::arg(
                "endian", utils::byte_order_to_endian(schema->byte_order)));
    }

    std::string make_cursor_accessors(
        const sbe::composite& c,
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
            fmt::arg("type", ctx_manager->get(c).public_type),
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
        const sbe::type& t,
        const std::string_view name,
        const offset_t offset,
        const std::size_t header_size,
        const offset_t absolute_offset) const
    {
        const auto& context = ctx_manager->get(t);

        if(t.length == 1)
        {
            return make_last_type_cursor_accessors(
                name,
                offset,
                context.public_type,
                header_size,
                absolute_offset,
                context.underlying_type);
        }
        return make_last_array_cursor_accessor(
            name, context.public_type, offset, header_size, absolute_offset);
    }

    std::string make_last_cursor_accessors(
        const sbe::enumeration& e,
        const std::string_view name,
        const offset_t offset,
        const std::size_t header_size,
        const offset_t absolute_offset) const
    {
        return make_last_primitive_cursor_accessors(
            name,
            ctx_manager->get(e).public_type,
            offset,
            header_size,
            absolute_offset);
    }

    std::string make_last_cursor_accessors(
        const sbe::set& s,
        const std::string_view name,
        const offset_t offset,
        const std::size_t header_size,
        const offset_t absolute_offset) const
    {
        const auto& context = ctx_manager->get(s);

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
            fmt::arg("type", context.public_type),
            fmt::arg("offset", offset),
            fmt::arg("header_size", header_size),
            fmt::arg("absolute_offset", absolute_offset),
            fmt::arg("underlying_type", context.underlying_type),
            fmt::arg(
                "endian", utils::byte_order_to_endian(schema->byte_order)));
    }

    std::string make_last_cursor_accessors(
        const sbe::composite& c,
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
            fmt::arg("type", ctx_manager->get(c).public_type),
            fmt::arg("offset", offset),
            fmt::arg("header_size", header_size),
            fmt::arg("absolute_offset", absolute_offset));
    }

    std::vector<const sbe::field*>
        get_non_const_fields(const std::vector<sbe::field>& fields)
    {
        std::vector<const sbe::field*> res;
        res.reserve(fields.size());

        for(const auto& f : fields)
        {
            if(ctx_manager->get(f).actual_presence != field_presence::constant)
            {
                res.push_back(&f);
            }
        }

        return res;
    }

    std::string make_fields_cursor_accessors(
        const std::vector<sbe::field>& fields, const std::size_t header_size)
    {
        const auto non_const_fields = get_non_const_fields(fields);

        if(non_const_fields.empty())
        {
            return {};
        }

        std::string res;
        std::size_t absolute_offset{};

        std::for_each(
            std::cbegin(non_const_fields),
            std::cend(non_const_fields) - 1,
            [&res, this, &absolute_offset, header_size](const auto pf)
            {
                const auto& f = *pf;
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
                            absolute_offset += ctx_manager->get(enc).size;
                            return res;
                        },
                        utils::get_schema_encoding(*schema, f.type));
                }
            });

        const auto& last = *non_const_fields.back();
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
                utils::get_schema_encoding(*schema, last.type));
        }

        return res;
    }

    std::string make_first_group_cursor_accessor(const sbe::group& g)
    {
        const auto group_header_size =
            ctx_manager
                ->get(utils::get_schema_encoding_as<sbe::composite>(
                    *schema, g.dimension_type))
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
            fmt::arg("type", ctx_manager->get(g).impl_type),
            fmt::arg("group_header_size", group_header_size));
    }

    std::string make_group_cursor_accessor(const sbe::group& g)
    {
        const auto group_header_size =
            ctx_manager
                ->get(utils::get_schema_encoding_as<sbe::composite>(
                    *schema, g.dimension_type))
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
            fmt::arg("type", ctx_manager->get(g).impl_type),
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
            fmt::arg(
                "length_type",
                ctx_manager->get(*ctx_manager->get(d).length_type).public_type),
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
            fmt::arg(
                "length_type",
                ctx_manager->get(*ctx_manager->get(d).length_type).public_type),
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
        const sbe::level_members& members, const std::size_t header_size)
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

    std::string make_message_header_filler(const sbe::message& m)
    {
        const auto& header = utils::get_schema_encoding_as<sbe::composite>(
            *schema, schema->header_type);
        dependencies.emplace(header.name);

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
            fmt::arg("header_type", ctx_manager->get(header).public_type),
            fmt::arg("schema_id", schema->id),
            fmt::arg("template_id", m.id),
            fmt::arg("schema_version", schema->version),
            fmt::arg("block_length", ctx_manager->get(m).actual_block_length),
            fmt::arg(
                "num_groups_setter",
                make_num_groups_setter(header, m.members.groups.size())),
            fmt::arg(
                "num_var_data_fields_setter",
                make_num_var_data_fields_setter(
                    header, m.members.data.size())));
    }

    std::string make_groups(const std::vector<sbe::group>& groups)
    {
        std::string res;

        for(auto& g : groups)
        {
            res += make_group(g);
        }
        return res;
    }

    std::string make_alias(const sbe::message& m) const
    {
        return utils::make_alias_template(
            m.name, ctx_manager->get(m).impl_type);
    }

    std::vector<std::string>
        make_member_visit_calls(const sbe::level_members& members) const
    {
        std::vector<std::string> res;
        res.reserve(
            members.fields.size() + members.groups.size()
            + members.data.size());

        for(const auto& f : members.fields)
        {
            const auto& context = ctx_manager->get(f);
            if(context.actual_presence == field_presence::constant)
            {
                continue;
            }

            res.push_back(fmt::format(
                "v.on_field(this->{name}(c), {tag}{{}})",
                fmt::arg("name", f.name),
                fmt::arg("tag", context.tag)));
        }

        for(const auto& g : members.groups)
        {
            res.push_back(fmt::format(
                "v.on_group(this->{name}(c), c, {tag}{{}})",
                fmt::arg("name", g.name),
                fmt::arg("tag", ctx_manager->get(g).tag)));
        }

        for(const auto& d : members.data)
        {
            res.push_back(fmt::format(
                "v.on_data(this->{name}(c), {tag}{{}})",
                fmt::arg("name", d.name),
                fmt::arg("tag", ctx_manager->get(d).tag)));
        }

        return res;
    }

    std::string make_visit_children(const sbe::level_members& members) const
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

    void compile_message(const sbe::message& m)
    {
        dependencies.clear();

        const auto& header = utils::get_schema_encoding_as<sbe::composite>(
            *schema, schema->header_type);
        const auto& header_context = ctx_manager->get(header);
        auto& message_context = ctx_manager->get(m);
        const auto groups = make_groups(m.members.groups);
        const auto accessors =
            make_level_accessors(m.members, header_context.size);
        const auto& schema_name = ctx_manager->get(*schema).name;

        message_context.impl_type = fmt::format(
            "::{}::detail::messages::{}",
            schema_name,
            message_context.impl_name);

        const auto& header_type = header_context.public_type;
        const auto size_bytes_impl = make_level_size_bytes_impl(
            is_flat_level(m.members),
            get_last_member(m.members),
            header_context.size);
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
            fmt::arg("name", message_context.impl_name),
            fmt::arg("header_type", header_type),
            fmt::arg("accessors", accessors),
            fmt::arg("size_getter", size_bytes_impl),
            fmt::arg("header_filler", make_message_header_filler(m)),
            fmt::arg("groups", groups),
            fmt::arg("visit_children_impl", visit_children_impl),
            fmt::arg("tag", message_context.tag));

        message_context.public_type = fmt::format(
            "::{schema}::messages::{name}",
            fmt::arg("schema", schema_name),
            fmt::arg("name", m.name));

        const auto alias = make_alias(m);
        const auto traits = traits_gen->make_message_traits(m);
        on_message_cb(m.name, implementation, alias, dependencies, traits);
    }
};
} // namespace sbepp::sbeppc
