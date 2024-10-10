// SPDX-License-Identifier: MIT
// Copyright (c) 2024, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/source_location.hpp>
#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/context_manager.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <cassert>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <cerrno>

namespace sbepp::sbeppc
{
// TODO: is "checker" a good name? Maybe verifier/validator is better?
// performs complete SBE schema correctness check
class sbe_checker
{
public:
    void check(const sbe::message_schema& schema, context_manager& ctx_manager)
    {
        this->schema = &schema;
        this->ctx_manager = &ctx_manager;

        validate_types();
        validate_messages();
    }

    static bool is_sbe_symbolic_name(const std::string_view name)
    {
        // strict: SBE also sets maximum name length to 64 but it doesn't make
        // sense to me
        if(name.empty() || std::isdigit(static_cast<unsigned char>(name[0])))
        {
            return false;
        }

        auto search = std::find_if_not(
            std::begin(name),
            std::end(name),
            [](unsigned char ch)
            {
                return std::isalnum(ch) || (ch == '_');
            });
        return (search == std::end(name));
    }

private:
    const sbe::message_schema* schema{};
    context_manager* ctx_manager{};

    enum class processing_state
    {
        in_progress,
        complete
    };
    std::unordered_map<std::string_view, processing_state>
        encoding_processing_states;
    std::unordered_set<std::string> validated_group_headers;
    std::unordered_set<std::string> validated_data_headers;

    static bool is_single_byte_type(const std::string_view type)
    {
        static const std::unordered_set<std::string_view> single_byte_types{
            "char", "int8", "uint8"};
        return single_byte_types.count(type);
    }

    void validate_data_element_type(const sbe::composite& c)
    {
        static constexpr std::string_view element_name = "varData";
        // TODO: deduplicate
        const auto element = utils::find_composite_element(c, element_name);
        if(!element)
        {
            throw_error(
                "{}: data header `{}` doesn't have required `{}` element",
                c.location,
                c.name,
                element_name);
        }

        const sbe::type* t = std::get_if<sbe::type>(element);
        if(!t)
        {
            const auto r = std::get_if<sbe::ref>(element);
            if(!r)
            {
                throw_error(
                    "{}: data header element `{}` must be a type or a ref",
                    utils::get_location(*element),
                    element_name);
            }

            t = std::get_if<sbe::type>(get_encoding(r->type));
            if(!t)
            {
                throw_error(
                    "{}: data header element `{}` must refer to a type",
                    r->location,
                    element_name);
            }
        }

        if(t->length != 0)
        {
            throw_error(
                "{}: data header element `{}` must have length equal to 0",
                utils::get_location(*element),
                element_name);
        }

        // strict: t->presence should be `required`?

        // `t->primitive_type` must be a single-byte type but it's already
        // checked during type validation
    }

    void validate_data_header(const sbe::data& d)
    {
        // multiple groups usually share the same header type, we don't need to
        // validate it more than once
        const auto lowered_name = utils::to_lower(d.type);
        if(!validated_data_headers.count(lowered_name))
        {
            const auto enc = get_encoding(d.type);
            if(!enc)
            {
                throw_error(
                    "{}: data header encoding `{}` doesn't exist",
                    d.location,
                    d.type);
            }

            const auto c = std::get_if<sbe::composite>(enc);
            if(!c)
            {
                throw_error(
                    "{}: data header encoding `{}` is not a composite",
                    utils::get_location(*enc),
                    d.type);
            }

            validate_level_header_element(*c, "data", "length");
            validate_data_element_type(*c);
            // strict: the order should be `length -> varData` and no other
            //  elements are allowed

            validated_data_headers.insert(lowered_name);
        }
    }

    void validate_field_offset(const sbe::field& f, offset_t& current_offset)
    {
        auto& context = ctx_manager->get(f);

        if(f.offset)
        {
            if(f.offset < current_offset)
            {
                throw_error(
                    "{}: custom offset ({}) is less than minimum "
                    "possible ({})",
                    f.location,
                    *f.offset,
                    current_offset);
            }
            context.level_offset = *f.offset;
            current_offset = *f.offset;
        }
        else
        {
            context.level_offset = current_offset;
        }

        const auto enc_size = context.size;
        current_offset += enc_size;
    }

    static field_presence
        get_actual_presence(const sbe::field& f, const sbe::encoding& enc)
    {
        // ideally, field presence must match encoding presence but in
        // reality some schemas break this rule and, for example,
        // specify enum fields as optional. Here, we figure out valid
        // presence silently, without error or warning message about
        // their mismatch.
        return std::visit(
            utils::overloaded{
                [](const sbe::type& t)
                {
                    return t.presence;
                },
                [&f](const sbe::composite&)
                {
                    return f.presence;
                },
                [&f](const sbe::enumeration&)
                {
                    // enum field can be either `constant`, with value
                    // provided in `valueRef`, or `required`, but not
                    // `optional`
                    if(f.presence == field_presence::optional)
                    {
                        return field_presence::required;
                    }
                    return f.presence;
                },
                [](const sbe::set&)
                {
                    return field_presence::required;
                }},
            enc);
    }

    // TODO: deduplicate
    void validate_value_ref2(const sbe::field& f)
    {
        if(!f.value_ref)
        {
            throw_error("{}: field constant must have `valueRef`", f.location);
        }

        const auto& value_ref = *f.value_ref;
        const auto parsed = utils::parse_value_ref(value_ref);
        if(parsed.enum_name.empty() || parsed.enumerator.empty())
        {
            throw_error(
                "{}: `{}` is not a valid `valueRef`", f.location, value_ref);
        }

        const auto enc = get_encoding(parsed.enum_name);
        if(!enc)
        {
            throw_error(
                "{}: encoding `{}` doesn't exist",
                f.location,
                parsed.enum_name);
        }

        const auto e = std::get_if<sbe::enumeration>(enc);
        if(!e)
        {
            throw_error(
                "{}: encoding `{}` is not an enum",
                f.location,
                parsed.enum_name);
        }

        const auto& values = e->valid_values;
        const auto search = std::find_if(
            std::begin(values),
            std::end(values),
            [&parsed](const auto& value)
            {
                return (value.name == parsed.enumerator);
            });
        if(search == std::end(values))
        {
            throw_error(
                "{}: enum `{}` doesn't have valid value `{}`",
                f.location,
                parsed.enum_name,
                parsed.enumerator);
        }

        if(!value_fits_into_type(search->value, f.type))
        {
            throw_error(
                "{}: valueRef `{}` ({}) cannot be represented by type `{}`",
                f.location,
                value_ref,
                search->value,
                f.type);
        }
    }

    // TODO: deduplicate
    void validate_value_ref3(const sbe::field& f)
    {
        if(!f.value_ref)
        {
            throw_error("{}: field constant must have `valueRef`", f.location);
        }

        const auto& value_ref = *f.value_ref;
        const auto parsed = utils::parse_value_ref(value_ref);
        if(parsed.enum_name.empty() || parsed.enumerator.empty())
        {
            throw_error(
                "{}: `{}` is not a valid `valueRef`", f.location, value_ref);
        }

        const auto enc = get_encoding(parsed.enum_name);
        if(!enc)
        {
            throw_error(
                "{}: encoding `{}` doesn't exist",
                f.location,
                parsed.enum_name);
        }

        const auto e = std::get_if<sbe::enumeration>(enc);
        if(!e)
        {
            throw_error(
                "{}: encoding `{}` is not an enum",
                f.location,
                parsed.enum_name);
        }

        const auto& values = e->valid_values;
        const auto search = std::find_if(
            std::begin(values),
            std::end(values),
            [&parsed](const auto& value)
            {
                return (value.name == parsed.enumerator);
            });
        if(search == std::end(values))
        {
            throw_error(
                "{}: enum `{}` doesn't have valid value `{}`",
                f.location,
                parsed.enum_name,
                parsed.enumerator);
        }

        const auto lowered_field_type = utils::to_lower(f.type);
        const auto lowered_enum_type = utils::to_lower(parsed.enum_name);
        if(lowered_field_type != lowered_enum_type)
        {
            throw_error(
                "{}: enum constant type `{}` should match field type `{}`",
                f.location,
                parsed.enum_name,
                f.type);
        }
    }

    void validate_constant_field(const sbe::field& f)
    {
        if(utils::is_primitive_type(f.type))
        {
            validate_value_ref2(f);
        }
        else
        {
            const auto enc = get_encoding(f.type);
            assert(enc);
            std::visit(
                utils::overloaded{
                    [](const sbe::type&)
                    {
                        // nothing to validate because types are already
                        // validated
                    },
                    [](const sbe::set&)
                    {
                        // not possible because sets can't be constant
                        assert(false);
                    },
                    [&f](const sbe::composite&)
                    {
                        throw_error(
                            "{}: composite field can't be a constant",
                            f.location);
                    },
                    [&f, this](const sbe::enumeration&)
                    {
                        validate_value_ref3(f);
                    }},
                *enc);
        }
    }

    template<typename MessageOrGroup>
    void validate_block_length(
        const MessageOrGroup& level, const block_length_t actual_block_length)
    {
        if(level.block_length)
        {
            if(*level.block_length < actual_block_length)
            {
                throw_error(
                    "{}: custom `blockLength` ({}) is less than minimum "
                    "possible "
                    "({})",
                    level.location,
                    *level.block_length,
                    actual_block_length);
            }
            ctx_manager->get(level).actual_block_length = *level.block_length;
        }
        else
        {
            ctx_manager->get(level).actual_block_length = actual_block_length;
        }
    }

    template<typename MessageOrGroup>
    void validate_members(const MessageOrGroup& level)
    {
        const auto& members = level.members;
        offset_t offset{};

        for(const auto& f : members.fields)
        {
            validate_name(f);
            auto& context = ctx_manager->create(f);
            field_presence actual_presence{};
            if(!utils::is_primitive_type(f.type))
            {
                const auto enc = get_encoding(f.type);
                if(!enc)
                {
                    throw_error(
                        "{}: field type `{}` doesn't exist",
                        f.location,
                        f.type);
                }

                // TODO: refactor?
                context.size = std::visit(
                    [this](const auto& type)
                    {
                        return ctx_manager->get(type).size;
                    },
                    *enc);
                actual_presence = get_actual_presence(f, *enc);
            }
            else
            {
                context.size = get_primitive_type_size(f.type);
                actual_presence = f.presence;
            }

            context.actual_presence = actual_presence;
            if(actual_presence == field_presence::constant)
            {
                validate_constant_field(f);
            }
            // else
            // {
            //     // strict: warn about optional composite
            // }

            validate_field_offset(f, offset);
        }

        // at this point `offset` is essentially a minimal blockLength value
        validate_block_length(level, offset);

        for(const auto& g : members.groups)
        {
            validate_name(g);
            ctx_manager->create(g);
            validate_group_header(g);
            validate_members(g);
        }

        for(const auto& d : members.data)
        {
            validate_name(d);
            validate_data_header(d);
        }
    }

    void validate_message(const sbe::message& m)
    {
        validate_name(m);
        ctx_manager->create(m);
        validate_members(m);
    }

    void validate_messages()
    {
        validate_message_header();

        for(const auto& m : schema->messages)
        {
            validate_message(m);
        }
    }

    void validate_level_header_element(
        const sbe::composite& c,
        const std::string_view level_name,
        const std::string_view name) const
    {
        const auto element = utils::find_composite_element(c, name);
        if(!element)
        {
            throw_error(
                "{}: {} header `{}` doesn't have required `{}` element",
                c.location,
                level_name,
                c.name,
                name);
        }

        const sbe::type* t = std::get_if<sbe::type>(element);
        if(!t)
        {
            const auto r = std::get_if<sbe::ref>(element);
            if(!r)
            {
                throw_error(
                    "{}: {} header element `{}` must be a type or a ref",
                    utils::get_location(*element),
                    level_name,
                    name);
            }

            t = std::get_if<sbe::type>(get_encoding(r->type));
            if(!t)
            {
                throw_error(
                    "{}: {} header element `{}` must refer to a type",
                    r->location,
                    level_name,
                    name);
            }
        }

        // strict: SBE requires underlying type to be unsigned integer

        if(t->length != 1)
        {
            throw_error(
                "{}: {} header element `{}` must be a non-array type",
                utils::get_location(*element),
                level_name,
                name);
        }

        if(t->presence == field_presence::constant)
        {
            throw_error(
                "{}: {} header element `{}` cannot be a constant",
                utils::get_location(*element),
                level_name,
                name);
        }
    }

    void validate_level_header(
        const std::string_view level_name,
        const source_location& level_location,
        const std::string_view header_type,
        const std::vector<std::string_view>& required_fields) const
    {
        const auto enc = get_encoding(header_type);
        if(!enc)
        {
            throw_error(
                "{}: {} header encoding `{}` doesn't exist",
                level_location,
                level_name,
                header_type);
        }

        const auto c = std::get_if<sbe::composite>(enc);
        if(!c)
        {
            throw_error(
                "{}: {} header encoding `{}` is not a composite",
                utils::get_location(*enc),
                level_name,
                header_type);
        }

        for(const auto& field : required_fields)
        {
            validate_level_header_element(*c, level_name, field);
        }
    }

    void validate_message_header() const
    {
        validate_level_header(
            "message",
            schema->location,
            schema->header_type,
            {"schemaId", "templateId", "version", "blockLength"});
        // strict: validate optional `numGroups` and `numVarDataFields`
    }

    void validate_group_header(const sbe::group& g)
    {
        // multiple groups usually share the same header type, we don't need to
        // validate it more than once
        const auto lowered_name = utils::to_lower(g.dimension_type);
        if(!validated_group_headers.count(lowered_name))
        {
            validate_level_header(
                "group",
                g.location,
                g.dimension_type,
                {"numInGroup", "blockLength"});
            // strict: validate optional `numGroups` and `numVarDataFields`

            validated_group_headers.insert(lowered_name);
        }
    }

    const sbe::encoding* get_encoding(std::string_view name) const
    {
        const auto lowered_name = utils::to_lower(name);
        const auto search = schema->types.find(lowered_name);
        if(search != std::end(schema->types))
        {
            return &search->second;
        }

        return {};
    }

    template<typename T>
    static bool can_be_parsed_as(const std::string_view str)
    {
        return utils::string_to_number<T>(str).has_value();
    }

    template<typename T>
    static bool can_be_parsed_as_fp(const std::string_view str)
    {
        // `std::from_chars` for FP types support is poor over the compilers so
        // we have to use custom implementation. SBE is not clear about FP
        // string requirements so XML rules are used.

        if(str.empty())
        {
            return false;
        }

        // XML doesn't allow leading spaces
        if(std::isspace(str.front()))
        {
            return false;
        }

        auto signless_value = str;
        bool has_sign{};
        if((str.front() == '+') || (str.front() == '-'))
        {
            has_sign = true;
            signless_value = str.substr(1);
        }

        if(signless_value.size() > 1)
        {
            // XML doesn't allow hex FP literals
            if((signless_value[0] == '0')
               && ((signless_value[1] == 'x') || (signless_value[1] == 'X')))
            {
                return false;
            }

            // XML allows only `NaN` and `INF`, case-sensitive
            if(std::isalpha(signless_value.front()))
            {
                // XML doesn't allow `NaN` with a sign
                const auto has_signless_nan =
                    ((signless_value == "NaN") && (!has_sign));
                if(!has_signless_nan && (signless_value != "INF"))
                {
                    return false;
                }
            }
        }

        // what's left should effectively be one of:
        //  - "+/-INF" or "NaN"
        //  - non-hexadecimal FP literal with optional "e/E" exponent part
        // anything else should be detected by `std::strtof/d` as error

        char* last_parsed{};
        errno = 0;
        if constexpr(std::is_same_v<T, float>)
        {
            std::strtof(str.data(), &last_parsed);
        }
        else
        {
            std::strtod(str.data(), &last_parsed);
        }

        // XML doesn't allow trailing spaces or any other garbage at the end
        const auto fully_parsed = (last_parsed == str.data() + str.size());
        if((errno == ERANGE) || !fully_parsed)
        {
            return false;
        }

        return true;
    }

    static bool value_fits_into_type(
        const std::string_view value, const std::string_view primitive_type)
    {
        if(value.empty())
        {
            return false;
        }

        if(primitive_type == "char")
        {
            return can_be_parsed_as<char>(value);
        }
        else if(primitive_type == "int8")
        {
            return can_be_parsed_as<std::int8_t>(value);
        }
        else if(primitive_type == "uint8")
        {
            return can_be_parsed_as<std::uint8_t>(value);
        }
        else if(primitive_type == "int16")
        {
            return can_be_parsed_as<std::int16_t>(value);
        }
        else if(primitive_type == "uint16")
        {
            return can_be_parsed_as<std::uint16_t>(value);
        }
        else if(primitive_type == "int32")
        {
            return can_be_parsed_as<std::int32_t>(value);
        }
        else if(primitive_type == "uint32")
        {
            return can_be_parsed_as<std::uint32_t>(value);
        }
        else if(primitive_type == "int64")
        {
            return can_be_parsed_as<std::int64_t>(value);
        }
        else if(primitive_type == "uint64")
        {
            return can_be_parsed_as<std::uint64_t>(value);
        }
        else if(primitive_type == "float")
        {
            return can_be_parsed_as_fp<float>(value);
        }
        else if(primitive_type == "double")
        {
            return can_be_parsed_as_fp<double>(value);
        }

        assert(false && "Wrong primitive type");
        return false;
    }

    void validate_value_ref(const sbe::type& t)
    {
        const auto& value_ref = *t.value_ref;
        const auto parsed = utils::parse_value_ref(value_ref);
        if(parsed.enum_name.empty() || parsed.enumerator.empty())
        {
            throw_error(
                "{}: `{}` is not a valid valueRef", t.location, value_ref);
        }

        const auto enc = get_encoding(parsed.enum_name);
        if(!enc)
        {
            throw_error(
                "{}: encoding `{}` doesn't exist",
                t.location,
                parsed.enum_name);
        }

        const auto e = std::get_if<sbe::enumeration>(enc);
        if(!e)
        {
            throw_error(
                "{}: encoding `{}` is not an enum",
                t.location,
                parsed.enum_name);
        }

        const auto& values = e->valid_values;
        const auto search = std::find_if(
            std::begin(values),
            std::end(values),
            [&parsed](const auto& value)
            {
                return (value.name == parsed.enumerator);
            });
        if(search == std::end(values))
        {
            throw_error(
                "{}: enum `{}` doesn't have valid value `{}`",
                t.location,
                parsed.enum_name,
                parsed.enumerator);
        }

        if(!value_fits_into_type(search->value, t.primitive_type))
        {
            throw_error(
                "{}: valueRef `{}` ({}) cannot be represented by type `{}`",
                t.location,
                value_ref,
                search->value,
                t.primitive_type);
        }
    }

    void validate_constant_value(const sbe::type& t)
    {
        assert(t.value_ref || t.constant_value);

        if(t.value_ref)
        {
            validate_value_ref(t);
        }
        else if(t.primitive_type == "char")
        {
            const auto value_length = t.constant_value->length();
            if(t.length < value_length)
            {
                throw_error(
                    "{}: constant length ({}) is greater than `length` "
                    "attribute ({})",
                    t.location,
                    value_length,
                    t.length);
            }
        }
        else
        {
            if(!value_fits_into_type(*t.constant_value, t.primitive_type))
            {
                throw_error(
                    "{}: value `{}` cannot be represented by type `{}`",
                    t.location,
                    *t.constant_value,
                    t.primitive_type);
            }
        }

        if((t.value_ref || (t.primitive_type != "char")) && (t.length != 1))
        {
            throw_error(
                "{}: non-char constant length must be equal to 1, got `{}`",
                t.location,
                t.length);
        }
    }

    static std::size_t get_primitive_type_size(const std::string_view type)
    {
        static const std::unordered_map<std::string_view, std::size_t> map{
            {"char", sizeof(char)},
            {"int8", sizeof(std::int8_t)},
            {"int16", sizeof(std::int16_t)},
            {"int32", sizeof(std::int32_t)},
            {"int64", sizeof(std::int64_t)},
            {"uint8", sizeof(std::uint8_t)},
            {"uint16", sizeof(std::uint16_t)},
            {"uint32", sizeof(std::uint32_t)},
            {"uint64", sizeof(std::uint64_t)},
            {"float", sizeof(float)},
            {"double", sizeof(double)}};

        return map.at(type);
    }

    static void validate_optional_value(
        const std::optional<std::string>& value,
        const std::string_view primitive_type,
        const source_location& location)
    {
        if(value && !value_fits_into_type(*value, primitive_type))
        {
            throw_error(
                "{}: value `{}` cannot be represented by type `{}`",
                location,
                *value,
                primitive_type);
        }
    }

    void validate_encoding(const sbe::type& t)
    {
        validate_name(t);

        if(t.presence == field_presence::constant)
        {
            validate_constant_value(t);
        }
        else
        {
            if((t.length != 1) && !is_single_byte_type(t.primitive_type))
            {
                throw_error(
                    "{}: arrays must have a single-byte type", t.location);
            }

            validate_optional_value(t.min_value, t.primitive_type, t.location);
            validate_optional_value(t.max_value, t.primitive_type, t.location);
            // strict: min_value should be less than max_value

            if(t.presence == field_presence::optional)
            {
                validate_optional_value(
                    t.null_value, t.primitive_type, t.location);
            }
        }

        ctx_manager->create(t).size =
            t.length * get_primitive_type_size(t.primitive_type);
    }

    void validate_valid_values(
        const std::vector<sbe::enum_valid_value>& valid_values,
        const std::string_view primitive_type) const
    {
        for(const auto& value : valid_values)
        {
            validate_name(value);
            // strict: SBE char has strictly defined range
            const auto has_wrong_char_value =
                ((primitive_type == "char") && (value.value.size() != 1));
            if(has_wrong_char_value
               || !value_fits_into_type(value.value, primitive_type))
            {
                throw_error(
                    "{}: value `{}` cannot be represented by type `{}`",
                    value.location,
                    value.value,
                    primitive_type);
            }
        }
    }

    static bool is_integral_type(const std::string_view type)
    {
        static const std::unordered_set<std::string_view> integral_types{
            "char",
            "int8",
            "uint8",
            "int16",
            "uint16",
            "int32",
            "uint32",
            "int64",
            "uint64"};
        return integral_types.count(type);
    }

    void validate_encoding(const sbe::enumeration& e)
    {
        validate_name(e);
        std::string_view primitive_type;

        if(utils::is_primitive_type(e.type))
        {
            primitive_type = e.type;
        }
        else
        {
            const auto enc = get_encoding(e.type);
            if(!enc)
            {
                throw_error(
                    "{}: encoding `{}` doesn't exist", e.location, e.type);
            }

            const auto t = std::get_if<sbe::type>(enc);
            if(!t)
            {
                throw_error(
                    "{}: encoding `{}` is not a type", e.location, e.type);
            }
            // strict: type should be non-const

            if(t->length != 1)
            {
                throw_error(
                    "{}: encoding type `{}` must have length equal to 1, got "
                    "`{}`",
                    e.location,
                    e.type,
                    t->length);
            }

            primitive_type = t->primitive_type;
        }

        // strict: SBE even requires unsigned integer or char but I see no
        // reason to forbid signed integers
        if(!is_integral_type(primitive_type))
        {
            throw_error(
                "{}: enum type should be `char` or integer, got `{}`",
                e.location,
                primitive_type);
        }

        auto& context = ctx_manager->create(e);
        context.primitive_type = primitive_type;
        validate_valid_values(e.valid_values, primitive_type);
        context.size = get_primitive_type_size(primitive_type);
    }

    static bool is_unsigned_primitive_type(const std::string_view type)
    {
        static const std::unordered_set<std::string_view> unsigned_types{
            "uint8", "uint16", "uint32", "uint64"};
        return unsigned_types.count(type);
    }

    void validate_choices(
        const std::vector<sbe::set_choice>& choices,
        const std::string_view primitive_type) const
    {
        const auto primitive_type_size =
            get_primitive_type_size(primitive_type);
        static constexpr auto bits_per_byte = 8;
        const auto bit_length = (primitive_type_size * bits_per_byte) - 1;

        for(const auto& choice : choices)
        {
            validate_name(choice);

            if(choice.value > bit_length)
            {
                throw_error(
                    "{}: choice index `{}` is out of valid range ([0, {}])",
                    choice.location,
                    choice.value,
                    bit_length);
            }
        }
    }

    void validate_encoding(const sbe::set& s)
    {
        validate_name(s);
        std::string_view primitive_type;

        if(utils::is_primitive_type(s.type))
        {
            primitive_type = s.type;
        }
        else
        {
            const auto enc = get_encoding(s.type);
            if(!enc)
            {
                throw_error(
                    "{}: encoding `{}` doesn't exist", s.location, s.type);
            }

            const auto t = std::get_if<sbe::type>(enc);
            if(!t)
            {
                throw_error(
                    "{}: encoding `{}` is not a type", s.location, s.type);
            }
            // strict: type should be non-const

            if(t->length != 1)
            {
                throw_error(
                    "{}: encoding type `{}` must have length equal to 1, got "
                    "`{}`",
                    s.location,
                    s.type,
                    t->length);
            }

            primitive_type = t->primitive_type;
        }

        if(!is_unsigned_primitive_type(primitive_type))
        {
            throw_error("{}: underlying type must be unsigned", s.location);
        }

        auto& context = ctx_manager->create(s);
        context.primitive_type = primitive_type;
        validate_choices(s.choices, primitive_type);
        context.size = get_primitive_type_size(primitive_type);
    }

    void validate_encoding(const sbe::ref& r)
    {
        validate_name(r);

        const auto enc = get_encoding(r.type);
        if(!enc)
        {
            throw_error("{}: encoding `{}` doesn't exist", r.location, r.type);
        }

        validate_public_encoding(*enc);

        std::visit(
            [this, &r](const auto& enc)
            {
                ctx_manager->create(r).size = ctx_manager->get(enc).size;
            },
            *enc);
    }

    template<typename T>
    static bool is_constant_composite_element(const T& element)
    {
        if constexpr(std::is_same_v<T, sbe::type>)
        {
            return element.presence == field_presence::constant;
        }
        else
        {
            return false;
        }
    }

    bool is_constant_composite_element(const sbe::ref& r)
    {
        const auto enc = get_encoding(r.type);
        assert(enc);

        return std::visit(
            [](const auto& enc)
            {
                return is_constant_composite_element(enc);
            },
            *enc);
    }

    template<typename T>
    void validate_element_offset(const T& element, offset_t& current_offset)
    {
        if(is_constant_composite_element(element))
        {
            return;
        }

        auto& context = ctx_manager->get(element);

        if(element.offset)
        {
            if(element.offset < current_offset)
            {
                throw_error(
                    "{}: custom offset ({}) is less than minimum "
                    "possible ({})",
                    element.location,
                    *element.offset,
                    current_offset);
            }
            context.offset_in_composite = *element.offset;
            current_offset = *element.offset;
        }
        else
        {
            context.offset_in_composite = current_offset;
        }

        const auto enc_size = context.size;
        current_offset += enc_size;
    }

    void validate_encoding(const sbe::composite& c)
    {
        validate_name(c);

        offset_t offset{};

        for(const auto& element : c.elements)
        {
            std::visit(
                [this, &offset](const auto& enc)
                {
                    validate_encoding(enc);
                    validate_element_offset(enc, offset);
                },
                element);
        }

        // `offset` points past the last element, it's effectively the size
        ctx_manager->create(c).size = offset;
    }

    void validate_public_encoding(const sbe::encoding& encoding)
    {
        const auto [it, emplaced] = encoding_processing_states.try_emplace(
            utils::get_encoding_name(encoding), processing_state::in_progress);
        if(emplaced)
        {
            std::visit(
                [this](const auto& enc)
                {
                    validate_encoding(enc);
                },
                encoding);
            it->second = processing_state::complete;
        }
        else if(it->second != processing_state::complete)
        {
            throw_error(
                "{}: cyclic reference detected while processing encoding `{}`",
                utils::get_location(encoding),
                utils::get_encoding_name(encoding));
        }
    }

    void validate_types()
    {
        for(const auto& [name, encoding] : schema->types)
        {
            validate_public_encoding(encoding);
        }
    }

    template<typename T>
    void validate_name(const T& entity) const
    {
        if(!is_sbe_symbolic_name(entity.name))
        {
            throw_error(
                "{}: `{}` is not a valid SBE name",
                entity.location,
                entity.name);
        }
    }
};
} // namespace sbepp::sbeppc