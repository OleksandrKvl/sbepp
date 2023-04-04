// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <sbepp/sbepp.hpp>

namespace sbepp
{
void assertion_failed(
    char const* /*expr*/,
    char const* /*function*/,
    char const* /*file*/,
    long /*line*/)
{
    std::terminate();
}
} // namespace sbepp
