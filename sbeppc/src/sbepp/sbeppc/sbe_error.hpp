// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <stdexcept>

namespace sbepp::sbeppc
{
class sbe_error : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};
} // namespace sbepp::sbeppc
