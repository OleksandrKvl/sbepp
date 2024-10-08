// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#if defined(USE_TOP_FILE)
#    include <test_schema/test_schema.hpp>
#    include <test_schema2/test_schema2.hpp>
#else
#    include <test_schema/messages/Msg1.hpp>
#    include <test_schema/messages/msg2.hpp>
#    include <test_schema/messages/msg8.hpp>
#    include <test_schema2/messages/msg1.hpp>
#endif

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <type_traits>

namespace
{
using byte_type = std::uint8_t;
using message_t = test_schema::messages::Msg1<byte_type>;
using const_message_t = test_schema::messages::Msg1<const byte_type>;
using schema_tag = test_schema::schema;
using message_tag = schema_tag::messages::Msg1;

#if SBEPP_SIZE_CHECKS_ENABLED
STATIC_ASSERT(sizeof(message_t) == sizeof(void*) * 2);
#else
STATIC_ASSERT(sizeof(message_t) == sizeof(void*));
#endif

STATIC_ASSERT_V(std::is_nothrow_default_constructible<message_t>);
STATIC_ASSERT_V(std::is_trivially_copy_constructible<message_t>);
STATIC_ASSERT_V(std::is_trivially_copy_assignable<message_t>);
STATIC_ASSERT_V(std::is_trivially_move_constructible<message_t>);
STATIC_ASSERT_V(std::is_trivially_move_assignable<message_t>);
STATIC_ASSERT_V(std::is_trivially_destructible<message_t>);

STATIC_ASSERT(sbepp::is_message<message_t>::value);

#if SBEPP_HAS_INLINE_VARS
STATIC_ASSERT(sbepp::is_message_v<message_t>);
#endif

#if SBEPP_HAS_CONCEPTS
STATIC_ASSERT(sbepp::message<message_t>);
#endif

class MessageTest : public ::testing::Test
{
public:
    std::array<byte_type, 512> buf{};
};

TEST_F(MessageTest, DefaultConstructedToNullptr)
{
    message_t m;

    ASSERT_EQ(sbepp::addressof(m), nullptr);
}

TEST_F(MessageTest, CanBeConstructedFromBeginEndPointers)
{
    message_t m{buf.data(), buf.data() + buf.size()};

    ASSERT_EQ(sbepp::addressof(m), buf.data());
    STATIC_ASSERT_V(std::is_nothrow_constructible<
                    message_t,
                    decltype(buf.data()),
                    decltype(buf.data())>);
}

TEST_F(MessageTest, CanBeConstructedFromPointerAndSize)
{
    message_t m{buf.data(), buf.size()};

    ASSERT_EQ(sbepp::addressof(m), buf.data());
    STATIC_ASSERT_V(std::is_nothrow_constructible<
                    message_t,
                    decltype(buf.data()),
                    decltype(buf.size())>);
}

TEST_F(MessageTest, CopyAndMoveCopyPointer)
{
    message_t m{buf.data(), buf.size()};
    auto m2{m};

    ASSERT_EQ(sbepp::addressof(m2), sbepp::addressof(m));

    message_t m3;
    m2 = m3;

    ASSERT_EQ(sbepp::addressof(m2), sbepp::addressof(m3));

    auto m4{std::move(m)};

    ASSERT_EQ(sbepp::addressof(m4), sbepp::addressof(m));

    m = std::move(m3);

    ASSERT_EQ(sbepp::addressof(m), sbepp::addressof(m3));
}

TEST_F(MessageTest, CanBeConstructedFromLessConstType)
{
    STATIC_ASSERT_V(!std::is_nothrow_constructible<message_t, const_message_t>);
    STATIC_ASSERT_V(std::is_nothrow_constructible<const_message_t, message_t>);

    message_t m{buf.data(), buf.size()};
    const_message_t m2{m};

    ASSERT_EQ(sbepp::addressof(m2), sbepp::addressof(m));

    m2 = message_t{};

    ASSERT_EQ(sbepp::addressof(m2), nullptr);
}

TEST_F(MessageTest, SizeBytesReturnsMessageSize)
{
    test_schema::messages::msg8<byte_type> m{
        buf.data(), buf.data() + buf.size()};
    sbepp::fill_message_header(m);
    auto c2 = m.composite2();

    const auto correct_size =
        sbepp::addressof(c2) + sbepp::size_bytes(c2) - sbepp::addressof(m);

    ASSERT_EQ(sbepp::size_bytes(m), correct_size);
    STATIC_ASSERT(noexcept(sbepp::size_bytes(m)));
}

TEST_F(MessageTest, SizeBytesTakesBlockLengthIntoAccount)
{
    test_schema::messages::msg8<byte_type> m{
        buf.data(), buf.data() + buf.size()};
    auto header = sbepp::fill_message_header(m);
    auto c2 = m.composite2();

    const auto correct_size =
        sbepp::addressof(c2) + sbepp::size_bytes(c2) - sbepp::addressof(m);

    ASSERT_EQ(sbepp::size_bytes(m), correct_size);

    static constexpr auto block_length_increase = 2;
    header.blockLength(*header.blockLength() + block_length_increase);

    ASSERT_EQ(sbepp::size_bytes(m), correct_size + block_length_increase);
}

TEST_F(MessageTest, SizeBytesIncludesGroupsAndDataSizes)
{
    test_schema::messages::msg2<byte_type> m{
        buf.data(), buf.data() + buf.size()};
    auto header = sbepp::fill_message_header(m);
    auto group = m.group();
    sbepp::fill_group_header(group, 0);
    auto data = m.data();
    data.resize(3);

    const auto correct_size = sbepp::size_bytes(header) + *header.blockLength()
                              + sbepp::size_bytes(group)
                              + sbepp::size_bytes(data);

    ASSERT_EQ(sbepp::size_bytes(m), correct_size);
}

TEST_F(MessageTest, GetHeaderReturnsMessageHeader)
{
    message_t m{buf.data(), buf.size()};
    const_message_t m2{m};
    auto h = sbepp::get_header(m);
    auto h2 = sbepp::get_header(m2);
    (void)h.blockLength();
    (void)h.schemaId();
    (void)h.templateId();
    (void)h.version();
    (void)h2.blockLength();
    (void)h2.schemaId();
    (void)h2.templateId();
    (void)h2.version();

    STATIC_ASSERT_V(
        std::is_same<
            decltype(h),
            sbepp::schema_traits<test_schema::schema>::header_type<byte_type>>);
    STATIC_ASSERT_V(std::is_same<
                    decltype(h2),
                    sbepp::schema_traits<test_schema::schema>::header_type<
                        const byte_type>>);
    ASSERT_EQ(sbepp::addressof(h), sbepp::addressof(h2));
    ASSERT_EQ(sbepp::addressof(h), sbepp::addressof(m));
    STATIC_ASSERT(noexcept(sbepp::get_header(m)));
    STATIC_ASSERT(noexcept(sbepp::get_header(m2)));
}

TEST_F(
    MessageTest, FillMessageHeaderSetsBlockLengthTemplateIdSchemaIdAndVersion)
{
    message_t m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);

    ASSERT_EQ(
        header.blockLength(),
        sbepp::message_traits<message_tag>::block_length());
    ASSERT_EQ(header.schemaId(), sbepp::schema_traits<schema_tag>::id());
    ASSERT_EQ(header.templateId(), sbepp::message_traits<message_tag>::id());
    ASSERT_EQ(header.version(), sbepp::schema_traits<schema_tag>::version());
    STATIC_ASSERT(noexcept(sbepp::fill_message_header(m)));
}

struct fill_message_header
{
    template<typename T>
    auto operator()(T obj) -> decltype(sbepp::fill_message_header(obj))
    {
    }
};

TEST_F(MessageTest, FillMessageHeaderNeedsNonConstByteType)
{
    STATIC_ASSERT(
        !sbepp::test::utils::
            is_invocable<fill_message_header, const_message_t>::value);
}

TEST_F(
    MessageTest, FillMessageHeaderSetsNumGroupsAndNumVarDataFieldsIfTheyExist)
{
    using message_t = test_schema2::messages::msg1<byte_type>;
    using schema_tag = test_schema2::schema;
    using message_tag = schema_tag::messages::msg1;

    message_t m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    static constexpr auto num_groups = 1;
    static constexpr auto num_var_data_fields = 1;

    ASSERT_EQ(
        header.blockLength(),
        sbepp::message_traits<message_tag>::block_length());
    ASSERT_EQ(header.schemaId(), sbepp::schema_traits<schema_tag>::id());
    ASSERT_EQ(header.templateId(), sbepp::message_traits<message_tag>::id());
    ASSERT_EQ(header.version(), sbepp::schema_traits<schema_tag>::version());
    ASSERT_EQ(header.numGroups(), num_groups);
    ASSERT_EQ(header.numVarDataFields(), num_var_data_fields);
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test()
{
    std::array<byte_type, 512> buf{};
    message_t m;
    m = message_t{buf.data(), buf.size()};
    m = m;
    auto m2 = std::move(m);
    (void)m2;

    sbepp::fill_message_header(m);
    sbepp::addressof(m);
    sbepp::get_header(m);

    sbepp::size_bytes(m);
    return buf;
}

constexpr auto buf = constexpr_test();
#endif
} // namespace
