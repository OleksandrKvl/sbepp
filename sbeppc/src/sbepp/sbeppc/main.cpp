// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <sbepp/sbeppc/throw_error.hpp>
#include <sbepp/sbeppc/fs_provider.hpp>
#include <sbepp/sbeppc/reporter.hpp>
#include <sbepp/sbeppc/schema_parser.hpp>
#include <sbepp/sbeppc/sbe_error.hpp>
// #include <sbepp/sbeppc/schema_compiler.hpp>
#include <sbepp/sbeppc/build_info.hpp>
#include <sbepp/sbeppc/sbe_checker.hpp>

#include <fmt/core.h>

#include <string>
#include <string_view>
#include <optional>

namespace sbepp::sbeppc
{
char* get_option_value(const int argc, char** argv, int option_index)
{
    if(option_index + 1 < argc)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return argv[option_index + 1];
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    throw_error("no value for option `{}`", argv[option_index]);
}

[[noreturn]] void print_help_and_exit()
{
    fmt::print(
        // clang-format off
R"(Usage:  sbeppc [OPTIONS]... FILE

Options:
    --schema-name NAME    override schema name. Uses `messageSchema.package` by
                          default
    --output-dir DIR      output directory. Uses current directory by default
    --inject-include PATH injects `#include "PATH"` directive at the top of
                          `schema/schema.hpp`
    --version             print version and exit
    --help                print this help and exit
    --                    end of optional arguments
)"
        // clang-format on
    );
    std::exit(0);
}

[[noreturn]] void print_version_and_exit()
{
    fmt::print("sbeppc version: {}\n", build_info::get_version());
    std::exit(0);
}

struct sbeppc_config
{
    std::string schema_file;
    std::optional<std::string> schema_name;
    std::filesystem::path output_dir{"."};
    std::optional<std::string> inject_include;
};

sbeppc_config parse_command_line(int argc, char** argv)
{
    if(argc < 2)
    {
        print_help_and_exit();
    }

    sbeppc_config config;

    int i = 1;
    for(; i != argc; i++)
    {
        using namespace std::string_view_literals;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const auto arg = argv[i];
        if(arg == "--schema-name"sv)
        {
            config.schema_name = get_option_value(argc, argv, i);
            i++;
        }
        else if(arg == "--output-dir"sv)
        {
            config.output_dir = get_option_value(argc, argv, i);
            i++;
        }
        else if(arg == "--inject-include"sv)
        {
            config.inject_include = get_option_value(argc, argv, i);
            i++;
        }
        else if(arg == "--version"sv)
        {
            print_version_and_exit();
        }
        else if(arg == "--help"sv)
        {
            print_help_and_exit();
        }
        else if(arg == "--"sv)
        {
            i++;
            break;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        else if(arg[0] == '-')
        {
            throw_error("unknown argument: `{}`", arg);
            break;
        }
        else
        {
            break;
        }
    }

    if(i == argc)
    {
        throw_error("missing filename");
    }
    else if(argc - i != 1)
    {
        throw_error("too many arguments");
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    config.schema_file = argv[i];

    return config;
}
} // namespace sbepp::sbeppc

int main(int argc, char** argv)
{
    using namespace sbepp::sbeppc;

    reporter reporter;
    fs_provider fs_provider;

    try
    {
        const auto config = sbepp::sbeppc::parse_command_line(argc, argv);
        schema_parser parser{config.schema_file, reporter, fs_provider};
        parser.parse_schema();
        const auto& schema = parser.get_message_schema();
        // TODO: SBE-related verification
        sbe_checker checker;
        context_manager ctx_manager;
        checker.check(schema, ctx_manager);

        // schema.name = config.schema_name.value_or(schema.package);
        // utils::validate_schema_name(schema, reporter);

        // schema_compiler::compile(
        //     config.output_dir,
        //     config.inject_include,
        //     schema,
        //     types,
        //     messages,
        //     fs_provider);
    }
    catch(const sbe_error& e)
    {
        reporter.error(e.what());
        return 1;
    }

    return 0;
}
