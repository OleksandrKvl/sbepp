#pragma once

#include <sbepp/sbepp.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/context_manager.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <cassert>

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

private:
    const sbe::message_schema* schema{};
    context_manager* ctx_manager{};
    // TODO: better names?
    enum class processing_state
    {
        in_progress,
        complete
    };

    std::unordered_map<std::string_view, processing_state> processing_states;

    void validate_messages()
    {
        validate_message_header();
    }

    void validate_message_header_element(
        const sbe::composite& c, const std::string_view name) const
    {
        const auto element = utils::find_composite_element(c, name);
        if(!element)
        {
            throw_error(
                "{}: message header `{}` doesn't have required `{}` element",
                c.location,
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
                    "{}: message header element `{}` must be a type or a ref",
                    utils::get_location(*element),
                    name);
            }

            t = std::get_if<sbe::type>(get_encoding(r->type));
            if(!t)
            {
                throw_error(
                    "{}: message header element `{}` must refer to a type",
                    r->location,
                    name);
            }
        }

        // strict: SBE requires underlying type to be unsigned integer

        if(t->length != 1)
        {
            throw_error(
                "{}: message header element `{}` must be a non-array type",
                utils::get_location(*element),
                name);
        }

        if(t->presence == field_presence::constant)
        {
            throw_error(
                "{}: message header element `{}` cannot be a constant",
                utils::get_location(*element),
                name);
        }
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
        const auto lowered_name = utils::to_lower(name);
        const auto search = schema->types.find(lowered_name);
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
        const std::string_view value, const std::string_view primitive_type)
    {
        if(value.empty())
        {
            return false;
        }

        // TODO: is char case ever used?
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

    void validate_encoding(const sbe::type& t)
    {
        if(t.presence == field_presence::constant)
        {
            validate_constant_value(t);
        }
        // else - nothing we validate at the moment

        ctx_manager->get(t).size =
            t.length * get_primitive_type_size(t.primitive_type);
    }

    static void validate_valid_values(
        const std::vector<sbe::enum_valid_value>& valid_values,
        const std::string_view primitive_type)
    {
        for(const auto& value : valid_values)
        {
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

        validate_valid_values(e.valid_values, primitive_type);
        ctx_manager->get(e).size = get_primitive_type_size(primitive_type);
    }

    static bool is_unsigned_primitive_type(const std::string_view type)
    {
        static const std::unordered_set<std::string_view> unsigned_types{
            "uint8", "uint16", "uint32", "uint64"};
        return unsigned_types.count(type);
    }

    static void validate_choice_indexes(
        const std::vector<sbe::set_choice>& choices,
        const std::string_view primitive_type)
    {
        const auto primitive_type_size =
            get_primitive_type_size(primitive_type);
        static constexpr auto bits_per_byte = 8;
        const auto bit_length = primitive_type_size * bits_per_byte - 1;

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

        validate_choice_indexes(s.choices, primitive_type);
        ctx_manager->get(s).size = get_primitive_type_size(primitive_type);
    }

    void validate_encoding(const sbe::ref& r)
    {
        const auto enc = get_encoding(r.type);
        if(!enc)
        {
            throw_error("{}: encoding `{}` doesn't exist", r.location, r.type);
        }

        validate_public_encoding(*enc);

        std::visit(
            [this, &r](const auto& enc)
            {
                ctx_manager->get(r).size = ctx_manager->get(enc).size;
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
        ctx_manager->get(c).size = offset;
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