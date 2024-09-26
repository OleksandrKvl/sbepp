#pragma once

#include "sbepp/sbeppc/source_location.hpp"
#include <cassert>
#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/utils.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace sbepp::sbeppc
{
// TODO: is "checker" a good name? Maybe verifier/validator is better?
// performs complete SBE schema correctness check
class sbe_checker
{
public:
    void check(const sbe::message_schema& schema)
    {
        this->schema = &schema;

        validate_types();
        // validate_message_header();
        // check all messages
    }

private:
    const sbe::message_schema* schema{};
    // TODO: better names?
    enum class processing_state
    {
        in_progress,
        complete
    };

    std::unordered_map<std::string_view, processing_state> processing_states;

    static void validate_message_header_element(
        const sbe::composite& c, const std::string_view name)
    {
        // TODO: complete this function when types are checked
        const auto e = utils::find_composite_element(c, name);
        if(!e)
        {
            throw_error(
                "{}: message header composite `{}` doesn't have required `{}` "
                "element",
                c.location,
                c.name,
                name);
        }

        // TODO: should not be a constant?
        // TODO: support `ref`
        // SBE also requires underlying type to be unsigned integer but I don't
        // want to restrict, we can do a warning about it
        const auto t = std::get_if<sbe::type>(e);
        if(!t)
        {
            throw_error(
                "{}: message header composite `{}` element `{}` must be a "
                "non-array type",
                t->location,
                c.name,
                t->name);
        }

        if(!t || (t->length != 1))
        {
            throw_error(
                "{}: message header composite element `{}` must be a "
                "non-array type",
                utils::get_location(*e),
                utils::get_encoding_name(*e));
        }

        if(t->presence == field_presence::constant)
        {
            throw_error(
                "{}: message header composite element `{}` cannot be a "
                "constant",
                t->location,
                t->name);
        }

        // if(!is_unsigned_primitive_type(t->primitive_type))
        // {
        //     // warning
        // }
    }

    void validate_message_header() const
    {
        const auto enc = get_encoding(schema->header_type);
        if(!enc)
        {
            throw_error(
                "{}: message header encoding `{}` doesn't exist",
                schema->location,
                schema->header_type);
        }

        const auto c = std::get_if<sbe::composite>(enc);
        if(!c)
        {
            throw_error(
                "{}: message header encoding `{}` is not a composite",
                utils::get_location(*enc),
                schema->header_type);
        }

        validate_message_header_element(*c, "schemaId");
        validate_message_header_element(*c, "templateId");
        validate_message_header_element(*c, "version");
        validate_message_header_element(*c, "blockLength");
    }

    const sbe::encoding* get_encoding(const std::string& name) const
    {
        const auto search = schema->types.find(name);
        if(search != std::end(schema->types))
        {
            return &search->second;
        }

        return {};
    }

    // template<typename T>
    // bool has_encoding_as(const std::string& name) const
    // {
    //     const auto search = schema->types.find(name);
    //     if(search != std::end(schema->types))
    //     {
    //         const auto& value = search->second;
    //         return std::holds_alternative<T>(value);
    //     }

    //     return false;
    // }

    template<typename T>
    static bool can_be_parsed_as(const std::string_view str)
    {
        T value{};
        const auto last = str.data() + str.size();
        auto res = std::from_chars(str.data(), last, value);
        if((res.ec == std::errc{}) && (res.ptr == last))
        {
            return true;
        }

        return false;
    }

    template<typename T>
    static bool can_be_parsed_as_fp(const std::string_view str)
    {
        static std::string_view nan{"nan"};
        if(std::equal(
               std::begin(str),
               std::end(str),
               std::begin(nan),
               [](const auto lhs, const auto rhs)
               {
                   return std::tolower(lhs) == rhs;
               }))
        {
            return true;
        }
        else
        {
            return can_be_parsed_as<T>(str);
        }
    }

    static bool value_fits_into_type(
        const std::string_view value, const std::string_view type)
    {
        if(value.empty())
        {
            return false;
        }

        if(type == "char")
        {
            return can_be_parsed_as<char>(value);
        }
        else if(type == "int8")
        {
            return can_be_parsed_as<std::int8_t>(value);
        }
        else if(type == "uint8")
        {
            return can_be_parsed_as<std::uint8_t>(value);
        }
        else if(type == "int16")
        {
            return can_be_parsed_as<std::int16_t>(value);
        }
        else if(type == "uint16")
        {
            return can_be_parsed_as<std::uint16_t>(value);
        }
        else if(type == "int32")
        {
            return can_be_parsed_as<std::int32_t>(value);
        }
        else if(type == "uint32")
        {
            return can_be_parsed_as<std::uint32_t>(value);
        }
        else if(type == "int64")
        {
            return can_be_parsed_as<std::int64_t>(value);
        }
        else if(type == "uint64")
        {
            return can_be_parsed_as<std::uint64_t>(value);
        }
        else if(type == "float")
        {
            return can_be_parsed_as_fp<float>(value);
        }
        else if(type == "double")
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

        // strict: `length` should be 1 if primitive type is not "char"

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
        // else
        // {
        //     // strict: check that value fits into primitive type
        // }
    }

    void validate_encoding(const sbe::type& t)
    {
        if(t.presence == field_presence::constant)
        {
            validate_constant_value(t);
        }
        // else - nothing we validate at the moment
    }

    static void validate_valid_values(
        const std::vector<sbe::enum_valid_value>& valid_values,
        const std::string_view underlying_type)
    {
        for(const auto& value : valid_values)
        {
            // strict: SBE char has strictly defined range
            const auto has_wrong_char_value =
                ((underlying_type == "char") && (value.value.size() != 1));
            if(has_wrong_char_value
               || !value_fits_into_type(value.value, underlying_type))
            {
                throw_error(
                    "{}: value `{}` cannot be represented by type `{}`",
                    value.location,
                    value.value,
                    underlying_type);
            }
        }
    }

    static bool is_integral_type(const std::string_view type)
    {
        static const std::unordered_set<std::string_view> unsigned_types{
            "char",
            "int8",
            "uint8",
            "int16",
            "uint16",
            "int32",
            "uint32",
            "int64",
            "uint64"};
        return unsigned_types.count(type);
    }

    void validate_encoding(const sbe::enumeration& e)
    {
        std::string_view underlying_type;

        if(utils::is_primitive_type(e.type))
        {
            underlying_type = e.type;
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
            // strict: type should be non-const, non-array

            underlying_type = t->primitive_type;
        }

        // strict: SBE even requires unsigned integer or char but I see no
        // reason to forbid signed integers
        if(!is_integral_type(underlying_type))
        {
            throw_error(
                "{}: enum type should be `char` or integer, got `{}`",
                e.location,
                underlying_type);
        }

        validate_valid_values(e.valid_values, underlying_type);
    }

    static bool is_unsigned_primitive_type(const std::string_view type)
    {
        static const std::unordered_set<std::string_view> unsigned_types{
            "uint8", "uint16", "uint32", "uint64"};
        return unsigned_types.count(type);
    }

    static std::size_t
        get_underlying_size(const std::string_view underlying_type)
    {
        static const std::unordered_map<std::string_view, std::size_t> map{
            {"uint8", sizeof(std::uint8_t)},
            {"uint16", sizeof(std::uint16_t)},
            {"uint32", sizeof(std::uint32_t)},
            {"uint64", sizeof(std::uint64_t)}};

        return map.at(underlying_type);
    }

    static void validate_choice_indexes(
        const std::vector<sbe::set_choice>& choices,
        const std::string_view underlying_type)
    {
        const auto underlying_size = get_underlying_size(underlying_type);
        static constexpr auto bits_per_byte = 8;
        const auto bit_length = underlying_size * bits_per_byte - 1;

        for(const auto& choice : choices)
        {
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
        std::string_view underlying_type;

        if(utils::is_primitive_type(s.type))
        {
            underlying_type = s.type;
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
            // strict: type should be non-const, non-array

            underlying_type = t->primitive_type;
        }

        if(!is_unsigned_primitive_type(underlying_type))
        {
            throw_error("{}: underlying type must be unsigned", s.location);
        }

        validate_choice_indexes(s.choices, underlying_type);
    }

    void validate_encoding(const sbe::ref& r)
    {
        const auto enc = get_encoding(utils::to_lower(r.type));
        if(!enc)
        {
            throw_error("{}: encoding `{}` doesn't exist", r.location, r.type);
        }

        validate_public_encoding(*enc);
    }

    void validate_encoding(const sbe::composite& c)
    {
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

    void validate_public_encoding(const sbe::encoding& encoding)
    {
        const auto [it, emplaced] = processing_states.try_emplace(
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
        // cyclic refs
        // collect all type-related errors from the existing code
        // probably it's a good idea to list them
        for(const auto& [name, encoding] : schema->types)
        {
            validate_public_encoding(encoding);
        }
    }
};
} // namespace sbepp::sbeppc