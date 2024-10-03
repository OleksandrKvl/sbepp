// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/source_location.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/location_manager.hpp>
#include <sbepp/sbeppc/ireporter.hpp>
#include <sbepp/sbeppc/ifs_provider.hpp>
#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/unique_set.hpp>
#include <sbepp/sbepp.hpp>

#include <pugixml.hpp>

#include <string>
#include <string_view>
#include <optional>
#include <cstring>
#include <vector>

namespace sbepp::sbeppc
{
// SBE schema parser, performs some basic XML checks but not related to C++ (or
// any other language) codegen
class schema_parser
{
public:
    schema_parser(
        const std::string& path, ireporter& reporter, ifs_provider& fs_provider)
        : reporter{&reporter}, fs_provider{&fs_provider}
    {
        const auto file_data = this->fs_provider->read_file(path);
        locations = location_manager{path, file_data};
        parse_xml(file_data);
    }

    void parse_schema()
    {
        const auto message_schema_node =
            get_message_schema_node(xml_doc.root());
        message_schema = parse_message_schema(message_schema_node);
        parse_schema_content(message_schema_node);
    }

    void parse_schema_content()
    {
        parse_schema_content(xml_doc.root());
    }

    const sbe::message_schema& get_message_schema() const
    {
        return message_schema;
    }

private:
    ireporter* reporter;
    ifs_provider* fs_provider;
    location_manager locations;
    pugi::xml_document xml_doc;
    sbe::message_schema message_schema;
    unique_set<std::string> unique_message_names;
    unique_set<message_id_t> unique_message_ids;

    enum class ordered_member_type
    {
        field,
        group,
        data
    };

    pugi::xml_attribute get_required_attribute(
        const pugi::xml_node root, const std::string_view attribute_name) const
    {
        auto attribute = root.attribute(attribute_name.data());
        if(!attribute)
        {
            throw_error(
                "{}: required attribute `{}` doesn't exist",
                locations.find(root.offset_debug()),
                attribute_name);
        }
        return attribute;
    }

    void add_unique_type(sbe::encoding e)
    {
        // SBE requires type lookup to be case insensitive
        auto name = utils::to_lower(utils::get_encoding_name(e));
        const auto [it, inserted] =
            message_schema.types.try_emplace(std::move(name), std::move(e));

        if(!inserted)
        {
            throw_error(
                "{}: encoding `{}` already exists at {}",
                utils::get_location(e),
                utils::get_encoding_name(e),
                utils::get_location(it->second));
        }
    }

    void
        merge_types(const std::unordered_map<std::string, sbe::encoding>& types)
    {
        for(const auto& [name, enc] : types)
        {
            add_unique_type(enc);
        }
    }

    void add_unique_message(sbe::message m)
    {
        unique_message_names.add_or_throw(
            m.name,
            "{}: message with name `{}` already exists",
            m.location,
            m.name);
        unique_message_ids.add_or_throw(
            m.id, "{}: message with id `{}` already exists", m.location, m.id);

        message_schema.messages.push_back(std::move(m));
    }

    void merge_messages(const std::vector<sbe::message>& messages)
    {
        for(const auto& m : messages)
        {
            add_unique_message(m);
        }
    }

    void parse_include(const pugi::xml_node root)
    {
        const auto path = get_required_non_empty_string(root, "href");
        auto parser = schema_parser{path, *reporter, *fs_provider};
        parser.parse_schema_content();

        const auto& schema = parser.get_message_schema();
        merge_types(schema.types);
        merge_messages(schema.messages);
    }

    std::string get_required_non_empty_string(
        const pugi::xml_node root, const std::string_view name) const
    {
        std::string value =
            get_required_attribute(root, name.data()).as_string();
        if(value.empty())
        {
            throw_error(
                "{}: `{}` attribute is empty",
                locations.find(root.offset_debug()),
                name);
        }

        return value;
    }

    std::string get_required_name(const pugi::xml_node root)
    {
        auto name = get_required_non_empty_string(root, "name");

        const auto location = locations.find(root.offset_debug());
        // TODO: move this check inside `sbe_checker`
        if(!utils::is_sbe_symbolic_name(name))
        {
            throw_error("{}: `{}` is not a valid SBE name", location, name);
        }

        return name;
    }

    static std::string get_description(const pugi::xml_node root)
    {
        return root.attribute("description").as_string();
    }

    static std::optional<std::string> get_optional_string_attribute(
        const pugi::xml_node root, const std::string_view attribute)
    {
        const auto value = root.attribute(attribute.data());
        if(value)
        {
            return value.as_string();
        }
        return {};
    }

    field_presence get_presence(const pugi::xml_node root)
    {
        const auto value = get_optional_string_attribute(root, "presence")
                               .value_or("required");
        if(value == "required")
        {
            return field_presence::required;
        }
        else if(value == "optional")
        {
            return field_presence::optional;
        }
        else if(value == "constant")
        {
            return field_presence::constant;
        }

        throw_error(
            "{}: wrong presence token `{}`",
            locations.find(root.offset_debug()),
            value);
    }

    std::optional<offset_t> get_offset(const pugi::xml_node root) const
    {
        return get_optional_numeric_attribute<offset_t>(root, "offset");
    }

    std::string get_primitive_type(const pugi::xml_node root)
    {
        const auto type = get_required_non_empty_string(root, "primitiveType");
        if(!utils::is_primitive_type(type))
        {
            throw_error(
                "{}: primitiveType `{}` is not a valid primitive type",
                locations.find(root.offset_debug()),
                type);
        }
        return type;
    }

    std::optional<version_t>
        get_deprecated_since(const pugi::xml_node root) const
    {
        return get_optional_numeric_attribute<version_t>(root, "deprecated");
    }

    version_t get_added_since(const pugi::xml_node root) const
    {
        return get_optional_numeric_attribute<version_t>(root, "sinceVersion")
            .value_or(0);
    }

    static std::optional<std::string>
        get_optional_node_content(const pugi::xml_node root)
    {
        const auto content = root.text();
        if(content.empty())
        {
            return {};
        }

        return content.get();
    }

    length_t get_type_length(const pugi::xml_node root) const
    {
        return get_optional_numeric_attribute<length_t>(root, "length")
            .value_or(1);
    }

    sbe::type parse_type_encoding(const pugi::xml_node root)
    {
        sbe::type t{};
        t.location = locations.find(root.offset_debug());
        t.name = get_required_name(root);
        t.description = get_description(root);
        t.presence = get_presence(root);
        t.null_value = get_optional_string_attribute(root, "nullValue");

        if(t.null_value && (t.presence != field_presence::optional))
        {
            reporter->warning(
                "{}: nullValue is ignored for type `{}` because `presence` is "
                "not `optional`",
                locations.find(root.offset_debug()),
                t.name);
        }

        t.min_value = get_optional_string_attribute(root, "minValue");
        t.max_value = get_optional_string_attribute(root, "maxValue");
        t.length = get_type_length(root);
        t.offset = get_offset(root);
        t.primitive_type = get_primitive_type(root);
        t.semantic_type = get_semantic_type(root);
        t.added_since = get_added_since(root);
        t.deprecated_since = get_deprecated_since(root);
        validate_versions(t);
        t.character_encoding =
            get_optional_string_attribute(root, "characterEncoding");

        if(t.presence == field_presence::constant)
        {
            t.value_ref = get_optional_string_attribute(root, "valueRef");
            t.constant_value = get_optional_node_content(root);
            if(t.value_ref.has_value() == t.constant_value.has_value())
            {
                throw_error(
                    "{}: either `valueRef` or value must be provided for "
                    "constant `{}`",
                    locations.find(root.offset_debug()),
                    t.name);
            }

            if((t.primitive_type == "char") && root.attribute("length").empty())
            {
                t.length = t.constant_value->size();
            }
        }

        return t;
    }

    static std::string get_semantic_type(const pugi::xml_node root)
    {
        return root.attribute("semanticType").as_string();
    }

    std::string get_required_node_content(const pugi::xml_node root)
    {
        const auto content = root.text();
        if(content.empty())
        {
            throw_error(
                "{}: required node content is empty",
                locations.find(root.offset_debug()));
        }

        return content.get();
    }

    sbe::enum_valid_value get_enum_valid_value(const pugi::xml_node root)
    {
        sbe::enum_valid_value value{};

        value.location = locations.find(root.offset_debug());
        value.name = get_required_name(root);
        value.description = get_description(root);
        value.added_since = get_added_since(root);
        value.deprecated_since = get_deprecated_since(root);
        validate_versions(value);
        value.value = get_required_node_content(root);

        return value;
    }

    std::vector<sbe::enum_valid_value>
        get_enum_valid_values(const pugi::xml_node root)
    {
        std::vector<sbe::enum_valid_value> values;
        unique_set<std::string> unique_value_names;

        for(const auto child : root.children("validValue"))
        {
            auto valid_value = get_enum_valid_value(child);
            unique_value_names.add_or_throw(
                valid_value.name,
                "{}: duplicate validValue name: `{}`",
                valid_value.location,
                valid_value.name);

            values.push_back(std::move(valid_value));
        }

        return values;
    }

    sbe::enumeration parse_enum_encoding(const pugi::xml_node root)
    {
        sbe::enumeration e{};
        e.location = locations.find(root.offset_debug());
        e.name = get_required_name(root);
        e.description = get_description(root);
        e.type = get_required_non_empty_string(root, "encodingType");
        e.added_since = get_added_since(root);
        e.deprecated_since = get_deprecated_since(root);
        validate_versions(e);
        e.offset = get_offset(root);
        e.valid_values = get_enum_valid_values(root);

        return e;
    }

    choice_index_t get_choice_index(const pugi::xml_node root)
    {
        const auto content = get_required_node_content(root);

        return utils::string_to_number_or_throw<choice_index_t>(
            content,
            "{}: node's value `{}` doesn't represent choice_index_t",
            locations.find(root.offset_debug()),
            content);
    }

    sbe::set_choice get_choice(const pugi::xml_node root)
    {
        sbe::set_choice choice{};

        choice.location = locations.find(root.offset_debug());
        choice.name = get_required_name(root);
        choice.description = get_description(root);
        choice.added_since = get_added_since(root);
        choice.deprecated_since = get_deprecated_since(root);
        validate_versions(choice);
        choice.value = get_choice_index(root);

        return choice;
    }

    std::vector<sbe::set_choice> get_set_choices(const pugi::xml_node root)
    {
        std::vector<sbe::set_choice> choices;
        unique_set<std::string> unique_choice_names;

        for(const auto child : root.children("choice"))
        {
            auto choice = get_choice(child);
            unique_choice_names.add_or_throw(
                choice.name,
                "{}: duplicate choice name: `{}`",
                choice.location,
                choice.name);
            choices.push_back(std::move(choice));
        }

        return choices;
    }

    sbe::set parse_set_encoding(const pugi::xml_node root)
    {
        sbe::set s{};

        s.location = locations.find(root.offset_debug());
        s.name = get_required_name(root);
        s.description = get_description(root);
        s.type = get_required_non_empty_string(root, "encodingType");
        s.added_since = get_added_since(root);
        s.deprecated_since = get_deprecated_since(root);
        validate_versions(s);
        s.offset = get_offset(root);
        s.choices = get_set_choices(root);

        return s;
    }

    sbe::ref parse_ref_encoding(const pugi::xml_node root)
    {
        sbe::ref r{};

        r.location = locations.find(root.offset_debug());
        r.name = get_required_name(root);
        r.type = get_required_non_empty_string(root, "type");
        r.offset = get_offset(root);
        r.added_since = get_added_since(root);
        r.deprecated_since = get_deprecated_since(root);
        validate_versions(r);

        return r;
    }

    std::vector<sbe::composite_element>
        parse_composite_elements(const pugi::xml_node root)
    {
        std::vector<sbe::composite_element> elements;
        unique_set<std::string> unique_element_names;

        for(const auto element_node : root)
        {
            sbe::composite_element element;

            if(is_node_name_equal_to(element_node.name(), "type"))
            {
                element = parse_type_encoding(element_node);
            }
            else if(is_node_name_equal_to(element_node.name(), "composite"))
            {
                element = parse_composite_encoding(element_node);
            }
            else if(is_node_name_equal_to(element_node.name(), "enum"))
            {
                element = parse_enum_encoding(element_node);
            }
            else if(is_node_name_equal_to(element_node.name(), "set"))
            {
                element = parse_set_encoding(element_node);
            }
            else if(is_node_name_equal_to(element_node.name(), "ref"))
            {
                element = parse_ref_encoding(element_node);
            }
            else
            {
                reporter->warning(
                    "{}: unhandled XML node `{}`",
                    locations.find(element_node.offset_debug()),
                    element_node.name());
                continue;
            }

            unique_element_names.add_or_throw(
                utils::get_encoding_name(element),
                "{}: duplicate composite element: `{}`",
                utils::get_location(element),
                utils::get_encoding_name(element));

            elements.push_back(std::move(element));
        }

        return elements;
    }

    sbe::composite parse_composite_encoding(const pugi::xml_node root)
    {
        sbe::composite c{};
        c.location = locations.find(root.offset_debug());
        c.name = get_required_name(root);
        c.offset = get_offset(root);
        c.description = get_description(root);
        c.semantic_type = get_semantic_type(root);
        c.added_since = get_added_since(root);
        c.deprecated_since = get_deprecated_since(root);
        validate_versions(c);
        c.elements = parse_composite_elements(root);

        return c;
    }

    void parse_types_node(const pugi::xml_node root)
    {
        for(const auto child : root)
        {
            if(is_node_name_equal_to(child.name(), "type"))
            {
                add_unique_type(parse_type_encoding(child));
            }
            else if(is_node_name_equal_to(child.name(), "composite"))
            {
                add_unique_type(parse_composite_encoding(child));
            }
            else if(is_node_name_equal_to(child.name(), "enum"))
            {
                add_unique_type(parse_enum_encoding(child));
            }
            else if(is_node_name_equal_to(child.name(), "set"))
            {
                add_unique_type(parse_set_encoding(child));
            }
        }
    }

    template<typename T>
    T get_required_numeric_attribute(
        const pugi::xml_node root, const std::string_view attr_name) const
    {
        const auto as_str = get_required_non_empty_string(root, attr_name);
        return utils::string_to_number_or_throw<T>(
            as_str,
            "{}: cannot convert `{}` value ({}) to its underlying numeric type",
            locations.find(root.offset_debug()),
            attr_name,
            as_str);
    }

    template<typename T>
    std::optional<T> get_optional_numeric_attribute(
        const pugi::xml_node root, const std::string_view attr_name) const
    {
        const auto as_str = get_optional_string_attribute(root, attr_name);
        if(as_str)
        {
            return utils::string_to_number_or_throw<T>(
                *as_str,
                "{}: cannot convert `{}` value ({}) to its underlying numeric "
                "type",
                locations.find(root.offset_debug()),
                attr_name,
                *as_str);
        }

        return {};
    }

    template<typename Id>
    Id get_id(const pugi::xml_node root)
    {
        return get_required_numeric_attribute<Id>(root, "id");
    }

    message_id_t get_message_id(const pugi::xml_node root)
    {
        return get_id<message_id_t>(root);
    }

    std::optional<block_length_t> get_block_length(const pugi::xml_node root)
    {
        return get_optional_numeric_attribute<block_length_t>(
            root, "blockLength");
    }

    static void throw_if_unexpected_member_type(
        const ordered_member_type current,
        const ordered_member_type previous,
        const std::string_view name,
        const source_location& location)
    {
        if(current < previous)
        {
            throw_error(
                "{}: member `{}` is unexpected here, valid order is fields, "
                "groups, data",
                location,
                name);
        }
    }

    static void throw_if_not_unique_member_name(
        unique_set<std::string>& unique_member_names,
        const std::string_view name,
        const source_location& location)
    {
        unique_member_names.add_or_throw(
            name, "{}: member with name `{}` already exists", location, name);
    }

    member_id_t get_field_id(const pugi::xml_node root)
    {
        return get_id<member_id_t>(root);
    }

    sbe::field parse_field_member(const pugi::xml_node root)
    {
        sbe::field f{};
        f.location = locations.find(root.offset_debug());
        f.name = get_required_name(root);
        f.id = get_field_id(root);
        f.description = get_description(root);
        f.type = get_required_non_empty_string(root, "type");
        f.offset = get_offset(root);
        f.presence = get_presence(root);
        f.value_ref = get_optional_string_attribute(root, "valueRef");
        f.added_since = get_added_since(root);
        f.deprecated_since = get_deprecated_since(root);
        validate_versions(f);

        return f;
    }

    static std::string get_group_dimension_type(const pugi::xml_node root)
    {
        return get_optional_string_attribute(root, "dimensionType")
            .value_or("groupSizeEncoding");
    }

    sbe::group parse_group_member(const pugi::xml_node root)
    {
        sbe::group g{};

        g.location = locations.find(root.offset_debug());
        g.name = get_required_name(root);
        g.id = get_field_id(root);
        g.description = get_description(root);
        g.dimension_type = get_group_dimension_type(root);
        g.block_length = get_block_length(root);
        g.semantic_type = get_semantic_type(root);
        g.added_since = get_added_since(root);
        g.deprecated_since = get_deprecated_since(root);
        validate_versions(g);
        g.members = get_level_members(root);

        return g;
    }

    sbe::data parse_data_member(const pugi::xml_node root)
    {
        sbe::data d{};
        d.location = locations.find(root.offset_debug());
        d.name = get_required_name(root);
        d.id = get_field_id(root);
        d.description = get_description(root);
        d.type = get_required_non_empty_string(root, "type");
        d.added_since = get_added_since(root);
        d.deprecated_since = get_deprecated_since(root);
        validate_versions(d);

        return d;
    }

    sbe::level_members get_level_members(const pugi::xml_node root)
    {
        // does NOT verify member IDs since their uniqueness scope is not
        // specified
        // https://github.com/FIXTradingCommunity/fix-simple-binary-encoding/issues/151#issuecomment-1194138068
        sbe::level_members members;
        auto prev_member_type = ordered_member_type::field;
        unique_set<std::string> unique_member_names;

        for(const auto child : root)
        {
            if(is_node_name_equal_to(child.name(), "field"))
            {
                auto f = parse_field_member(child);
                throw_if_unexpected_member_type(
                    ordered_member_type::field,
                    prev_member_type,
                    f.name,
                    f.location);
                throw_if_not_unique_member_name(
                    unique_member_names, f.name, f.location);
                members.fields.emplace_back(std::move(f));
            }
            else if(is_node_name_equal_to(child.name(), "group"))
            {
                auto g = parse_group_member(child);
                throw_if_unexpected_member_type(
                    ordered_member_type::group,
                    prev_member_type,
                    g.name,
                    g.location);
                prev_member_type = ordered_member_type::group;
                throw_if_not_unique_member_name(
                    unique_member_names, g.name, g.location);
                members.groups.emplace_back(std::move(g));
            }
            else if(is_node_name_equal_to(child.name(), "data"))
            {
                auto d = parse_data_member(child);
                // no need to use `throw_if_unexpected_member_type` here because
                // data members can appear after any other member type, they
                // can never trigger an error on their own
                prev_member_type = ordered_member_type::data;
                throw_if_not_unique_member_name(
                    unique_member_names, d.name, d.location);
                members.data.emplace_back(std::move(d));
            }
        }

        return members;
    }

    void parse_message_node(const pugi::xml_node root)
    {
        sbe::message m{};

        m.location = locations.find(root.offset_debug());
        m.name = get_required_name(root);
        m.id = get_message_id(root);
        m.description = get_description(root);
        m.block_length = get_block_length(root);
        m.semantic_type = get_semantic_type(root);
        m.added_since = get_added_since(root);
        m.deprecated_since = get_deprecated_since(root);
        validate_versions(m);
        m.members = get_level_members(root);

        add_unique_message(std::move(m));
    }

    void parse_schema_content(const pugi::xml_node root)
    {
        for(const auto child : root)
        {
            if(is_node_name_equal_to(child.name(), "types"))
            {
                parse_types_node(child);
            }
            else if(is_node_name_equal_to(child.name(), "message"))
            {
                parse_message_node(child);
            }
            else if(is_node_name_equal_to(child.name(), "include"))
            {
                parse_include(child);
            }
            else
            {
                reporter->warning(
                    "{}: unhandled XML node `{}`",
                    locations.find(child.offset_debug()),
                    child.name());
            }
        }
    }

    pugi::xml_node get_message_schema_node(const pugi::xml_node root)
    {
        for(const auto child : root)
        {
            if(is_node_name_equal_to(child.name(), "messageSchema"))
            {
                return child;
            }
            else
            {
                reporter->warning(
                    "{}: unhandled XML node `{}`",
                    locations.find(child.offset_debug()),
                    child.name());
            }
        }

        throw_error(
            "{}: can't find `messageSchema` child",
            locations.find(root.offset_debug()));
    }

    void parse_xml(const std::string_view data)
    {
        const auto result = xml_doc.load_buffer(
            data.data(),
            data.size(),
            pugi::parse_default | pugi::parse_pi); // for <?include?>

        if(!result)
        {
            const auto location = locations.find(result.offset);
            throw_error(
                "{}: XML parsing error: `{}`", location, result.description());
        }
    }

    schema_id_t get_schema_id(const pugi::xml_node root)
    {
        return get_id<schema_id_t>(root);
    }

    version_t get_schema_version(const pugi::xml_node root)
    {
        return get_required_numeric_attribute<version_t>(root, "version");
    }

    sbe::message_schema parse_message_schema(const pugi::xml_node root)
    {
        sbe::message_schema res{};

        res.package = root.attribute("package").as_string();
        res.id = get_schema_id(root);
        res.version = get_schema_version(root);
        res.semantic_version = root.attribute("semanticVersion").as_string();
        res.byte_order = get_byte_order(root);
        res.description = root.attribute("description").as_string();
        res.header_type =
            root.attribute("headerType").as_string("messageHeader");
        res.location = locations.find(root.offset_debug());

        return res;
    }

    sbe::byte_order_kind get_byte_order(const pugi::xml_node root)
    {
        const auto value = get_optional_string_attribute(root, "byteOrder")
                               .value_or("littleEndian");

        if(value == "littleEndian")
        {
            return sbe::byte_order_kind::little_endian;
        }
        else if(value == "bigEndian")
        {
            return sbe::byte_order_kind::big_endian;
        }

        throw_error(
            "{}: unknown byteOrder value: `{}`",
            locations.find(root.offset_debug()),
            value);
    }

    static bool is_node_name_equal_to(
        const char* node_name, const std::string_view name)
    {
        if(node_name == name)
        {
            return true;
        }

        const auto colon = std::strchr(node_name, ':');
        if(colon)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return (std::strcmp(colon + 1, name.data()) == 0);
        }

        return false;
    }

    void warn_about_greater_version(
        const version_t lhs,
        const std::string_view lhs_name,
        const version_t rhs,
        const std::string_view rhs_name,
        const source_location& location)
    {
        if(lhs > rhs)
        {
            reporter->warning(
                "{}: {} version ({}) is greater than {} version (`{}`)",
                location,
                lhs_name,
                lhs,
                rhs_name,
                rhs);
        }
    }

    template<typename T>
    void validate_versions(const T& entity)
    {
        warn_about_greater_version(
            entity.added_since,
            "sinceVersion",
            message_schema.version,
            "schema",
            entity.location);

        if(entity.deprecated_since)
        {
            warn_about_greater_version(
                *entity.deprecated_since,
                "deprecated",
                message_schema.version,
                "schema",
                entity.location);
            warn_about_greater_version(
                entity.added_since,
                "sinceVersion",
                *entity.deprecated_since,
                "deprecated",
                entity.location);
        }
    }
};
} // namespace sbepp::sbeppc
