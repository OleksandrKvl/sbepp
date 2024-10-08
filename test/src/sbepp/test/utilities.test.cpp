// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <test_schema/messages/msg2.hpp>
#include <test_schema/types/uint32_opt.hpp>
#include <test_schema/types/uint32_req.hpp>
#include <test_schema/types/options_set.hpp>

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>
#include <type_traits>

namespace
{
using byte_type = std::uint8_t;

template<typename Byte>
class view
{
public:
    using byte_type = Byte;

    SBEPP_CPP20_CONSTEXPR view(Byte* ptr, std::size_t size)
        : ptr{ptr}, size{size}
    {
    }

    Byte* get_ptr() const
    {
        return ptr;
    }

    std::size_t get_size() const
    {
        return size;
    }

private:
    Byte* ptr;
    std::size_t size;
};

TEST(MakeViewTest, MakeViewCreatesTypeFromPointerAndSize)
{
    std::array<byte_type, 10> arr{};

    auto v = sbepp::make_view<view>(arr.data(), arr.size());

    STATIC_ASSERT_V(std::is_same<decltype(v)::byte_type, byte_type>);
    STATIC_ASSERT(noexcept(sbepp::make_view<view>(arr.data(), arr.size())));
    ASSERT_EQ(v.get_ptr(), arr.data());
    ASSERT_EQ(v.get_size(), arr.size());
}

TEST(MakeViewTest, MakeViewPreservesByteType)
{
    std::array<byte_type, 10> arr{};
    const auto& const_arr = arr;

    auto v = sbepp::make_view<view>(const_arr.data(), const_arr.size());

    STATIC_ASSERT_V(std::is_same<decltype(v)::byte_type, const byte_type>);
    STATIC_ASSERT(
        noexcept(sbepp::make_view<view>(const_arr.data(), const_arr.size())));
    ASSERT_EQ(v.get_ptr(), const_arr.data());
    ASSERT_EQ(v.get_size(), const_arr.size());
}

TEST(MakeViewTest, ConstMakeViewAddsConstToByteType)
{
    std::array<byte_type, 10> arr{};

    auto v = sbepp::make_const_view<view>(arr.data(), arr.size());

    STATIC_ASSERT_V(std::is_same<decltype(v)::byte_type, const byte_type>);
    STATIC_ASSERT(noexcept(sbepp::make_view<view>(arr.data(), arr.size())));
    ASSERT_EQ(v.get_ptr(), arr.data());
    ASSERT_EQ(v.get_size(), arr.size());
}

TEST(ByteTypeTest, ByteTypeProvidesViewByteType)
{
    using view_type = test_schema::messages::msg2<byte_type>;

    STATIC_ASSERT_V(std::is_same<sbepp::byte_type_t<view_type>, byte_type>);
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test()
{
    std::array<byte_type, 512> buf{};
    sbepp::make_view<view>(buf.data(), buf.size());
    sbepp::make_const_view<view>(buf.data(), buf.size());

    return buf;
}

constexpr auto buf = constexpr_test();
#endif
} // namespace
