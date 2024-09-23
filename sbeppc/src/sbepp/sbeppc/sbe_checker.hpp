#pragma once

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

    void validate_encoding(const sbe::type& /*t*/)
    {
    }

    void validate_encoding(const sbe::enumeration& /*e*/)
    {
    }

    void validate_encoding(const sbe::set& /*s*/)
    {
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