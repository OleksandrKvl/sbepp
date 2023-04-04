// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <string>
#include <string_view>
#include <filesystem>

namespace sbepp::sbeppc
{
class ifs_provider
{
public:
    virtual ~ifs_provider() = default;

    virtual std::string read_file(const std::filesystem::path& path) = 0;
    virtual void write_file(
        const std::filesystem::path& path, std::string_view data) = 0;
    virtual void create_directories(const std::filesystem::path& path) = 0;
};
} // namespace sbepp::sbeppc
