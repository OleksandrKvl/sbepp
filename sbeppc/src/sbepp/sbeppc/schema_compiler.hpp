// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/ifs_provider.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/types_compiler.hpp>
#include <sbepp/sbeppc/messages_compiler.hpp>
#include <sbepp/sbeppc/traits_generator.hpp>
#include <sbepp/sbeppc/tags_generator.hpp>

#include <fmt/core.h>
#include <fmt/std.h>
#include <fmt/ranges.h>

#include <filesystem>
#include <string_view>

namespace sbepp::sbeppc
{
class schema_compiler
{
public:
    static void compile(
        const std::filesystem::path& output_dir,
        const std::optional<std::string>& inject_include,
        sbe::message_schema& schema,
        type_manager& types,
        message_manager& messages,
        ifs_provider& fs_provider)
    {
        create_dirs(output_dir, schema.name, fs_provider);

        tags_generator tags_gen{schema, types, messages};
        const auto& tags = tags_gen.get();
        traits_generator traits_gen{schema, types};

        // types
        std::vector<std::string> type_includes;
        types_compiler tc{schema.name, schema.byte_order, types, traits_gen};
        tc.compile(
            [&type_includes, &output_dir, &schema, &fs_provider](
                const auto name,
                const auto implementation,
                const auto alias,
                const auto& dependencies,
                const auto traits)
            {
                const auto include_path =
                    std::filesystem::path{"types"} / name += ".hpp";
                type_includes.push_back(
                    fmt::format("#include \"{}\"", include_path.string()));
                const auto file_path = output_dir / schema.name / include_path;

                fs_provider.write_file(
                    file_path,
                    fmt::format(
                        // clang-format off
R"({top_comment}
#pragma once

#include <sbepp/sbepp.hpp>

SBEPP_WARNINGS_OFF();

#include "../schema/schema.hpp"
{dependency_includes}

namespace {schema}
{{
namespace detail
{{
namespace types
{{
{implementation}
}} // namespace types
}} // namespace detail

namespace types
{{
{alias}
}} // namespace types
}} // namespace {schema}

namespace sbepp
{{
{traits}
}}  // namespace sbepp

SBEPP_WARNINGS_ON();
)",
                        // clang-format on
                        fmt::arg("schema", schema.name),
                        fmt::arg(
                            "dependency_includes",
                            make_local_includes(dependencies)),
                        fmt::arg("implementation", implementation),
                        fmt::arg("alias", alias),
                        fmt::arg("traits", traits),
                        fmt::arg(
                            "top_comment",
                            utils::get_compiled_header_top_comment())));
            });

        // schema/schema.hpp
        // schema traits must be generated after types compilation because they
        // need message header type to be compiled
        const auto schema_traits = traits_gen.make_schema_traits();
        const auto& header_type = types.get_as_or_throw<sbe::composite>(
            schema.header_type,
            "{}: type `{}` doesn't exist or it's not a composite",
            schema.location,
            schema.header_type);

        fs_provider.write_file(
            output_dir / schema.name / "schema" / "schema.hpp",
            fmt::format(
                // clang-format off
R"({top_comment}
#pragma once

{injected_include}
#include <sbepp/sbepp.hpp>

SBEPP_WARNINGS_OFF();

namespace {schema}
{{
{tags}

namespace detail
{{
namespace types
{{
template<typename Byte>
class {header_type};
}} // namespace types
}} // namespace detail
}} // namespace {schema}

namespace sbepp
{{
{schema_traits}
}}  // namespace sbepp

SBEPP_WARNINGS_ON();
)",
                // clang-format on
                fmt::arg("schema", schema.name),
                fmt::arg("tags", tags),
                fmt::arg("schema_traits", schema_traits),
                // can't use `header_type.name` here because it's an alias and
                // it's not possible to forward declare it
                fmt::arg("header_type", header_type.impl_name),
                fmt::arg(
                    "top_comment", utils::get_compiled_header_top_comment()),
                fmt::arg(
                    "injected_include",
                    make_injected_include(inject_include))));

        // messages
        std::vector<std::string> message_includes;
        messages_compiler mc{schema, types, messages, traits_gen};
        mc.compile(
            [&message_includes, &output_dir, &schema, &fs_provider](
                const auto name,
                const auto implementation,
                const auto alias,
                const auto& dependencies,
                const auto traits)
            {
                const auto include_path =
                    std::filesystem::path{"messages"} / name += ".hpp";
                message_includes.push_back(
                    fmt::format("#include \"{}\"", include_path.string()));
                const auto file_path = output_dir / schema.name / include_path;

                fs_provider.write_file(
                    file_path,
                    fmt::format(
                        // clang-format off
R"({top_comment}
#pragma once

#include <sbepp/sbepp.hpp>

SBEPP_WARNINGS_OFF();

#include "../schema/schema.hpp"
{dependency_includes}

namespace {schema}
{{
namespace detail
{{
namespace messages
{{
{implementation}
}} // namespace messages
}} // namespace detail

namespace messages
{{
{alias}
}} // namespace messages
}} // namespace {schema}

namespace sbepp
{{
{traits}
}}  // namespace sbepp

SBEPP_WARNINGS_ON();
)",
                        // clang-format on
                        fmt::arg("schema", schema.name),
                        fmt::arg(
                            "dependency_includes",
                            make_type_dependency_includes(dependencies)),
                        fmt::arg("implementation", implementation),
                        fmt::arg("alias", alias),
                        fmt::arg("traits", traits),
                        fmt::arg(
                            "top_comment",
                            utils::get_compiled_header_top_comment())));
            });

        // top level header
        fs_provider.write_file(
            output_dir / schema.name / schema.name += ".hpp",
            fmt::format(
                // clang-format off
R"({top_comment}
#pragma once

#include "schema/schema.hpp"
{type_includes}
{message_includes}
)",
                // clang-format on
                fmt::arg("type_includes", fmt::join(type_includes, "\n")),
                fmt::arg("message_includes", fmt::join(message_includes, "\n")),
                fmt::arg(
                    "top_comment", utils::get_compiled_header_top_comment())));
    }

private:
    static void create_dirs(
        const std::filesystem::path& output_dir,
        const std::string_view schema_name,
        ifs_provider& fs_provider)
    {
        fs_provider.create_directories(output_dir / schema_name / "schema");
        fs_provider.create_directories(output_dir / schema_name / "types");
        fs_provider.create_directories(output_dir / schema_name / "messages");
    }

    static std::string
        make_local_includes(const std::unordered_set<std::string>& types)
    {
        std::string res;
        for(const auto& type : types)
        {
            res += fmt::format("#include \"{}.hpp\"\n", type);
        }

        return res;
    }

    static std::string make_type_dependency_includes(
        const std::unordered_set<std::string>& types)
    {
        std::string res;
        for(const auto& type : types)
        {
            res += fmt::format("#include \"../types/{}.hpp\"\n", type);
        }

        return res;
    }

    static std::string
        make_injected_include(const std::optional<std::string>& inject_include)
    {
        if(inject_include)
        {
            return fmt::format("#include \"{}\"", *inject_include);
        }

        return {};
    }
};
} // namespace sbepp::sbeppc