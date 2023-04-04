// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#ifdef USE_TOP_FILE
#    include <test_schema/test_schema.hpp>
#else
#    include <test_schema/messages/msg2.hpp>
#    include <test_schema/messages/msg3.hpp>
#    include <test_schema/messages/msg4.hpp>
#    include <test_schema/messages/msg5.hpp>
#    include <test_schema/messages/msg6.hpp>
#    include <test_schema/messages/msg7.hpp>
#    include <test_schema/messages/msg8.hpp>
#    include <test_schema/messages/msg9.hpp>
#    include <test_schema/messages/msg10.hpp>
#    include <test_schema/messages/msg11.hpp>
#    include <test_schema/messages/msg12.hpp>
#    include <test_schema/messages/msg13.hpp>
#endif

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <iterator>
#include <array>
#include <limits>
#include <type_traits>
#include <random>

namespace
{
using byte_type = std::uint8_t;
using cursor_t = sbepp::cursor<byte_type>;
using const_cursor_t = sbepp::cursor<const byte_type>;

class CursorTest : public ::testing::Test
{
public:
    using field_tag = test_schema::schema::messages::msg4::number1;
    using enum_tag = test_schema::schema::messages::msg6::enumeration1;
    using set_tag = test_schema::schema::messages::msg7::set1;

    std::array<byte_type, 1024> buf{};
    cursor_t c;
    int number_value{123};
    char char_value{'x'};
    test_schema::types::numbers_enum enum_value =
        test_schema::types::numbers_enum::Two;
    test_schema::types::options_set set_value =
        test_schema::types::options_set{}.B(true);
    std::size_t magic_block_length{42};
    std::size_t header_size = sbepp::composite_traits<
        test_schema::schema::types::messageHeader>::size_bytes();
};

using CursorDeathTest = CursorTest;

TEST_F(CursorTest, InitCursorSetsItToMessageHeaderEnd)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    const auto header = sbepp::get_header(m);

    c = sbepp::init_cursor(m);

    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(noexcept(sbepp::init_cursor(m)));
    STATIC_ASSERT_V(std::is_same<decltype(c), cursor_t>);
}

TEST_F(CursorTest, InitCursorSetsItToGroupHeaderEnd)
{
    using group_tag =
        test_schema::schema::messages::msg3::nested_group::flat_group;
    using group_t = sbepp::group_traits<group_tag>::value_type<byte_type>;
    group_t g{buf.data(), buf.size()};
    sbepp::fill_group_header(g, 0);
    const auto header = sbepp::get_header(g);

    c = sbepp::init_cursor(g);

    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(noexcept(sbepp::init_cursor(g)));
    STATIC_ASSERT_V(std::is_same<decltype(c), cursor_t>);
}

TEST_F(CursorTest, InitConstCursorSetsItToMessageHeaderEnd)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    const auto header = sbepp::get_header(m);

    auto c = sbepp::init_const_cursor(m);

    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(noexcept(sbepp::init_const_cursor(m)));
    STATIC_ASSERT_V(std::is_same<decltype(c), const_cursor_t>);
}

TEST_F(CursorTest, InitConstCursorSetsItToGroupHeaderEnd)
{
    using group_tag =
        test_schema::schema::messages::msg3::nested_group::flat_group;
    using group_t = sbepp::group_traits<group_tag>::value_type<byte_type>;
    group_t g{buf.data(), buf.size()};
    sbepp::fill_group_header(g, 0);
    const auto header = sbepp::get_header(g);

    auto c = sbepp::init_const_cursor(g);

    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(noexcept(sbepp::init_const_cursor(g)));
    STATIC_ASSERT_V(std::is_same<decltype(c), const_cursor_t>);
}

TEST_F(CursorTest, NonLastTypeFieldReadMovesCursorToItsEnd)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.number1(number_value);

    auto n = m.number1(c);

    ASSERT_EQ(n, number_value);
    ASSERT_EQ(
        c.pointer(),
        old_ptr + sbepp::field_traits<field_tag>::offset() + sizeof(n));
    STATIC_ASSERT(noexcept(m.number1(c)));
}

TEST_F(CursorTest, NonLastTypeFieldWriteMovesCursorToItsEnd)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();

    m.number1(number_value, c);

    auto n = m.number1();
    ASSERT_EQ(n, number_value);
    ASSERT_EQ(
        c.pointer(),
        old_ptr + sbepp::field_traits<field_tag>::offset() + sizeof(n));
    STATIC_ASSERT(noexcept(m.number1(number_value, c)));
}

TEST_F(CursorTest, NonLastTypeFieldReadInitInitsAndMovesCursorToItsEnd)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);
    m.number1(number_value);

    auto n = m.number1(sbepp::cursor_ops::init(c));

    ASSERT_EQ(n, number_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<field_tag>::offset() + sizeof(n));
    STATIC_ASSERT(noexcept(m.number1(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, NonLastTypeFieldWriteInitInitsAndMovesCursorToItsEnd)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);

    m.number1(number_value, sbepp::cursor_ops::init(c));

    auto n = m.number1();
    ASSERT_EQ(n, number_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<field_tag>::offset() + sizeof(n));
    STATIC_ASSERT(
        noexcept(m.number1(number_value, sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, NonLastTypeFieldReadInitDontMoveInitsCursorToPrevFieldEnd)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);
    m.number1(number_value);

    auto n = m.number1(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(n, number_value);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(noexcept(m.number1(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, NonLastTypeFieldWriteInitDontMoveInitsCursorToPrevFieldEnd)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);

    m.number1(number_value, sbepp::cursor_ops::init_dont_move(c));

    auto n = m.number1();
    ASSERT_EQ(n, number_value);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(noexcept(
        m.number1(number_value, sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, NonLastTypeFieldReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.number1(number_value);

    auto n = m.number1(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(n, number_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.number1(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, NonLastTypeFieldWriteDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();

    m.number1(number_value, sbepp::cursor_ops::dont_move(c));

    auto n = m.number1();
    ASSERT_EQ(n, number_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(
        noexcept(m.number1(number_value, sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, NonLastTypeFieldSkipMovesCursorToItsEnd)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();

    m.number1(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(
        c.pointer(),
        old_ptr + sbepp::field_traits<field_tag>::offset()
            + sizeof(m.number1()));
    STATIC_ASSERT(noexcept(m.number1(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.number1(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorTest, LastTypeFieldReadMovesCursorToBlockLength)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.number1(sbepp::cursor_ops::skip(c));
    m.number2(number_value);

    auto n = m.number2(c);

    ASSERT_EQ(n, number_value);
    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.number2(c)));
}

TEST_F(CursorTest, LastTypeFieldWriteMovesCursorToBlockLength)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.number1(sbepp::cursor_ops::skip(c));

    m.number2(number_value, c);

    auto n = m.number2();
    ASSERT_EQ(n, number_value);
    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.number2(number_value, c)));
}

TEST_F(CursorTest, LastTypeFieldReadInitInitsAndMovesCursorToBlockLength)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    m.number2(number_value);

    auto n = m.number2(sbepp::cursor_ops::init(c));

    ASSERT_EQ(n, number_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + magic_block_length);
    STATIC_ASSERT(noexcept(m.number2(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, LastTypeFieldWriteInitInitsAndMovesCursorToBlockLength)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);

    m.number2(number_value, sbepp::cursor_ops::init(c));

    auto n = m.number2();
    ASSERT_EQ(n, number_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + magic_block_length);
    STATIC_ASSERT(
        noexcept(m.number2(number_value, sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, LastTypeFieldReadInitDontMoveInitsCursorToPrevFieldEnd)
{
    using prev_field_tag = test_schema::schema::messages::msg4::number1;
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    m.number2(number_value);

    auto n = m.number2(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(n, number_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<prev_field_tag>::offset()
            + sizeof(sbepp::field_traits<prev_field_tag>::value_type));
    STATIC_ASSERT(noexcept(m.number2(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, LastTypeFieldWriteInitDontMoveInitsCursorToPrevFieldEnd)
{
    using prev_field_tag = test_schema::schema::messages::msg4::number1;
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);

    m.number2(number_value, sbepp::cursor_ops::init_dont_move(c));

    auto n = m.number2();
    ASSERT_EQ(n, number_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<prev_field_tag>::offset()
            + sizeof(sbepp::field_traits<prev_field_tag>::value_type));
    STATIC_ASSERT(noexcept(
        m.number2(number_value, sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, LastTypeFieldReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    m.number2(number_value);
    m.number1(sbepp::cursor_ops::skip(c));
    auto old_ptr = c.pointer();

    auto n = m.number2(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(n, number_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.number2(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, LastTypeFieldWriteDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    m.number1(sbepp::cursor_ops::skip(c));
    auto old_ptr = c.pointer();

    m.number2(number_value, sbepp::cursor_ops::dont_move(c));

    auto n = m.number2();
    ASSERT_EQ(n, number_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(
        noexcept(m.number2(number_value, sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, LastTypeFieldSkipMovesCursorToBlockLength)
{
    test_schema::messages::msg4<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.number1(sbepp::cursor_ops::skip(c));

    m.number2(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.number2(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.number2(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorDeathTest, TypeFieldAccessorsTerminateIfBufferIsNotEnough)
{
    // buffer is only enough to hold the header
    test_schema::messages::msg4<byte_type> m{buf.data(), header_size};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);

    ASSERT_DEATH({ m.number1(c); }, ".*");
    ASSERT_DEATH({ m.number1(1, c); }, ".*");
    ASSERT_DEATH({ m.number1(sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.number1(1, sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.number1(sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.number1(1, sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.number1(sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.number1(1, sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.number1(sbepp::cursor_ops::skip(c)); }, ".*");

    ASSERT_DEATH({ m.number2(c); }, ".*");
    ASSERT_DEATH({ m.number2(1, c); }, ".*");
    ASSERT_DEATH({ m.number2(sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.number2(1, sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.number2(sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.number2(1, sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.number2(sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.number2(1, sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.number2(sbepp::cursor_ops::skip(c)); }, ".*");
}

TEST_F(CursorTest, NonLastArrayFieldReadMovesCursorToItsEnd)
{
    test_schema::messages::msg5<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    m.array1()[0] = char_value;

    auto array = m.array1(c);

    ASSERT_EQ(array[0], char_value);
    ASSERT_EQ(c.pointer(), sbepp::addressof(array) + sbepp::size_bytes(array));
    STATIC_ASSERT(noexcept(m.array1(c)));
}

TEST_F(CursorTest, NonLastArrayFieldReadInitInitsAndMovesCursorToItsEnd)
{
    test_schema::messages::msg5<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    m.array1()[0] = char_value;

    auto array = m.array1(sbepp::cursor_ops::init(c));

    ASSERT_EQ(array[0], char_value);
    ASSERT_EQ(c.pointer(), sbepp::addressof(array) + sbepp::size_bytes(array));
    STATIC_ASSERT(noexcept(m.array1(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, NonLastArrayFieldReadInitDontMoveInitsCursorToPrevFieldEnd)
{
    test_schema::messages::msg5<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    m.array1()[0] = char_value;

    auto array = m.array1(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(array[0], char_value);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(noexcept(m.array1(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, NonLastArrayFieldReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg5<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.array1()[0] = char_value;

    auto array = m.array1(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(array[0], char_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.array1(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, NonLastArrayFieldSkipMovesCursorToItsEnd)
{
    test_schema::messages::msg5<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto array = m.array1();

    m.array1(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(c.pointer(), sbepp::addressof(array) + sbepp::size_bytes(array));
    STATIC_ASSERT(noexcept(m.array1(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.array1(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorTest, LastArrayFieldReadMovesCursorToBlockLength)
{
    test_schema::messages::msg5<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.array1(sbepp::cursor_ops::skip(c));
    m.array2()[0] = char_value;

    auto array = m.array2(c);

    ASSERT_EQ(array[0], char_value);
    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.array2(c)));
}

TEST_F(CursorTest, LastArrayFieldReadInitInitsAndMovesCursorToBlockLength)
{
    test_schema::messages::msg5<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    m.array2()[0] = char_value;

    auto array = m.array2(sbepp::cursor_ops::init(c));

    ASSERT_EQ(array[0], char_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + magic_block_length);
    STATIC_ASSERT(noexcept(m.array2(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, LastArrayFieldReadInitDontMoveInitsCursorToPrevFieldEnd)
{
    test_schema::messages::msg5<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    m.array2()[0] = char_value;
    auto prev = m.array1();

    auto array = m.array2(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(array[0], char_value);
    ASSERT_EQ(c.pointer(), sbepp::addressof(prev) + sbepp::size_bytes(prev));
    STATIC_ASSERT(noexcept(m.array2(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, LastArrayFieldReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg5<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    m.array2()[0] = char_value;
    m.array1(sbepp::cursor_ops::skip(c));
    auto old_ptr = c.pointer();

    auto array = m.array2(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(array[0], char_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.array2(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, LastArrayFieldSkipMovesCursorToBlockLength)
{
    test_schema::messages::msg5<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.array1(sbepp::cursor_ops::skip(c));

    m.array2(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.array2(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.array2(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorDeathTest, ArrayFieldAccessorsTerminateIfBufferIsNotEnough)
{
    // buffer is only enough to hold the header
    test_schema::messages::msg5<byte_type> m{buf.data(), header_size};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);

    ASSERT_DEATH({ m.array1(c); }, ".*");
    ASSERT_DEATH({ m.array1(sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.array1(sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.array1(sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.array1(sbepp::cursor_ops::skip(c)); }, ".*");

    ASSERT_DEATH({ m.array2(c); }, ".*");
    ASSERT_DEATH({ m.array2(sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.array2(sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.array2(sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.array2(sbepp::cursor_ops::skip(c)); }, ".*");
}

TEST_F(CursorTest, NonLastEnumFieldReadMovesCursorToItsEnd)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.enumeration1(enum_value);

    auto e = m.enumeration1(c);

    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(
        c.pointer(),
        old_ptr + sbepp::field_traits<enum_tag>::offset() + sizeof(e));
    STATIC_ASSERT(noexcept(m.enumeration1(c)));
}

TEST_F(CursorTest, NonLastEnumFieldWriteMovesCursorToItsEnd)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();

    m.enumeration1(enum_value, c);

    auto e = m.enumeration1();
    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(
        c.pointer(),
        old_ptr + sbepp::field_traits<enum_tag>::offset() + sizeof(e));
    STATIC_ASSERT(noexcept(m.enumeration1(enum_value, c)));
}

TEST_F(CursorTest, NonLastEnumFieldReadInitInitsAndMovesCursorToItsEnd)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);
    m.enumeration1(enum_value);

    auto e = m.enumeration1(sbepp::cursor_ops::init(c));

    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<enum_tag>::offset() + sizeof(e));
    STATIC_ASSERT(noexcept(m.enumeration1(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, NonLastEnumFieldWriteInitInitsAndMovesCursorToItsEnd)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);

    m.enumeration1(enum_value, sbepp::cursor_ops::init(c));

    auto e = m.enumeration1();
    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<enum_tag>::offset() + sizeof(e));
    STATIC_ASSERT(
        noexcept(m.enumeration1(enum_value, sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, NonLastEnumFieldReadInitDontMoveInitsCursorToPrevFieldEnd)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);
    m.enumeration1(enum_value);

    auto e = m.enumeration1(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(
        noexcept(m.enumeration1(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, NonLastEnumFieldWriteInitDontMoveInitsCursorToPrevFieldEnd)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);

    m.enumeration1(enum_value, sbepp::cursor_ops::init_dont_move(c));

    auto e = m.enumeration1();
    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(noexcept(
        m.enumeration1(enum_value, sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, NonLastEnumFieldReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.enumeration1(enum_value);

    auto e = m.enumeration1(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.enumeration1(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, NonLastEnumFieldWriteDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();

    m.enumeration1(enum_value, sbepp::cursor_ops::dont_move(c));

    auto e = m.enumeration1();
    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(
        noexcept(m.enumeration1(enum_value, sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, NonLastEnumFieldSkipMovesCursorToItsEnd)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();

    m.enumeration1(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(
        c.pointer(),
        old_ptr + sbepp::field_traits<enum_tag>::offset()
            + sizeof(m.enumeration1()));
    STATIC_ASSERT(noexcept(m.enumeration1(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.enumeration1(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorTest, LastEnumFieldReadMovesCursorToBlockLength)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.enumeration1(sbepp::cursor_ops::skip(c));
    m.enumeration2(enum_value);

    auto e = m.enumeration2(c);

    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.enumeration2(c)));
}

TEST_F(CursorTest, LastEnumFieldWriteMovesCursorToBlockLength)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.enumeration1(sbepp::cursor_ops::skip(c));

    m.enumeration2(enum_value, c);

    auto e = m.enumeration2();
    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.enumeration2(enum_value, c)));
}

TEST_F(CursorTest, LastEnumFieldReadInitInitsAndMovesCursorToBlockLength)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    m.enumeration2(enum_value);

    auto e = m.enumeration2(sbepp::cursor_ops::init(c));

    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + magic_block_length);
    STATIC_ASSERT(noexcept(m.enumeration2(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, LastEnumFieldWriteInitInitsAndMovesCursorToBlockLength)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);

    m.enumeration2(enum_value, sbepp::cursor_ops::init(c));

    auto e = m.enumeration2();
    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + magic_block_length);
    STATIC_ASSERT(
        noexcept(m.enumeration2(enum_value, sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, LastEnumFieldReadInitDontMoveInitsToPrevFieldEnd)
{
    using prev_field_tag = test_schema::schema::messages::msg6::enumeration1;
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    m.enumeration2(enum_value);

    auto e = m.enumeration2(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<prev_field_tag>::offset()
            + sizeof(sbepp::field_traits<prev_field_tag>::value_type));
    STATIC_ASSERT(
        noexcept(m.enumeration2(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, LastEnumFieldWriteInitDontMoveInitsCursorToPrevFieldEnd)
{
    using prev_field_tag = test_schema::schema::messages::msg6::enumeration1;
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);

    m.enumeration2(enum_value, sbepp::cursor_ops::init_dont_move(c));

    auto e = m.enumeration2();
    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<prev_field_tag>::offset()
            + sizeof(sbepp::field_traits<prev_field_tag>::value_type));
    STATIC_ASSERT(noexcept(
        m.enumeration2(enum_value, sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, LastEnumFieldReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    m.enumeration2(enum_value);
    m.enumeration1(sbepp::cursor_ops::skip(c));
    auto old_ptr = c.pointer();

    auto e = m.enumeration2(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.enumeration2(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, LastEnumFieldWriteDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    m.enumeration1(sbepp::cursor_ops::skip(c));
    auto old_ptr = c.pointer();

    m.enumeration2(enum_value, sbepp::cursor_ops::dont_move(c));

    auto e = m.enumeration2();
    ASSERT_EQ(e, enum_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(
        noexcept(m.enumeration2(enum_value, sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, LastEnumFieldSkipMovesCursorToBlockLength)
{
    test_schema::messages::msg6<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.enumeration1(sbepp::cursor_ops::skip(c));

    m.enumeration2(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.enumeration2(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.enumeration2(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorDeathTest, EnumFieldAccessorsTerminateIfBufferIsNotEnough)
{
    // buffer is only enough to hold the header
    test_schema::messages::msg6<byte_type> m{buf.data(), header_size};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);

    ASSERT_DEATH({ m.enumeration1(c); }, ".*");
    ASSERT_DEATH({ m.enumeration1(enum_value, c); }, ".*");
    ASSERT_DEATH({ m.enumeration1(sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH(
        { m.enumeration1(enum_value, sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.enumeration1(sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH(
        { m.enumeration1(enum_value, sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH(
        { m.enumeration1(sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH(
        { m.enumeration1(enum_value, sbepp::cursor_ops::init_dont_move(c)); },
        ".*");
    ASSERT_DEATH({ m.enumeration1(sbepp::cursor_ops::skip(c)); }, ".*");

    ASSERT_DEATH({ m.enumeration2(c); }, ".*");
    ASSERT_DEATH({ m.enumeration2(enum_value, c); }, ".*");
    ASSERT_DEATH({ m.enumeration2(sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH(
        { m.enumeration2(enum_value, sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.enumeration2(sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH(
        { m.enumeration2(enum_value, sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH(
        { m.enumeration2(sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH(
        { m.enumeration2(enum_value, sbepp::cursor_ops::init_dont_move(c)); },
        ".*");
    ASSERT_DEATH({ m.enumeration2(sbepp::cursor_ops::skip(c)); }, ".*");
}

TEST_F(CursorTest, NonLastSetFieldReadMovesCursorToItsEnd)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.set1(set_value);

    auto s = m.set1(c);

    ASSERT_EQ(s, set_value);
    ASSERT_EQ(
        c.pointer(),
        old_ptr + sbepp::field_traits<set_tag>::offset() + sizeof(s));
    STATIC_ASSERT(noexcept(m.set1(c)));
}

TEST_F(CursorTest, NonLastSetFieldWriteMovesCursorToItsEnd)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();

    m.set1(set_value, c);

    auto s = m.set1();
    ASSERT_EQ(s, set_value);
    ASSERT_EQ(
        c.pointer(),
        old_ptr + sbepp::field_traits<set_tag>::offset() + sizeof(s));
    STATIC_ASSERT(noexcept(m.set1(set_value, c)));
}

TEST_F(CursorTest, NonLastSetFieldReadInitInitsAndMovesCursorToItsEnd)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);
    m.set1(set_value);

    auto s = m.set1(sbepp::cursor_ops::init(c));

    ASSERT_EQ(s, set_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<set_tag>::offset() + sizeof(s));
    STATIC_ASSERT(noexcept(m.set1(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, NonLastSetFieldWriteInitInitsAndMovesCursorToItsEnd)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);

    m.set1(set_value, sbepp::cursor_ops::init(c));

    auto s = m.set1();
    ASSERT_EQ(s, set_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<set_tag>::offset() + sizeof(s));
    STATIC_ASSERT(noexcept(m.set1(set_value, sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, NonLastSetFieldReadInitDontMoveInitsCursorToPrevFieldEnd)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);
    m.set1(set_value);

    auto s = m.set1(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(s, set_value);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(noexcept(m.set1(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, NonLastSetFieldWriteInitDontMoveInitsCursorToPrevFieldEnd)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto header = sbepp::get_header(m);

    m.set1(set_value, sbepp::cursor_ops::init_dont_move(c));

    auto s = m.set1();
    ASSERT_EQ(s, set_value);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(
        noexcept(m.set1(set_value, sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, NonLastSetFieldReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.set1(set_value);

    auto s = m.set1(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(s, set_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.set1(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, NonLastSetFieldWriteDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();

    m.set1(set_value, sbepp::cursor_ops::dont_move(c));

    auto s = m.set1();
    ASSERT_EQ(s, set_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.set1(set_value, sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, NonLastSetFieldSkipMovesCursorToItsEnd)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();

    m.set1(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(
        c.pointer(),
        old_ptr + sbepp::field_traits<set_tag>::offset() + sizeof(m.set1()));
    STATIC_ASSERT(noexcept(m.set1(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(std::is_void<decltype(m.set1(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorTest, LastSetFieldReadMovesCursorToBlockLength)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.set1(sbepp::cursor_ops::skip(c));
    m.set2(set_value);

    auto s = m.set2(c);

    ASSERT_EQ(s, set_value);
    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.set2(c)));
}

TEST_F(CursorTest, LastSetFieldWriteMovesCursorToBlockLength)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.set1(sbepp::cursor_ops::skip(c));

    m.set2(set_value, c);

    auto s = m.set2();
    ASSERT_EQ(s, set_value);
    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.set2(set_value, c)));
}

TEST_F(CursorTest, LastSetFieldReadInitInitsAndMovesCursorToBlockLength)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    m.set2(set_value);

    auto s = m.set2(sbepp::cursor_ops::init(c));

    ASSERT_EQ(s, set_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + magic_block_length);
    STATIC_ASSERT(noexcept(m.set2(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, LastSetFieldWriteInitInitsAndMovesCursorToBlockLength)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);

    m.set2(set_value, sbepp::cursor_ops::init(c));

    auto s = m.set2();
    ASSERT_EQ(s, set_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + magic_block_length);
    STATIC_ASSERT(noexcept(m.set2(set_value, sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, LastSetFieldReadInitDontMoveInitsCursorToPrevFieldEnd)
{
    using prev_field_tag = test_schema::schema::messages::msg7::set1;
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    m.set2(set_value);

    auto s = m.set2(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(s, set_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<prev_field_tag>::offset()
            + sizeof(sbepp::field_traits<prev_field_tag>::value_type));
    STATIC_ASSERT(noexcept(m.set2(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, LastSetFieldWriteInitDontMoveInitsCursorToPrevFieldEnd)
{
    using prev_field_tag = test_schema::schema::messages::msg7::set1;
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);

    m.set2(set_value, sbepp::cursor_ops::init_dont_move(c));

    auto s = m.set2();
    ASSERT_EQ(s, set_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + sbepp::field_traits<prev_field_tag>::offset()
            + sizeof(sbepp::field_traits<prev_field_tag>::value_type));
    STATIC_ASSERT(
        noexcept(m.set2(set_value, sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, LastSetFieldReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    m.set2(set_value);
    m.set1(sbepp::cursor_ops::skip(c));
    auto old_ptr = c.pointer();

    auto s = m.set2(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(s, set_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.set2(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, LastSetFieldWriteDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    m.set1(sbepp::cursor_ops::skip(c));
    auto old_ptr = c.pointer();

    m.set2(set_value, sbepp::cursor_ops::dont_move(c));

    auto s = m.set2();
    ASSERT_EQ(s, set_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.set2(set_value, sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, LastSetFieldSkipMovesCursorToBlockLength)
{
    test_schema::messages::msg7<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.set1(sbepp::cursor_ops::skip(c));

    m.set2(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.set2(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(std::is_void<decltype(m.set2(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorDeathTest, SetFieldAccessorsTerminateIfBufferIsNotEnough)
{
    // buffer is only enough to hold the header
    test_schema::messages::msg7<byte_type> m{buf.data(), header_size};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);

    ASSERT_DEATH({ m.set1(c); }, ".*");
    ASSERT_DEATH({ m.set1(set_value, c); }, ".*");
    ASSERT_DEATH({ m.set1(sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.set1(set_value, sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.set1(sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.set1(set_value, sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.set1(sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH(
        { m.set1(set_value, sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.set1(sbepp::cursor_ops::skip(c)); }, ".*");

    ASSERT_DEATH({ m.set2(c); }, ".*");
    ASSERT_DEATH({ m.set2(set_value, c); }, ".*");
    ASSERT_DEATH({ m.set2(sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.set2(set_value, sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.set2(sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.set2(set_value, sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.set2(sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH(
        { m.set2(set_value, sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.set2(sbepp::cursor_ops::skip(c)); }, ".*");
}

TEST_F(CursorTest, NonLastCompositeFieldReadMovesCursorToItsEnd)
{
    test_schema::messages::msg8<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    m.composite1().x(number_value);

    auto composite = m.composite1(c);

    ASSERT_EQ(composite.x(), number_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(composite) + sbepp::size_bytes(composite));
    STATIC_ASSERT(noexcept(m.composite1(c)));
}

TEST_F(CursorTest, NonLastCompositeFieldReadInitInitsAndMovesCursorToItsEnd)
{
    test_schema::messages::msg8<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    m.composite1().x(number_value);

    auto composite = m.composite1(sbepp::cursor_ops::init(c));

    ASSERT_EQ(composite.x(), number_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(composite) + sbepp::size_bytes(composite));
    STATIC_ASSERT(noexcept(m.composite1(sbepp::cursor_ops::init(c))));
}

TEST_F(
    CursorTest, NonLastCompositeFieldReadInitDontMoveInitsCursorToPrevFieldEnd)
{
    test_schema::messages::msg8<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    m.composite1().x(number_value);

    auto composite = m.composite1(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(composite.x(), number_value);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    STATIC_ASSERT(noexcept(m.composite1(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, NonLastCompositeFieldReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg8<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.composite1().x(number_value);

    auto composite = m.composite1(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(composite.x(), number_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.composite1(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, NonLastCompositeFieldSkipMovesCursorToItsEnd)
{
    test_schema::messages::msg8<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    auto composite = m.composite1();

    m.composite1(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(composite) + sbepp::size_bytes(composite));
    STATIC_ASSERT(noexcept(m.composite1(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.composite1(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorTest, LastCompositeFieldReadMovesCursorToBlockLength)
{
    test_schema::messages::msg8<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.composite1(sbepp::cursor_ops::skip(c));
    m.composite2().x(number_value);

    auto composite = m.composite2(c);

    ASSERT_EQ(composite.x(), number_value);
    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.composite2(c)));
}

TEST_F(CursorTest, LastCompositeFieldReadInitInitsAndMovesCursorToBlockLength)
{
    test_schema::messages::msg8<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    m.composite2().x(number_value);

    auto composite = m.composite2(sbepp::cursor_ops::init(c));

    ASSERT_EQ(composite.x(), number_value);
    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + magic_block_length);
    STATIC_ASSERT(noexcept(m.composite2(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, LastCompositeFieldReadInitDontMoveInitsCursorToPrevFieldEnd)
{
    test_schema::messages::msg8<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    auto prev = m.composite1();
    m.composite2().x(number_value);

    auto composite = m.composite2(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(composite.x(), number_value);
    ASSERT_EQ(c.pointer(), sbepp::addressof(prev) + sbepp::size_bytes(prev));
    STATIC_ASSERT(noexcept(m.composite2(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, LastCompositeFieldReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg8<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);
    m.composite2().x(number_value);
    m.composite1(sbepp::cursor_ops::skip(c));
    auto old_ptr = c.pointer();

    auto composite = m.composite2(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(composite.x(), number_value);
    ASSERT_EQ(c.pointer(), old_ptr);
    STATIC_ASSERT(noexcept(m.composite2(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, LastCompositeFieldSkipMovesCursorToBlockLength)
{
    test_schema::messages::msg8<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);
    header.blockLength(magic_block_length);
    c = sbepp::init_cursor(m);
    auto old_ptr = c.pointer();
    m.composite1(sbepp::cursor_ops::skip(c));

    m.composite2(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(c.pointer(), old_ptr + magic_block_length);
    STATIC_ASSERT(noexcept(m.composite2(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.composite2(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorDeathTest, CompositeFieldAccessorsTerminateIfBufferIsNotEnough)
{
    // buffer is only enough to hold the header
    test_schema::messages::msg8<byte_type> m{buf.data(), header_size};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);

    ASSERT_DEATH({ m.composite1(c); }, ".*");
    ASSERT_DEATH({ m.composite1(sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.composite1(sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.composite1(sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.composite1(sbepp::cursor_ops::skip(c)); }, ".*");

    ASSERT_DEATH({ m.composite2(c); }, ".*");
    ASSERT_DEATH({ m.composite2(sbepp::cursor_ops::init(c)); }, ".*");
    ASSERT_DEATH({ m.composite2(sbepp::cursor_ops::dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.composite2(sbepp::cursor_ops::init_dont_move(c)); }, ".*");
    ASSERT_DEATH({ m.composite2(sbepp::cursor_ops::skip(c)); }, ".*");
}

TEST_F(CursorTest, FirstGroupReadInitsAndMovesCursorByHeaderSize)
{
    test_schema::messages::msg9<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);

    // first group accessors always uses `blockLength` to initialize the cursor
    auto g = m.group1(c);

    auto header = sbepp::get_header(g);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    ASSERT_EQ(sbepp::addressof(g), sbepp::addressof(m.group1()));
    STATIC_ASSERT(noexcept(m.group1(c)));
}

TEST_F(CursorTest, FirstGroupReadInitInitsAndMovesCursorByHeaderSize)
{
    test_schema::messages::msg9<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);

    auto g = m.group1(sbepp::cursor_ops::init(c));

    auto header = sbepp::get_header(g);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    ASSERT_EQ(sbepp::addressof(g), sbepp::addressof(m.group1()));
    STATIC_ASSERT(noexcept(m.group1(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, FirstGroupReadInitDontMoveInitsCursorUsingBlockLength)
{
    test_schema::messages::msg9<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);

    auto g = m.group1(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + *header.blockLength());
    ASSERT_EQ(sbepp::addressof(g), sbepp::addressof(m.group1()));
    STATIC_ASSERT(noexcept(m.group1(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, FirstGroupReadDontMoveInitsCursorUsingBlockLength)
{
    test_schema::messages::msg9<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    m.group1(sbepp::cursor_ops::init_dont_move(c));
    auto prev_ptr = c.pointer();

    auto g = m.group1(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(c.pointer(), prev_ptr);
    ASSERT_EQ(sbepp::addressof(g), sbepp::addressof(m.group1()));
    STATIC_ASSERT(noexcept(m.group1(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, FirstGroupReadSkipMovesCursorToGroupEnd)
{
    test_schema::messages::msg9<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto g = m.group1();
    sbepp::fill_group_header(g, 3);

    m.group1(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(c.pointer(), sbepp::addressof(g) + sbepp::size_bytes(g));
    STATIC_ASSERT(noexcept(m.group1(sbepp::cursor_ops::skip(c))));
    ASSERT_EQ(sbepp::addressof(g), sbepp::addressof(m.group1()));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.group1(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorTest, NonFirstGroupReadMovesCursorByHeaderSize)
{
    test_schema::messages::msg9<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto g1 = m.group1(c);
    sbepp::fill_group_header(g1, 0);

    auto g2 = m.group2(c);

    auto header = sbepp::get_header(g2);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    ASSERT_EQ(sbepp::addressof(g2), sbepp::addressof(m.group2()));
    STATIC_ASSERT(noexcept(m.group2(c)));
}

TEST_F(CursorTest, NonFirstGroupReadInitInitsAndMovesCursorByHeaderSize)
{
    test_schema::messages::msg9<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    sbepp::fill_group_header(m.group1(), 0);

    auto g2 = m.group2(sbepp::cursor_ops::init(c));

    auto header = sbepp::get_header(g2);
    ASSERT_EQ(
        c.pointer(), sbepp::addressof(header) + sbepp::size_bytes(header));
    ASSERT_EQ(sbepp::addressof(g2), sbepp::addressof(m.group2()));
    STATIC_ASSERT(noexcept(m.group2(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, NonFirstGroupReadInitDontMoveInitsCursorToGroupStart)
{
    test_schema::messages::msg9<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    sbepp::fill_group_header(m.group1(), 0);

    auto g = m.group2(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(c.pointer(), sbepp::addressof(g));
    ASSERT_EQ(sbepp::addressof(g), sbepp::addressof(m.group2()));
    STATIC_ASSERT(noexcept(m.group2(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, NonFirstGroupReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg9<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto g1 = m.group1(sbepp::cursor_ops::init(c));
    sbepp::fill_group_header(g1, 0);
    auto prev_ptr = c.pointer();

    auto g2 = m.group2(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(c.pointer(), prev_ptr);
    ASSERT_EQ(sbepp::addressof(g2), sbepp::addressof(m.group2()));
    STATIC_ASSERT(noexcept(m.group2(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, NonFirstGroupReadSkipMovesCursorToGroupEnd)
{
    test_schema::messages::msg9<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    sbepp::fill_group_header(m.group1(), 0);
    sbepp::fill_group_header(m.group2(), 3);
    m.group1(sbepp::cursor_ops::skip(c));

    m.group2(sbepp::cursor_ops::skip(c));

    auto g = m.group2();
    ASSERT_EQ(c.pointer(), sbepp::addressof(g) + sbepp::size_bytes(g));
    STATIC_ASSERT(noexcept(m.group2(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.group2(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorTest, FirstDataReadInitsAndMovesCursorByItsSize)
{
    test_schema::messages::msg10<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);

    // first data accessors always uses `blockLength` to initialize the cursor
    auto d = m.data1(c);

    ASSERT_EQ(c.pointer(), sbepp::addressof(d) + sbepp::size_bytes(d));
    ASSERT_EQ(sbepp::addressof(d), sbepp::addressof(m.data1()));
    STATIC_ASSERT(noexcept(m.data1(c)));
}

TEST_F(CursorTest, FirstDataReadInitInitsAndMovesCursorByHeaderSize)
{
    test_schema::messages::msg10<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);

    auto d = m.data1(sbepp::cursor_ops::init(c));

    ASSERT_EQ(c.pointer(), sbepp::addressof(d) + sbepp::size_bytes(d));
    ASSERT_EQ(sbepp::addressof(d), sbepp::addressof(m.data1()));
    STATIC_ASSERT(noexcept(m.data1(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, FirstDataReadInitDontMoveInitsCursorUsingBlockLength)
{
    test_schema::messages::msg10<byte_type> m{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(m);

    auto d = m.data1(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(
        c.pointer(),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + *header.blockLength());
    ASSERT_EQ(sbepp::addressof(d), sbepp::addressof(m.data1()));
    STATIC_ASSERT(noexcept(m.data1(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, FirstDataReadDontMoveInitsCursorUsingBlockLength)
{
    test_schema::messages::msg10<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    m.data1(sbepp::cursor_ops::init_dont_move(c));
    auto prev_ptr = c.pointer();

    auto d = m.data1(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(c.pointer(), prev_ptr);
    ASSERT_EQ(sbepp::addressof(d), sbepp::addressof(m.data1()));
    STATIC_ASSERT(noexcept(m.data1(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, FirstDataReadSkipSetsCursorToDataEnd)
{
    test_schema::messages::msg10<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto d = m.data1();
    d.resize(3);

    m.data1(sbepp::cursor_ops::skip(c));

    ASSERT_EQ(c.pointer(), sbepp::addressof(d) + sbepp::size_bytes(d));
    STATIC_ASSERT(noexcept(m.data1(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.data1(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorTest, NonFirstDataReadMovesCursorByDataSize)
{
    test_schema::messages::msg10<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    m.data1(c);
    m.data2().resize(3);

    auto d = m.data2(c);

    ASSERT_EQ(c.pointer(), sbepp::addressof(d) + sbepp::size_bytes(d));
    ASSERT_EQ(sbepp::addressof(d), sbepp::addressof(m.data2()));
    STATIC_ASSERT(noexcept(m.data2(c)));
}

TEST_F(CursorTest, NonFirstDataReadInitInitsAndMovesCursorByDataSize)
{
    test_schema::messages::msg10<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    m.data2().resize(3);

    auto d = m.data2(sbepp::cursor_ops::init(c));

    ASSERT_EQ(c.pointer(), sbepp::addressof(d) + sbepp::size_bytes(d));
    ASSERT_EQ(sbepp::addressof(d), sbepp::addressof(m.data2()));
    STATIC_ASSERT(noexcept(m.data2(sbepp::cursor_ops::init(c))));
}

TEST_F(CursorTest, NonFirstDataReadInitDontMoveInitsCursorToDataStart)
{
    test_schema::messages::msg10<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);

    auto d = m.data2(sbepp::cursor_ops::init_dont_move(c));

    ASSERT_EQ(c.pointer(), sbepp::addressof(d));
    ASSERT_EQ(sbepp::addressof(d), sbepp::addressof(m.data2()));
    STATIC_ASSERT(noexcept(m.data2(sbepp::cursor_ops::init_dont_move(c))));
}

TEST_F(CursorTest, NonFirstDataReadDontMoveDoesNotMoveCursor)
{
    test_schema::messages::msg10<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    m.data1(c);
    auto prev_ptr = c.pointer();

    auto d = m.data2(sbepp::cursor_ops::dont_move(c));

    ASSERT_EQ(c.pointer(), prev_ptr);
    ASSERT_EQ(sbepp::addressof(d), sbepp::addressof(m.data2()));
    STATIC_ASSERT(noexcept(m.data2(sbepp::cursor_ops::dont_move(c))));
}

TEST_F(CursorTest, NonFirstDataReadSkipMovesCursorToGroupEnd)
{
    test_schema::messages::msg10<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    m.data1(sbepp::cursor_ops::skip(c));

    m.data2(sbepp::cursor_ops::skip(c));

    auto d = m.data2();
    ASSERT_EQ(c.pointer(), sbepp::addressof(d) + sbepp::size_bytes(d));
    STATIC_ASSERT(noexcept(m.data2(sbepp::cursor_ops::skip(c))));
    STATIC_ASSERT_V(
        std::is_void<decltype(m.data2(sbepp::cursor_ops::skip(c)))>);
}

TEST_F(CursorTest, EntriesFromCursorRangeHaveValidOffsets)
{
    test_schema::messages::msg11<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto g = m.flat_group();
    sbepp::fill_group_header(g, 3);
    std::size_t i{};

    for(const auto entry :
        m.flat_group(sbepp::cursor_ops::init(c)).cursor_range(c))
    {
        ASSERT_EQ(sbepp::addressof(entry), sbepp::addressof(g[i]));
        entry.number(c);
        i++;
    }
}

TEST_F(CursorTest, CanIterateOverEmptyGroupUsingCursor)
{
    // In this test `msg12` works like newer version of `msg11` which does not
    // have any fields inside `flat_group`. However, it's still required to be
    // iterated over in order to move cursor up to the `data` field.
    test_schema::messages::msg11<byte_type> old_msg{buf.data(), buf.size()};
    sbepp::fill_message_header(old_msg);
    sbepp::fill_group_header(old_msg.flat_group(), 3);
    auto d = old_msg.data();
    d.push_back(0x20);

    test_schema::messages::msg12<byte_type> new_msg{buf.data(), buf.size()};
    c = sbepp::init_cursor(new_msg);
    for(const auto entry : new_msg.flat_group(c).cursor_range(c))
    {
        (void)entry;
        // no fields we can use to move cursor here, it's done inside entry's
        // constructor
    }
    auto d2 = new_msg.data(c);
    ASSERT_EQ(d2.size(), d.size());
    ASSERT_EQ(d[0], d2[0]);
}

TEST_F(
    CursorTest, CursorBasedSizeBytesReturnsDistanceFromViewStartToCursorPointer)
{
    test_schema::messages::msg8<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    c = sbepp::init_cursor(m);

    ASSERT_EQ(sbepp::size_bytes(m, c), c.pointer() - sbepp::addressof(m));

    m.composite1(c);

    ASSERT_EQ(sbepp::size_bytes(m, c), c.pointer() - sbepp::addressof(m));

    m.composite2(c);

    ASSERT_EQ(sbepp::size_bytes(m, c), c.pointer() - sbepp::addressof(m));
    STATIC_ASSERT(noexcept(sbepp::size_bytes(m, c)));
}

TEST_F(CursorTest, ViewAccessorReturnViewWithSameByteTypeAsCursor)
{
    using cursor_byte_type = const byte_type;
    test_schema::messages::msg13<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    sbepp::cursor<cursor_byte_type> c;

    auto composite = m.composite(sbepp::cursor_ops::init(c));
    auto array = m.array(sbepp::cursor_ops::init(c));

    STATIC_ASSERT_V(
        std::is_same<
            decltype(composite),
            sbepp::field_traits<test_schema::schema::messages::msg13::
                                    composite>::value_type<cursor_byte_type>>);
    STATIC_ASSERT_V(
        std::is_same<
            decltype(array),
            sbepp::field_traits<test_schema::schema::messages::msg13::array>::
                value_type<cursor_byte_type>>);

    test_schema::messages::msg12<byte_type> m2{buf.data(), buf.size()};
    sbepp::cursor<cursor_byte_type> c2;

    auto group = m2.flat_group(sbepp::cursor_ops::init(c));
    auto data = m2.data(sbepp::cursor_ops::init(c));

    STATIC_ASSERT_V(std::is_same<
                    decltype(group),
                    sbepp::group_traits<
                        test_schema::schema::messages::msg12::flat_group>::
                        value_type<cursor_byte_type>>);
    STATIC_ASSERT_V(
        std::is_same<
            decltype(data),
            sbepp::data_traits<test_schema::schema::messages::msg12::data>::
                value_type<cursor_byte_type>>);
}

TEST_F(CursorTest, CanReadFromNonConstViewUsingConstCursor)
{
    test_schema::messages::msg13<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    m.number(1);
    m.enumeration(test_schema::types::numbers_enum::One);
    m.set(test_schema::types::options_set{}.A(true));
    auto c = sbepp::init_const_cursor(m);

    ASSERT_EQ(m.number(), m.number(c));
    ASSERT_EQ(m.enumeration(), m.enumeration(c));
    ASSERT_EQ(m.set(), m.set(c));
}

struct get_number
{
    template<typename T, typename C>
    auto operator()(T obj, C c) -> decltype(obj.number(c))
    {
    }
};

struct get_enum
{
    template<typename T, typename C>
    auto operator()(T obj, C c) -> decltype(obj.enumeration(c))
    {
    }
};

struct get_set
{
    template<typename T, typename C>
    auto operator()(T obj, C c) -> decltype(obj.set(c))
    {
    }
};

struct set_number
{
    template<typename T, typename C>
    auto operator()(T obj, C c) -> decltype(obj.number({}, c))
    {
    }
};

struct set_enum
{
    template<typename T, typename C>
    auto operator()(T obj, C c) -> decltype(obj.enumeration({}, c))
    {
    }
};

struct set_set
{
    template<typename T, typename C>
    auto operator()(T obj, C c) -> decltype(obj.set({}, c))
    {
    }
};

struct get_composite
{
    template<typename T, typename C>
    auto operator()(T obj, C c) -> decltype(obj.composite(c))
    {
    }
};

struct get_array
{
    template<typename T, typename C>
    auto operator()(T obj, C c) -> decltype(obj.array(c))
    {
    }
};

struct get_group
{
    template<typename T, typename C>
    auto operator()(T obj, C c) -> decltype(obj.flat_group(c))
    {
    }
};

struct get_data
{
    template<typename T, typename C>
    auto operator()(T obj, C c) -> decltype(obj.data(c))
    {
    }
};

TEST_F(CursorTest, CanNotUseNonConstCursorWithConstView)
{
    using msg_type1 = test_schema::messages::msg13<const byte_type>;
    using msg_type2 = test_schema::messages::msg12<const byte_type>;

    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<get_number, msg_type1, cursor_t>::value);
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<get_enum, msg_type1, cursor_t>::value);
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<get_set, msg_type1, cursor_t>::value);
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_number, msg_type1, cursor_t>::value);
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_enum, msg_type1, cursor_t>::value);
    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<set_set, msg_type1, cursor_t>::value);
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<get_composite, msg_type1, cursor_t>::value);
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<get_array, msg_type1, cursor_t>::value);
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<get_group, msg_type2, cursor_t>::value);
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<get_data, msg_type2, cursor_t>::value);
}

TEST_F(CursorTest, CanNotWriteUsingConstCursor)
{
    using cursor_type = sbepp::cursor<const byte_type>;
    using msg_type = test_schema::messages::msg13<byte_type>;

    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_number, msg_type, cursor_type>::value);
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_enum, msg_type, cursor_type>::value);
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_set, msg_type, cursor_type>::value);
}

TEST_F(CursorTest, CursorSubrange1IteratesFromGivenPositionTillTheEnd)
{
    test_schema::messages::msg11<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto g = m.flat_group();
    sbepp::fill_group_header(g, 3);
    std::size_t i{};
    for(auto entry : g)
    {
        entry.number(i);
        i++;
    }
    auto cg = m.flat_group(sbepp::cursor_ops::init(c));
    for(auto entry : cg.cursor_range(c))
    {
        // put the cursor after the first group entry
        entry.number(sbepp::cursor_ops::skip(c));
        break;
    }

    // with correct iteration we expect to see [1, 2]
    i = 1;
    for(auto entry : cg.cursor_subrange(c, i))
    {
        ASSERT_EQ(entry.number(c), i);
        i++;
    }
    ASSERT_EQ(i, 3);
    STATIC_ASSERT(noexcept(cg.cursor_subrange(c, i)));
}

TEST_F(
    CursorDeathTest, CursorSubrange1TerminatesIfPositionIsNotLessThanGroupSize)
{
    test_schema::messages::msg11<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto g = m.flat_group();
    sbepp::fill_group_header(g, 3);

    ASSERT_DEATH(
        {
            m.flat_group(sbepp::cursor_ops::init(c))
                .cursor_subrange(c, g.size());
        },
        ".*");
}

TEST_F(CursorTest, CursorSubrange2IteratesFromGivenPositionGivenNumberOfTimes)
{
    test_schema::messages::msg11<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto g = m.flat_group();
    sbepp::fill_group_header(g, 3);
    std::size_t i{};
    for(auto entry : g)
    {
        entry.number(i);
        i++;
    }
    auto cg = m.flat_group(sbepp::cursor_ops::init(c));
    for(auto entry : cg.cursor_range(c))
    {
        // put the cursor after the first group entry
        entry.number(sbepp::cursor_ops::skip(c));
        break;
    }
    static constexpr auto count = 1;

    // expect only a single element [1]
    i = 1;
    for(auto entry : cg.cursor_subrange(c, i, count))
    {
        ASSERT_EQ(entry.number(c), i);
        i++;
    }
    ASSERT_EQ(i, 2);
    STATIC_ASSERT(noexcept(cg.cursor_subrange(c, i, count)));
}

TEST_F(
    CursorDeathTest, CursorSubrange2TerminatesIfPositionIsNotLessThanGroupSize)
{
    test_schema::messages::msg11<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto g = m.flat_group();
    sbepp::fill_group_header(g, 3);

    ASSERT_DEATH(
        {
            m.flat_group(sbepp::cursor_ops::init(c))
                .cursor_subrange(c, g.size(), 0);
        },
        ".*");
}

TEST_F(
    CursorDeathTest, CursorSubrange2TerminatesIfCountIsGreaterThanSizeMinusPos)
{
    test_schema::messages::msg11<byte_type> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto g = m.flat_group();
    sbepp::fill_group_header(g, 3);
    static constexpr auto pos = 0;
    const auto count = g.size() - pos + 1;
    ASSERT_GT(count, g.size() - pos);

    ASSERT_DEATH(
        {
            m.flat_group(sbepp::cursor_ops::init(c))
                .cursor_subrange(c, pos, count);
        },
        ".*");
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr auto constexpr_test1()
{
    std::array<char, 1024> buf{};
    test_schema::messages::msg2<char> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto c = sbepp::init_cursor(m);
    sbepp::init_const_cursor(m);

    m.number(c);
    m.number(sbepp::cursor_ops::init(c));
    m.number(sbepp::cursor_ops::dont_move(c));
    m.number(sbepp::cursor_ops::init_dont_move(c));
    m.number(sbepp::cursor_ops::skip(c));
    m.composite(c);
    m.composite(sbepp::cursor_ops::init(c));
    m.composite(sbepp::cursor_ops::dont_move(c));
    m.composite(sbepp::cursor_ops::init_dont_move(c));
    m.composite(sbepp::cursor_ops::skip(c));
    m.enumeration(c);
    m.enumeration(sbepp::cursor_ops::init(c));
    m.enumeration(sbepp::cursor_ops::dont_move(c));
    m.enumeration(sbepp::cursor_ops::init_dont_move(c));
    m.enumeration(sbepp::cursor_ops::skip(c));
    m.set(c);
    m.set(sbepp::cursor_ops::init(c));
    m.set(sbepp::cursor_ops::dont_move(c));
    m.set(sbepp::cursor_ops::init_dont_move(c));
    m.set(sbepp::cursor_ops::skip(c));
    m.composite(c);
    m.composite(sbepp::cursor_ops::init(c));
    m.composite(sbepp::cursor_ops::dont_move(c));
    m.composite(sbepp::cursor_ops::init_dont_move(c));
    m.composite(sbepp::cursor_ops::skip(c));
    m.number(1, c);
    m.number(1, sbepp::cursor_ops::init(c));
    m.number(1, sbepp::cursor_ops::dont_move(c));
    m.number(1, sbepp::cursor_ops::init_dont_move(c));
    m.enumeration(test_schema::types::numbers_enum::One, c);
    m.enumeration(
        test_schema::types::numbers_enum::One, sbepp::cursor_ops::init(c));
    m.enumeration(
        test_schema::types::numbers_enum::One, sbepp::cursor_ops::dont_move(c));
    m.enumeration(
        test_schema::types::numbers_enum::One,
        sbepp::cursor_ops::init_dont_move(c));
    auto set = m.set().A(true).B(true);
    m.set(set, c);
    m.set(set, sbepp::cursor_ops::init(c));
    m.set(set, sbepp::cursor_ops::dont_move(c));
    m.set(set, sbepp::cursor_ops::init_dont_move(c));

    auto g = m.group(c);
    sbepp::fill_group_header(g, 3);
    g.cursor_subrange(c, 0);
    g.cursor_subrange(c, 0, 0);

    for(auto entry : g.cursor_range(c))
    {
        entry.number(c);
        entry.number(sbepp::cursor_ops::init(c));
        entry.number(sbepp::cursor_ops::dont_move(c));
        entry.number(sbepp::cursor_ops::init_dont_move(c));
        entry.number(sbepp::cursor_ops::skip(c));
        entry.composite(c);
        entry.composite(sbepp::cursor_ops::init(c));
        entry.composite(sbepp::cursor_ops::dont_move(c));
        entry.composite(sbepp::cursor_ops::init_dont_move(c));
        entry.composite(sbepp::cursor_ops::skip(c));
        entry.enumeration(c);
        entry.enumeration(sbepp::cursor_ops::init(c));
        entry.enumeration(sbepp::cursor_ops::dont_move(c));
        entry.enumeration(sbepp::cursor_ops::init_dont_move(c));
        entry.enumeration(sbepp::cursor_ops::skip(c));
        entry.set(c);
        entry.set(sbepp::cursor_ops::init(c));
        entry.set(sbepp::cursor_ops::dont_move(c));
        entry.set(sbepp::cursor_ops::init_dont_move(c));
        entry.set(sbepp::cursor_ops::skip(c));
        entry.composite(c);
        entry.composite(sbepp::cursor_ops::init(c));
        entry.composite(sbepp::cursor_ops::dont_move(c));
        entry.composite(sbepp::cursor_ops::init_dont_move(c));
        entry.composite(sbepp::cursor_ops::skip(c));
        entry.number(1, c);
        entry.number(1, sbepp::cursor_ops::init(c));
        entry.number(1, sbepp::cursor_ops::dont_move(c));
        entry.number(1, sbepp::cursor_ops::init_dont_move(c));
        entry.enumeration(test_schema::types::numbers_enum::One, c);
        entry.enumeration(
            test_schema::types::numbers_enum::One, sbepp::cursor_ops::init(c));
        entry.enumeration(
            test_schema::types::numbers_enum::One,
            sbepp::cursor_ops::dont_move(c));
        entry.enumeration(
            test_schema::types::numbers_enum::One,
            sbepp::cursor_ops::init_dont_move(c));
        auto set = entry.set().A(true).B(true);
        entry.set(set, c);
        entry.set(set, sbepp::cursor_ops::init(c));
        entry.set(set, sbepp::cursor_ops::dont_move(c));
        entry.set(set, sbepp::cursor_ops::init_dont_move(c));

        auto g = entry.group(c);
        sbepp::fill_group_header(g, 3);
        g.cursor_range(c);
        g.cursor_subrange(c, 0);
        g.cursor_subrange(c, 0, 0);
        entry.group(sbepp::cursor_ops::init(c));
        entry.group(sbepp::cursor_ops::dont_move(c));
        entry.group(sbepp::cursor_ops::init_dont_move(c));
        entry.group(sbepp::cursor_ops::skip(c));

        entry.data(c);
        entry.data(sbepp::cursor_ops::init(c));
        entry.data(sbepp::cursor_ops::dont_move(c));
        entry.data(sbepp::cursor_ops::init_dont_move(c));
        entry.data(sbepp::cursor_ops::skip(c));
    }

    m.group(sbepp::cursor_ops::init(c));
    m.group(sbepp::cursor_ops::dont_move(c));
    m.group(sbepp::cursor_ops::init_dont_move(c));
    m.group(sbepp::cursor_ops::skip(c));

    m.data(c);
    m.data(sbepp::cursor_ops::init(c));
    m.data(sbepp::cursor_ops::dont_move(c));
    m.data(sbepp::cursor_ops::init_dont_move(c));
    m.data(sbepp::cursor_ops::skip(c));

    sbepp::size_bytes(m, c);

    return buf;
}

constexpr auto constexpr_test2()
{
    std::array<char, 1024> buf{};
    test_schema::messages::msg4<char> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto c = sbepp::init_cursor(m);

    m.number1(c);
    m.number1(sbepp::cursor_ops::init(c));
    m.number1(sbepp::cursor_ops::dont_move(c));
    m.number1(sbepp::cursor_ops::init_dont_move(c));
    m.number1(sbepp::cursor_ops::skip(c));
    m.number1(1, c);
    m.number1(1, sbepp::cursor_ops::init(c));
    m.number1(1, sbepp::cursor_ops::dont_move(c));
    m.number1(1, sbepp::cursor_ops::init_dont_move(c));

    m.number2(c);
    m.number2(sbepp::cursor_ops::init(c));
    m.number2(sbepp::cursor_ops::dont_move(c));
    m.number2(sbepp::cursor_ops::init_dont_move(c));
    m.number2(sbepp::cursor_ops::skip(c));
    m.number2(1, c);
    m.number2(1, sbepp::cursor_ops::init(c));
    m.number2(1, sbepp::cursor_ops::dont_move(c));
    m.number2(1, sbepp::cursor_ops::init_dont_move(c));

    return buf;
}

constexpr auto constexpr_test3()
{
    std::array<char, 1024> buf{};
    test_schema::messages::msg6<char> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto c = sbepp::init_cursor(m);

    m.enumeration1(c);
    m.enumeration1(sbepp::cursor_ops::init(c));
    m.enumeration1(sbepp::cursor_ops::dont_move(c));
    m.enumeration1(sbepp::cursor_ops::init_dont_move(c));
    m.enumeration1(sbepp::cursor_ops::skip(c));
    m.enumeration1(test_schema::types::numbers_enum::One, c);
    m.enumeration1(
        test_schema::types::numbers_enum::One, sbepp::cursor_ops::init(c));
    m.enumeration1(
        test_schema::types::numbers_enum::One, sbepp::cursor_ops::dont_move(c));
    m.enumeration1(
        test_schema::types::numbers_enum::One,
        sbepp::cursor_ops::init_dont_move(c));

    m.enumeration2(c);
    m.enumeration2(sbepp::cursor_ops::init(c));
    m.enumeration2(sbepp::cursor_ops::dont_move(c));
    m.enumeration2(sbepp::cursor_ops::init_dont_move(c));
    m.enumeration2(sbepp::cursor_ops::skip(c));
    m.enumeration2(test_schema::types::numbers_enum::One, c);
    m.enumeration2(
        test_schema::types::numbers_enum::One, sbepp::cursor_ops::init(c));
    m.enumeration2(
        test_schema::types::numbers_enum::One, sbepp::cursor_ops::dont_move(c));
    m.enumeration2(
        test_schema::types::numbers_enum::One,
        sbepp::cursor_ops::init_dont_move(c));

    return buf;
}

constexpr auto constexpr_test4()
{
    std::array<char, 1024> buf{};
    test_schema::messages::msg7<char> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto c = sbepp::init_cursor(m);

    auto s = m.set1().A(true).B(true);

    m.set1(c);
    m.set1(sbepp::cursor_ops::init(c));
    m.set1(sbepp::cursor_ops::dont_move(c));
    m.set1(sbepp::cursor_ops::init_dont_move(c));
    m.set1(sbepp::cursor_ops::skip(c));
    m.set1(s, c);
    m.set1(s, sbepp::cursor_ops::init(c));
    m.set1(s, sbepp::cursor_ops::dont_move(c));
    m.set1(s, sbepp::cursor_ops::init_dont_move(c));

    m.set2(c);
    m.set2(sbepp::cursor_ops::init(c));
    m.set2(sbepp::cursor_ops::dont_move(c));
    m.set2(sbepp::cursor_ops::init_dont_move(c));
    m.set2(sbepp::cursor_ops::skip(c));
    m.set2(s, c);
    m.set2(s, sbepp::cursor_ops::init(c));
    m.set2(s, sbepp::cursor_ops::dont_move(c));
    m.set2(s, sbepp::cursor_ops::init_dont_move(c));

    return buf;
}

constexpr auto constexpr_test5()
{
    std::array<char, 1024> buf{};
    test_schema::messages::msg5<char> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto c = sbepp::init_cursor(m);

    m.array1(c);
    m.array1(sbepp::cursor_ops::init(c));
    m.array1(sbepp::cursor_ops::dont_move(c));
    m.array1(sbepp::cursor_ops::init_dont_move(c));
    m.array1(sbepp::cursor_ops::skip(c));

    m.array2(c);
    m.array2(sbepp::cursor_ops::init(c));
    m.array2(sbepp::cursor_ops::dont_move(c));
    m.array2(sbepp::cursor_ops::init_dont_move(c));
    m.array2(sbepp::cursor_ops::skip(c));

    return buf;
}

constexpr auto constexpr_test6()
{
    std::array<char, 1024> buf{};
    test_schema::messages::msg8<char> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto c = sbepp::init_cursor(m);

    m.composite1(c);
    m.composite1(sbepp::cursor_ops::init(c));
    m.composite1(sbepp::cursor_ops::dont_move(c));
    m.composite1(sbepp::cursor_ops::init_dont_move(c));
    m.composite1(sbepp::cursor_ops::skip(c));

    m.composite2(c);
    m.composite2(sbepp::cursor_ops::init(c));
    m.composite2(sbepp::cursor_ops::dont_move(c));
    m.composite2(sbepp::cursor_ops::init_dont_move(c));
    m.composite2(sbepp::cursor_ops::skip(c));

    return buf;
}

constexpr auto constexpr_test7()
{
    std::array<char, 1024> buf{};
    test_schema::messages::msg9<char> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto c = sbepp::init_cursor(m);

    m.number(c);

    auto g1 = m.group1(c);
    m.group1(sbepp::cursor_ops::init(c));
    m.group1(sbepp::cursor_ops::dont_move(c));
    m.group1(sbepp::cursor_ops::init_dont_move(c));
    m.group1(sbepp::cursor_ops::skip(c));
    sbepp::fill_group_header(g1, 3);
    g1.cursor_range(c);
    g1.cursor_subrange(c, 0);
    g1.cursor_subrange(c, 0, 1);
    for(auto entry : g1.cursor_range(c))
    {
        (void)entry;
    }

    auto g2 = m.group2(c);
    m.group2(sbepp::cursor_ops::init(c));
    m.group2(sbepp::cursor_ops::dont_move(c));
    m.group2(sbepp::cursor_ops::init_dont_move(c));
    m.group2(sbepp::cursor_ops::skip(c));
    sbepp::fill_group_header(g2, 3);
    g2.cursor_range(c);
    g2.cursor_subrange(c, 0);
    g2.cursor_subrange(c, 0, 1);
    for(auto entry : g2.cursor_range(c))
    {
        (void)entry;
    }

    return buf;
}

constexpr auto constexpr_test8()
{
    std::array<char, 1024> buf{};
    test_schema::messages::msg10<char> m{buf.data(), buf.size()};
    sbepp::fill_message_header(m);
    auto c = sbepp::init_cursor(m);

    m.number(c);

    m.data1(c);
    m.data1(sbepp::cursor_ops::init(c));
    m.data1(sbepp::cursor_ops::dont_move(c));
    m.data1(sbepp::cursor_ops::init_dont_move(c));
    m.data1(sbepp::cursor_ops::skip(c));

    m.data2(c);
    m.data2(sbepp::cursor_ops::init(c));
    m.data2(sbepp::cursor_ops::dont_move(c));
    m.data2(sbepp::cursor_ops::init_dont_move(c));
    m.data2(sbepp::cursor_ops::skip(c));

    return buf;
}

constexpr auto buf1 = constexpr_test1();
constexpr auto buf2 = constexpr_test2();
constexpr auto buf3 = constexpr_test3();
constexpr auto buf4 = constexpr_test4();
constexpr auto buf5 = constexpr_test5();
constexpr auto buf6 = constexpr_test6();
constexpr auto buf7 = constexpr_test7();
constexpr auto buf8 = constexpr_test8();
#endif
} // namespace
