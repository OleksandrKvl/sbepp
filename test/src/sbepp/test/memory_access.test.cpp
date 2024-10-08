// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <test_schema/types/composite_1.hpp>
#include <test_schema/types/composite_2.hpp>
#include <test_schema/types/composite_3.hpp>
#include <test_schema/types/composite_4.hpp>
#include <test_schema/types/composite_5.hpp>
#include <test_schema/types/composite_6.hpp>
#include <test_schema/types/composite_7.hpp>
#include <test_schema/types/composite_8.hpp>
#include <test_schema/types/composite_9.hpp>
#include <test_schema/types/composite_10.hpp>
#include <test_schema/types/composite_11.hpp>
#include <test_schema/types/composite_12.hpp>
#include <test_schema/types/composite_13.hpp>
#include <test_schema/types/composite_14.hpp>
#include <test_schema/types/composite_15.hpp>
#include <test_schema/types/composite_16.hpp>
#include <test_schema/types/composite_17.hpp>
#include <test_schema/types/composite_18.hpp>
#include <test_schema/messages/msg14.hpp>
#include <test_schema/messages/msg15.hpp>
#include <test_schema/messages/msg16.hpp>
#include <test_schema/messages/msg17.hpp>
#include <test_schema/messages/msg18.hpp>
#include <test_schema/messages/msg19.hpp>
#include <test_schema/messages/msg20.hpp>
#include <test_schema/messages/msg21.hpp>
#include <test_schema/messages/msg22.hpp>
#include <test_schema/messages/msg23.hpp>
#include <test_schema/messages/msg24.hpp>
#include <test_schema/messages/msg25.hpp>

#include <big_endian_schema/types/composite_1.hpp>
#include <big_endian_schema/types/composite_2.hpp>
#include <big_endian_schema/types/composite_3.hpp>
#include <big_endian_schema/types/composite_4.hpp>
#include <big_endian_schema/types/composite_5.hpp>
#include <big_endian_schema/messages/msg1.hpp>

#include <algorithm>
#include <gtest/gtest.h>

#include <cstring>
#include <type_traits>
#include <utility>
#include <array>

namespace
{
template<bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template<typename T>
using decay_t = typename std::decay<T>::type;

using byte_type = std::uint8_t;

template<typename...>
using void_t = void;

template<typename T, sbepp::endian E, typename Byte>
T bit_cast(Byte* ptr)
{
    std::array<std::uint8_t, sizeof(T)> underlying{};
    if(E == sbepp::endian::native)
    {
        std::copy(ptr, ptr + sizeof(T), std::begin(underlying));
    }
    else
    {
        std::reverse_copy(ptr, ptr + sizeof(T), std::begin(underlying));
    }

    T res;
    std::memcpy(&res, underlying.data(), underlying.size());

    return res;
}

template<typename T, typename = void_t<>>
struct has_get_level_tag : std::false_type
{
};

template<typename T>
struct has_get_level_tag<
    T,
    void_t<decltype(std::declval<T>()(sbepp::detail::get_level_tag{}))>>
    : std::true_type
{
};

template<typename View, typename = enable_if_t<has_get_level_tag<View>::value>>
auto get_fields_start(View view)
    -> decltype(view(sbepp::detail::get_level_tag{}))
{
    return view(sbepp::detail::get_level_tag{});
}

template<typename View, typename = enable_if_t<!has_get_level_tag<View>::value>>
auto get_fields_start(View view)
    -> decltype(view(sbepp::detail::addressof_tag{}))
{
    return view(sbepp::detail::addressof_tag{});
}

template<typename C>
class FieldsContainer : public ::testing::Test
{
public:
    std::array<byte_type, 512> buf{};
    C view{buf.data(), buf.size()};
};

// simple wrapper to initialize `block_length` parameter of entry's constructor
// to make it constructible from pointer+size
template<typename GroupTag>
class entry_wrapper
    : public sbepp::group_traits<GroupTag>::template entry_type<byte_type>
{
public:
    using base_t =
        typename sbepp::group_traits<GroupTag>::template entry_type<byte_type>;

    constexpr entry_wrapper(byte_type* ptr, const std::size_t size)
        : base_t{ptr, size, sbepp::group_traits<GroupTag>::block_length()}
    {
    }
};

template<typename T>
using entry_type_t =
    typename sbepp::group_traits<T>::template entry_type<byte_type>;

using DefaultOffsetFieldContainers = ::testing::Types<
    test_schema::types::composite_1<byte_type>,
    test_schema::types::composite_2<byte_type>,
    test_schema::types::composite_3<byte_type>,
    test_schema::types::composite_4<byte_type>,
    test_schema::types::composite_5<byte_type>,
    test_schema::types::composite_6<byte_type>,
    test_schema::messages::msg14<byte_type>,
    test_schema::messages::msg15<byte_type>,
    test_schema::messages::msg16<byte_type>,
    test_schema::messages::msg17<byte_type>,
    entry_wrapper<test_schema::schema::messages::msg14::group>,
    entry_wrapper<test_schema::schema::messages::msg15::group>,
    entry_wrapper<test_schema::schema::messages::msg16::group>,
    entry_wrapper<test_schema::schema::messages::msg17::group>>;

template<typename T>
using DefaultOffsetsTest = FieldsContainer<T>;

TYPED_TEST_SUITE(DefaultOffsetsTest, DefaultOffsetFieldContainers);

using ReferenceSemanticsDefaultOffsetFieldContainers = ::testing::Types<
    test_schema::types::composite_15<byte_type>,
    test_schema::types::composite_16<byte_type>,
    test_schema::types::composite_17<byte_type>,
    test_schema::types::composite_18<byte_type>,
    test_schema::messages::msg24<byte_type>,
    test_schema::messages::msg25<byte_type>,
    entry_wrapper<test_schema::schema::messages::msg24::group>,
    entry_wrapper<test_schema::schema::messages::msg25::group>>;

template<typename T>
using ReferenceSemanticsDefaultOffsetsTest = FieldsContainer<T>;

TYPED_TEST_SUITE(
    ReferenceSemanticsDefaultOffsetsTest,
    ReferenceSemanticsDefaultOffsetFieldContainers);

using CustomOffsetFieldContainers = ::testing::Types<
    test_schema::types::composite_7<byte_type>,
    test_schema::types::composite_8<byte_type>,
    test_schema::types::composite_9<byte_type>,
    test_schema::types::composite_12<byte_type>,
    test_schema::types::composite_13<byte_type>,
    test_schema::types::composite_14<byte_type>,
    test_schema::messages::msg18<byte_type>,
    test_schema::messages::msg19<byte_type>,
    test_schema::messages::msg20<byte_type>,
    test_schema::messages::msg21<byte_type>,
    entry_wrapper<test_schema::schema::messages::msg18::group>,
    entry_wrapper<test_schema::schema::messages::msg19::group>,
    entry_wrapper<test_schema::schema::messages::msg20::group>,
    entry_wrapper<test_schema::schema::messages::msg21::group>>;

template<typename T>
using ValueSemanticsCustomOffsetTest = FieldsContainer<T>;

TYPED_TEST_SUITE(ValueSemanticsCustomOffsetTest, CustomOffsetFieldContainers);

using CustomOffsetReferenceFieldContainers = ::testing::Types<
    test_schema::types::composite_10<byte_type>,
    test_schema::types::composite_11<byte_type>,
    test_schema::messages::msg22<byte_type>,
    test_schema::messages::msg23<byte_type>,
    entry_wrapper<test_schema::schema::messages::msg22::group>,
    entry_wrapper<test_schema::schema::messages::msg23::group>>;

template<typename T>
using ReferenceSemanticsCustomOffsetTest = FieldsContainer<T>;

TYPED_TEST_SUITE(
    ReferenceSemanticsCustomOffsetTest, CustomOffsetReferenceFieldContainers);

template<typename T>
enable_if_t<std::is_enum<T>::value, T> get_value_to_set(T /*enumeration*/)
{
    return T::Two;
}

template<typename T, typename = void_t<>>
struct has_value : std::false_type
{
};

template<typename T>
struct has_value<T, void_t<decltype(std::declval<T>().value())>>
    : std::true_type
{
};

template<typename T>
using is_numeric_type = has_value<T>;

template<typename T>
using is_integral_type = std::integral_constant<
    bool,
    has_value<T>::value && std::is_integral<typename T::value_type>::value>;

template<typename T>
using is_floating_point_type = std::integral_constant<
    bool,
    has_value<T>::value
        && std::is_floating_point<typename T::value_type>::value>;

template<typename T>
enable_if_t<is_integral_type<T>::value, T> get_value_to_set(T /*integral*/)
{
    return T{123};
}

template<typename T>
enable_if_t<is_floating_point_type<T>::value, T>
    get_value_to_set(T /*floating*/)
{
    return T{123.45};
}

template<typename T>
using is_set = typename std::
    integral_constant<bool, !(std::is_enum<T>::value || has_value<T>::value)>;

template<typename T>
enable_if_t<is_set<T>::value, T> get_value_to_set(T)
{
    return T{}.A(true).B(true);
}

template<typename T, typename = void>
struct underlying_type;

template<typename T>
using underlying_type_t = typename underlying_type<T>::type;

template<typename T>
struct underlying_type<T, enable_if_t<std::is_enum<T>::value>>
{
    using type = typename std::underlying_type<T>::type;
};

template<typename T>
struct underlying_type<T, enable_if_t<is_numeric_type<T>::value>>
{
    using type = typename T::value_type;
};

template<typename T>
struct underlying_type<T, enable_if_t<is_set<T>::value>>
{
    using type = decay_t<decltype(*std::declval<T>())>;
};

template<typename View>
byte_type* get_second_field_ptr(View v)
{
    return get_fields_start(v) + sizeof(v.first_field());
}

TYPED_TEST(
    DefaultOffsetsTest, FieldWithoutExplicitOffsetLocatedAfterPreviousOne)
{
    const auto& v = this->view;
    auto value_to_set = get_value_to_set(v.second_field());
    v.second_field(value_to_set);
    auto value_by_getter = v.second_field();
    using field_type = decay_t<decltype(v.second_field())>;
    using underlying_type = underlying_type_t<field_type>;
    auto ptr = get_second_field_ptr(v);

    auto value_by_memcpy = bit_cast<
        underlying_type,
        sbepp::schema_traits<test_schema::schema>::byte_order()>(ptr);

    ASSERT_EQ(value_to_set, value_by_getter);
    ASSERT_EQ(value_to_set, static_cast<field_type>(value_by_memcpy));
}

TYPED_TEST(
    DefaultOffsetsTest, FirstFieldWithoutExplicitOffsetLocatedAfterHeader)
{
    const auto& v = this->view;
    auto value_to_set = get_value_to_set(v.first_field());
    v.first_field(value_to_set);
    auto value_by_getter = v.first_field();
    using field_type = decay_t<decltype(v.first_field())>;
    using underlying_type = underlying_type_t<field_type>;
    auto ptr = get_fields_start(v);

    auto value_by_memcpy = bit_cast<
        underlying_type,
        sbepp::schema_traits<test_schema::schema>::byte_order()>(ptr);

    ASSERT_EQ(value_to_set, value_by_getter);
    ASSERT_EQ(value_to_set, static_cast<field_type>(value_by_memcpy));
}

TYPED_TEST(
    ReferenceSemanticsDefaultOffsetsTest,
    FieldWithoutExplicitOffsetLocatedAfterPreviousOne)
{
    const auto& v = this->view;

    auto first_field_address = sbepp::addressof(v.first_field());
    auto first_field_size = sbepp::size_bytes(v.first_field());
    auto second_field_address = sbepp::addressof(v.second_field());

    ASSERT_EQ(first_field_address + first_field_size, second_field_address);
}

TYPED_TEST(
    ReferenceSemanticsDefaultOffsetsTest,
    FirstFieldWithoutExplicitOffsetLocatedAfterHeader)
{
    const auto& v = this->view;
    auto first_field_address = sbepp::addressof(v.first_field());
    auto fields_start = get_fields_start(v);

    ASSERT_EQ(first_field_address, fields_start);
}

TYPED_TEST(
    ValueSemanticsCustomOffsetTest,
    CustomOffsetsRespectedForValueSemanticsTypes)
{
    static constexpr auto custom_offset = 20;
    const auto& v = this->view;
    auto value_to_set = get_value_to_set(v.field());
    v.field(value_to_set);
    auto value_by_getter = v.field();
    using field_type = decay_t<decltype(v.field())>;
    using underlying_type = underlying_type_t<field_type>;
    auto ptr = get_fields_start(v) + custom_offset;

    auto value_by_memcpy = bit_cast<
        underlying_type,
        sbepp::schema_traits<test_schema::schema>::byte_order()>(ptr);

    ASSERT_EQ(value_to_set, value_by_getter);
    ASSERT_EQ(value_to_set, static_cast<field_type>(value_by_memcpy));
}

TYPED_TEST(
    ReferenceSemanticsCustomOffsetTest,
    CustomOffsetsRespectedForReferenceSemanticsTypes)
{
    static constexpr auto custom_offset = 20;
    const auto& v = this->view;

    auto fields_start = get_fields_start(v);
    auto field_address = sbepp::addressof(v.field());

    ASSERT_EQ(field_address - fields_start, custom_offset);
}

using BigEndianFieldContainers = ::testing::Types<
    big_endian_schema::types::composite_1<byte_type>,
    big_endian_schema::types::composite_2<byte_type>,
    big_endian_schema::types::composite_3<byte_type>,
    big_endian_schema::types::composite_4<byte_type>,
    big_endian_schema::types::composite_5<byte_type>>;

template<typename T>
using BigEndianTest = FieldsContainer<T>;

TYPED_TEST_SUITE(BigEndianTest, BigEndianFieldContainers);

TYPED_TEST(BigEndianTest, CorrectlyHandlesOppositeEndianness)
{
    const auto& v = this->view;
    auto value_to_set = get_value_to_set(v.field());
    v.field(value_to_set);
    using field_type = decay_t<decltype(v.field())>;
    using underlying_type = underlying_type_t<field_type>;
    auto ptr = get_fields_start(v);

    auto value_by_memcpy = bit_cast<
        underlying_type,
        sbepp::schema_traits<big_endian_schema::schema>::byte_order()>(ptr);

    ASSERT_EQ(value_to_set, static_cast<field_type>(value_by_memcpy));
}

TEST(DataBigEndianTest, DataSizeFieldRespectsEndianness)
{
    std::array<byte_type, 512> buf{};
    big_endian_schema::messages::msg1<byte_type> msg{buf.data(), buf.size()};
    auto d = msg.data();
    static constexpr std::uint32_t new_size = 10;

    d.resize(new_size);

    auto size_ptr = sbepp::addressof(d);
    auto raw_size = bit_cast<
        std::uint32_t,
        sbepp::schema_traits<big_endian_schema::schema>::byte_order()>(
        size_ptr);

    ASSERT_EQ(new_size, raw_size);
}
} // namespace
