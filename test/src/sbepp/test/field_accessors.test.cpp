// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#ifdef USE_TOP_FILE
#    include <test_schema/test_schema.hpp>
#else
#    include <test_schema/messages/msg2.hpp>
#    include <test_schema/messages/msg3.hpp>
#    include <test_schema/messages/msg10.hpp>
#endif

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <type_traits>
#include <array>

namespace
{
template<typename C>
class MessageLevel : public ::testing::Test
{
public:
    std::array<char, 256> buf{};
    C level{buf.data(), buf.size()};
};

// simple wrapper to initialize `block_length` parameter of entry's constructor
// to make it constructible from pointer+size
template<typename Byte>
class entry_wrapper
    : public sbepp::group_traits<
          test_schema::schema::messages::msg2::group>::entry_type<Byte>
{
public:
    using base_t = sbepp::group_traits<
        test_schema::schema::messages::msg2::group>::entry_type<Byte>;
    constexpr entry_wrapper(Byte* ptr, const std::size_t size)
        : base_t{
            ptr,
            size,
            sbepp::group_traits<
                test_schema::schema::messages::msg2::group>::block_length()}
    {
    }
};

using MutableMessageLevels =
    ::testing::Types<test_schema::messages::msg2<char>, entry_wrapper<char>>;

using ImmutableMessageLevels = ::testing::
    Types<test_schema::messages::msg2<const char>, entry_wrapper<const char>>;

using MessageLevels = ::testing::Types<
    test_schema::messages::msg2<char>,
    entry_wrapper<char>,
    test_schema::messages::msg2<const char>,
    entry_wrapper<const char>>;

template<typename T>
using MessageLevelTest = MessageLevel<T>;

template<typename T>
using MutableMessageLevelTest = MessageLevel<T>;

template<typename T>
using ImmutableMessageLevelTest = MessageLevel<T>;

TYPED_TEST_SUITE(MessageLevelTest, MessageLevels);
TYPED_TEST_SUITE(MutableMessageLevelTest, MutableMessageLevels);
TYPED_TEST_SUITE(ImmutableMessageLevelTest, ImmutableMessageLevels);

TYPED_TEST(MessageLevelTest, NonArrayTypeGettersReturnByValue)
{
    const auto& l = this->level;
    auto n = l.number();
    (void)n;

    STATIC_ASSERT_V(!std::is_reference<decltype(l.number())>);
    STATIC_ASSERT(noexcept(l.number()));
}

TYPED_TEST(MutableMessageLevelTest, NonArrayTypeSettersTakeByValue)
{
    const auto& l = this->level;
    auto n = l.number();
    n = 3;

    l.number(n);

    ASSERT_EQ(l.number(), n);
    STATIC_ASSERT(noexcept(l.number(n)));
}

struct set_number
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.number(obj.number()))
    {
    }
};

TYPED_TEST(ImmutableMessageLevelTest, ImmutableLevelsDontProvideTypeSetters)
{
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_number, decltype(this->level)>::value);
}

TYPED_TEST(MessageLevelTest, ArrayTypeGettersReturnByValue)
{
    const auto& l = this->level;
    auto arr = l.array();
    (void)arr;

    STATIC_ASSERT_V(!std::is_reference<decltype(l.array())>);
    STATIC_ASSERT(noexcept(l.array()));
}

struct set_array
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.array(obj.array()))
    {
    }
};

TYPED_TEST(MessageLevelTest, ArrayTypesDontHaveSetters)
{
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_array, decltype(this->level)>::value);
}

TYPED_TEST(MessageLevelTest, ArrayTypeGettersPreservePointerType)
{
    const auto& l = this->level;
    auto arr = l.array();

    STATIC_ASSERT_V(std::is_same<
                    decltype(sbepp::addressof(l)),
                    decltype(sbepp::addressof(arr))>);
}

TYPED_TEST(MessageLevelTest, CompositeGettersReturnByValue)
{
    const auto& l = this->level;
    auto c = l.composite();
    (void)c;

    STATIC_ASSERT_V(!std::is_reference<decltype(l.composite())>);
    STATIC_ASSERT(noexcept(l.composite()));
}

struct set_composite
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.composite(obj.composite()))
    {
    }
};

TYPED_TEST(MessageLevelTest, CompositesDontHaveSetters)
{
    STATIC_ASSERT(
        !sbepp::test::utils::
            is_invocable<set_composite, decltype(this->level)>::value);
}

TYPED_TEST(MessageLevelTest, CompositeGettersPreservePointerType)
{
    const auto& l = this->level;
    auto c = l.composite();

    STATIC_ASSERT_V(std::is_same<
                    decltype(sbepp::addressof(l)),
                    decltype(sbepp::addressof(c))>);
}

TYPED_TEST(MessageLevelTest, EnumGettersReturnByValue)
{
    const auto& l = this->level;
    auto e = l.enumeration();
    (void)e;

    STATIC_ASSERT_V(!std::is_reference<decltype(l.enumeration())>);
    STATIC_ASSERT(noexcept(l.enumeration()));
}

TYPED_TEST(MutableMessageLevelTest, EnumSettersTakeByValue)
{
    const auto& l = this->level;
    auto e = l.enumeration();
    e = decltype(e)::One;

    l.enumeration(e);

    ASSERT_EQ(l.enumeration(), e);
    STATIC_ASSERT(noexcept(l.enumeration(e)));
}

struct set_enum
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.enumeration(obj.enumeration()))
    {
    }
};

TYPED_TEST(ImmutableMessageLevelTest, ImmutableLevelsDontProvideEnumSetters)
{
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_enum, decltype(this->level)>::value);
}

TYPED_TEST(MessageLevelTest, SetGettersReturnByValue)
{
    const auto& l = this->level;
    auto s = l.set();
    (void)s;

    STATIC_ASSERT_V(!std::is_reference<decltype(l.set())>);
    STATIC_ASSERT(noexcept(l.set()));
}

TYPED_TEST(MutableMessageLevelTest, SetSettersTakeByValue)
{
    const auto& l = this->level;
    auto s = l.set();
    s.A(true);

    l.set(s);

    ASSERT_EQ(l.set(), s);
    STATIC_ASSERT(noexcept(l.set(s)));
}

struct set_set
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.set(obj.set()))
    {
    }
};

TYPED_TEST(ImmutableMessageLevelTest, ImmutableLevelsDontProvideSetSetters)
{
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_set, decltype(this->level)>::value);
}

TYPED_TEST(MessageLevelTest, GroupGettersReturnByValue)
{
    const auto& l = this->level;
    auto g = l.group();
    (void)g;

    STATIC_ASSERT_V(!std::is_reference<decltype(l.group())>);
    STATIC_ASSERT(noexcept(l.group()));
}

struct set_group
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.group(obj.group()))
    {
    }
};

TYPED_TEST(MessageLevelTest, GroupsDontHaveSetters)
{
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_group, decltype(this->level)>::value);
}

TYPED_TEST(MessageLevelTest, GroupGettersPreservePointerType)
{
    const auto& l = this->level;
    auto g = l.group();

    STATIC_ASSERT_V(std::is_same<
                    decltype(sbepp::addressof(l)),
                    decltype(sbepp::addressof(g))>);
}

TYPED_TEST(MessageLevelTest, DataGettersReturnByValue)
{
    const auto& l = this->level;
    auto g = l.data();
    (void)g;

    STATIC_ASSERT_V(!std::is_reference<decltype(l.data())>);
    STATIC_ASSERT(noexcept(l.data()));
}

struct set_data
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.data(obj.data()))
    {
    }
};

TYPED_TEST(MessageLevelTest, DataFieldsDontHaveSetters)
{
    STATIC_ASSERT(!sbepp::test::utils::
                      is_invocable<set_data, decltype(this->level)>::value);
}

TYPED_TEST(MessageLevelTest, DataGettersPreservePointerType)
{
    const auto& l = this->level;
    auto d = l.data();

    STATIC_ASSERT_V(std::is_same<
                    decltype(sbepp::addressof(l)),
                    decltype(sbepp::addressof(d))>);
}

TEST(SchemaExtensionTest, FirstGroupGetterTakesLevelBlockLengthIntoAccount)
{
    std::array<char, 256> buf{};
    test_schema::messages::msg3<char> msg{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(msg);
    static constexpr auto custom_block_length = 10;
    ASSERT_NE(header.blockLength(), custom_block_length);
    header.blockLength(custom_block_length);
    auto g = msg.nested_group();

    ASSERT_EQ(
        sbepp::addressof(g),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + custom_block_length);
}

TEST(SchemaExtensionTest, FirstDataGetterTakesLevelBlockLengthIntoAccount)
{
    std::array<char, 256> buf{};
    test_schema::messages::msg10<char> msg{buf.data(), buf.size()};
    auto header = sbepp::fill_message_header(msg);
    static constexpr auto custom_block_length = 10;
    ASSERT_NE(header.blockLength(), custom_block_length);
    header.blockLength(custom_block_length);
    auto g = msg.data1();

    ASSERT_EQ(
        sbepp::addressof(g),
        sbepp::addressof(header) + sbepp::size_bytes(header)
            + custom_block_length);
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
template<typename Level>
constexpr auto constexpr_test()
{
    std::array<char, 512> buf{};
    Level l{buf.data(), buf.size()};

    l.number();
    l.array();
    l.enumeration();
    l.set();
    l.composite();
    l.group();
    l.data();

    l.number(1);
    l.enumeration(test_schema::types::numbers_enum::One);
    l.set(l.set().A(true).B(true));

    return buf;
}

constexpr auto res1 = constexpr_test<test_schema::messages::msg2<char>>();
constexpr auto res2 = constexpr_test<entry_wrapper<char>>();
#endif
} // namespace
