// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/sbe.hpp>
#include <sbepp/sbeppc/ifs_provider.hpp>
#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/utils.hpp>
#include <sbepp/sbeppc/types_compiler.hpp>
#include <sbepp/sbeppc/messages_compiler.hpp>
#include <sbepp/sbeppc/traits_generator.hpp>
#include <sbepp/sbeppc/tags_generator.hpp>
#include <sbepp/sbeppc/context_manager.hpp>

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
        const sbe::message_schema& schema,
        context_manager& ctx_manager,
        ifs_provider& fs_provider)
    {
        schema_compiler compiler{
            output_dir, inject_include, schema, ctx_manager, fs_provider};
        compiler.compile();
    }

private:
    std::filesystem::path output_dir;
    std::optional<std::string> inject_include;
    const sbe::message_schema* schema{};
    context_manager* ctx_manager{};
    ifs_provider* fs_provider{};
    std::string_view schema_name;
    traits_generator traits_gen;
    std::vector<std::string> type_includes;
    std::vector<std::string> message_includes;

    schema_compiler(
        const std::filesystem::path& output_dir,
        const std::optional<std::string>& inject_include,
        const sbe::message_schema& schema,
        context_manager& ctx_manager,
        ifs_provider& fs_provider)
        : output_dir{output_dir},
          inject_include{inject_include},
          schema{&schema},
          ctx_manager{&ctx_manager},
          fs_provider{&fs_provider},
          schema_name{ctx_manager.get(schema).name},
          traits_gen{schema, ctx_manager}
    {
    }

    void compile_types()
    {
        types_compiler tc{*schema, traits_gen, *ctx_manager};
        tc.compile(
            [this](
                const std::string_view name,
                const std::string_view detail_type,
                const std::string_view public_type,
                const std::unordered_set<std::string>& dependencies,
                const std::string_view traits)
            {
                const auto include_path =
                    std::filesystem::path{"types"} / name += ".hpp";
                type_includes.push_back(
                    fmt::format("#include \"{}\"", include_path.string()));
                const auto file_path = output_dir / schema_name / include_path;

                this->fs_provider->write_file(
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
{detail_type}

namespace types
{{
{public_type}
}} // namespace types
}} // namespace {schema}

namespace sbepp
{{
{traits}
}} // namespace sbepp

SBEPP_WARNINGS_ON();
)",
                        // clang-format on
                        fmt::arg("schema", schema_name),
                        fmt::arg(
                            "dependency_includes",
                            make_local_includes(dependencies)),
                        fmt::arg("detail_type", make_type_detail(detail_type)),
                        fmt::arg("public_type", public_type),
                        fmt::arg("traits", traits),
                        fmt::arg(
                            "top_comment",
                            utils::get_compiled_header_top_comment())));
            });
    }

    void make_tags_header(const std::string_view tags)
    {
        // schema/schema.hpp
        // schema traits must be generated after types compilation because they
        // need message header type to be compiled
        const auto schema_traits = traits_gen.make_schema_traits();
        const auto& header_type = utils::get_schema_encoding_as<sbe::composite>(
            *schema, schema->header_type);

        fs_provider->write_file(
            output_dir / schema_name / "schema" / "schema.hpp",
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

{header_forward_decl}
}} // namespace {schema}

namespace sbepp
{{
{schema_traits}
}} // namespace sbepp

SBEPP_WARNINGS_ON();
)",
                // clang-format on
                fmt::arg("schema", schema_name),
                fmt::arg("tags", tags),
                fmt::arg("schema_traits", schema_traits),
                fmt::arg(
                    "top_comment", utils::get_compiled_header_top_comment()),
                fmt::arg(
                    "injected_include", make_injected_include(inject_include)),
                fmt::arg(
                    "header_forward_decl",
                    make_schema_header_forward_declaration(
                        header_type.name,
                        ctx_manager->get(header_type).mangled_name,
                        schema_name))));
    }

    void compile_messages()
    {
        messages_compiler mc{*schema, traits_gen, *ctx_manager};
        mc.compile(
            [this](
                const std::string_view name,
                const std::string_view detail_message,
                const std::string_view public_message,
                const std::unordered_set<std::string>& dependencies,
                const std::string_view traits)
            {
                const auto include_path =
                    std::filesystem::path{"messages"} / name += ".hpp";
                message_includes.push_back(
                    fmt::format("#include \"{}\"", include_path.string()));
                const auto file_path = output_dir / schema_name / include_path;

                this->fs_provider->write_file(
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
{detail_message}

namespace messages
{{
{public_message}
}} // namespace messages
}} // namespace {schema}

namespace sbepp
{{
{traits}
}} // namespace sbepp

SBEPP_WARNINGS_ON();
)",
                        // clang-format on
                        fmt::arg("schema", schema_name),
                        fmt::arg(
                            "dependency_includes",
                            make_type_dependency_includes(dependencies)),
                        fmt::arg(
                            "detail_message",
                            make_message_detail(detail_message)),
                        fmt::arg("public_message", public_message),
                        fmt::arg("traits", traits),
                        fmt::arg(
                            "top_comment",
                            utils::get_compiled_header_top_comment())));
            });
    }

    void make_top_header()
    {
        fs_provider->write_file(
            output_dir / schema_name / schema_name += ".hpp",
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

    void compile()
    {
        create_output_dirs();

        // tags must be generated first because they are used by types compiler
        const auto& tags = tags_generator::generate(*schema, *ctx_manager);

        compile_types();
        make_tags_header(tags);
        compile_messages();
        make_top_header();
    }

    void create_output_dirs()
    {
        fs_provider->create_directories(output_dir / schema_name / "schema");
        fs_provider->create_directories(output_dir / schema_name / "types");
        fs_provider->create_directories(output_dir / schema_name / "messages");
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

    static std::string make_schema_header_forward_declaration(
        const std::string_view public_name,
        const std::optional<std::string>& mangled_name,
        const std::string_view schema_name)
    {
        if(mangled_name)
        {
            return fmt::format(
                // clang-format off
R"(namespace detail
{{
namespace types
{{
template<typename Byte>
class {impl_name};
}} // namespace types
}} // namespace detail

namespace types
{{
template<typename Byte>
using {public_name} = ::{schema_name}::detail::types::{impl_name}<Byte>;
}} // namespace types)",
                // clang-format on
                fmt::arg("impl_name", *mangled_name),
                fmt::arg("public_name", public_name),
                fmt::arg("schema_name", schema_name));
        }
        else
        {
            return fmt::format(
                // clang-format off
R"(namespace types
{{
template<typename Byte>
class {public_name};
}} // namespace types)",
                // clang-format on
                fmt::arg("public_name", public_name));
        }
    }

    static std::string make_message_detail(const std::string_view content)
    {
        if(content.empty())
        {
            return {};
        }

        return fmt::format(
            // clang-format off
R"(namespace detail
{{
namespace messages
{{
{content}
}} // namespace messages
}} // namespace detail)",
            // clang-format on
            fmt::arg("content", content));
    }

    static std::string make_type_detail(const std::string_view content)
    {
        if(content.empty())
        {
            return {};
        }

        return fmt::format(
            // clang-format off
R"(namespace detail
{{
namespace types
{{
{content}
}} // namespace types
}} // namespace detail)",
            // clang-format on
            fmt::arg("content", content));
    }
};
} // namespace sbepp::sbeppc