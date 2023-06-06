// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include "sbepp/sbepp.hpp"
#ifdef USE_TOP_FILE
#    include <test_schema/test_schema.hpp>
#else
#    include <test_schema/types/messageHeader.hpp>
#endif

#include <gtest/gtest.h>

#include <cstdint>

namespace
{
namespace std_like
{
// like `std::byte`
enum class byte : std::uint8_t
{
};

// like `std::addressof`
template<class T>
const T* addressof(const T&&) = delete;
} // namespace std_like

TEST(StdByteAdlTest, AddressofIsNotAmbiguous)
{
    // This models issue with C++17 `std::byte` when `sbepp::init_cursor` called
    // unqualified `addressof` which was ambiguous between `sbepp::addressof`
    // and `std::addressof`. Nothing special to test here, it should just
    // compile
    test_schema::types::messageHeader<std_like::byte> m;
    sbepp::init_cursor(m);
}
} // namespace
