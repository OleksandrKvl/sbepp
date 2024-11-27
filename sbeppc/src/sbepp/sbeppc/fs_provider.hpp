// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbeppc/ifs_provider.hpp>
#include <sbepp/sbeppc/throw_error.hpp>

// for `std::filesystem::path` formatting
#include <fmt/std.h>

#include <string>
#include <fstream>

namespace sbepp::sbeppc
{
class fs_provider : public ifs_provider
{
public:
    std::string read_file(const std::filesystem::path& path) override
    {
        std::ifstream is{path, std::ios::in | std::ios::binary | std::ios::ate};
        if(is)
        {
            const auto file_size = is.tellg();
            std::string data;
            data.resize(file_size);
            is.seekg(0);
            if(is.read(data.data(), file_size))
            {
                return data;
            }
            throw_error("can't read file: `{}`", path);
        }
        throw_error("can't open file: `{}`", path);
    }

    void write_file(
        const std::filesystem::path& path, const std::string_view data) override
    {
        std::ofstream output_stream{path, std::ios::binary | std::ios::out};
        if(!output_stream)
        {
            throw_error("can't open file: `{}`", path);
        }
        output_stream << data;
    }

    void create_directories(const std::filesystem::path& path) override
    {
        std::error_code ec;
        std::filesystem::create_directories(path, ec);
        if(ec)
        {
            throw_error(
                "can't create directory {}, error: `{}`", path, ec.message());
        }
    }
};
} // namespace sbepp::sbeppc
