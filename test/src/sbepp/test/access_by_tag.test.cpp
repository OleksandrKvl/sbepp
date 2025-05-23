// SPDX-License-Identifier: MIT
// Copyright (c) 2025, Oleksandr Koval

#include <sbepp/test/utils.hpp>
#include <test_schema/test_schema.hpp>

#include <gtest/gtest.h>

#include <array>

namespace
{
struct call_set_by_tag
{
    template<typename T, typename Tag>
    auto operator()(T&& obj, Tag) -> decltype(sbepp::set_by_tag<Tag>(
        // it's OK to use `obj` after `forward` since we only care about types
        std::forward<T>(obj),
        sbepp::get_by_tag<Tag>(obj)))
    {
    }
};

struct call_get_by_tag
{
    template<typename T, typename Tag>
    auto operator()(T&& obj, Tag)
        -> decltype(sbepp::get_by_tag<Tag>(std::forward<T>(obj)))
    {
    }
};

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

template<typename Level, typename Tag>
struct level_tag_pair
{
    using level_t = Level;
    using tag_t = Tag;
};

template<typename LevelTagPair>
class LevelStorage : public ::testing::Test
{
public:
    using tag_t = typename LevelTagPair::tag_t;
    using level_t = typename LevelTagPair::level_t;

    std::array<char, 256> buf{};
    level_t level{buf.data(), buf.size()};
};

using FieldLevels = ::testing::Types<
    level_tag_pair<
        test_schema::types::refs_composite<char>,
        test_schema::schema::types::refs_composite>,
    level_tag_pair<
        test_schema::messages::msg2<char>,
        test_schema::schema::messages::msg2>,
    level_tag_pair<
        entry_wrapper<char>,
        test_schema::schema::messages::msg2::group>>;

template<typename T>
using ValueSemanticsByTagAccessorsTest = LevelStorage<T>;

TYPED_TEST_SUITE(ValueSemanticsByTagAccessorsTest, FieldLevels);

using ReferenceSemanticsMsgLevels = ::testing::Types<
    level_tag_pair<
        test_schema::messages::msg2<char>,
        test_schema::schema::messages::msg2>,
    level_tag_pair<
        entry_wrapper<char>,
        test_schema::schema::messages::msg2::group>>;

template<typename T>
using ReferenceSemanticsByTagAccessorsTestMsg = LevelStorage<T>;

TYPED_TEST_SUITE(
    ReferenceSemanticsByTagAccessorsTestMsg, ReferenceSemanticsMsgLevels);

template<typename T>
using ReferenceSemanticsByTagAccessorsTest = LevelStorage<T>;

TYPED_TEST_SUITE(ReferenceSemanticsByTagAccessorsTest, FieldLevels);

TYPED_TEST(
    ValueSemanticsByTagAccessorsTest, ProvideSameValuesAsNormalAccessorsForType)
{
    const auto& l = this->level;
    std::uint32_t valid_value = 1;

    l.number(valid_value);

    auto n = sbepp::get_by_tag<typename TestFixture::tag_t::number>(l);

    EXPECT_EQ(n, valid_value);
    IS_SAME_TYPE(
        decltype(l.number()),
        decltype(sbepp::get_by_tag<typename TestFixture::tag_t::number>(l)));
    IS_NOEXCEPT(sbepp::get_by_tag<typename TestFixture::tag_t::number>(l));

    valid_value++;
    sbepp::set_by_tag<typename TestFixture::tag_t::number>(l, valid_value);

    n = l.number();
    EXPECT_EQ(n, valid_value);
    IS_SAME_TYPE(
        decltype(l.number(valid_value)),
        decltype(sbepp::set_by_tag<typename TestFixture::tag_t::number>(
            l, valid_value)));
    IS_NOEXCEPT(
        sbepp::set_by_tag<typename TestFixture::tag_t::number>(l, valid_value));
}

TYPED_TEST(
    ValueSemanticsByTagAccessorsTest, ProvideSameValuesAsNormalAccessorsForEnum)
{
    const auto& l = this->level;
    auto valid_value = test_schema::types::numbers_enum::One;

    l.enumeration(valid_value);

    auto n = sbepp::get_by_tag<typename TestFixture::tag_t::enumeration>(l);

    EXPECT_EQ(n, valid_value);
    IS_SAME_TYPE(
        decltype(l.enumeration()),
        decltype(sbepp::get_by_tag<typename TestFixture::tag_t::enumeration>(
            l)));
    IS_NOEXCEPT(sbepp::get_by_tag<typename TestFixture::tag_t::enumeration>(l));

    valid_value = test_schema::types::numbers_enum::Two;
    sbepp::set_by_tag<typename TestFixture::tag_t::enumeration>(l, valid_value);

    n = l.enumeration();
    EXPECT_EQ(n, valid_value);
    IS_SAME_TYPE(
        decltype(l.enumeration(valid_value)),
        decltype(sbepp::set_by_tag<typename TestFixture::tag_t::enumeration>(
            l, valid_value)));
    IS_NOEXCEPT(
        sbepp::set_by_tag<typename TestFixture::tag_t::enumeration>(
            l, valid_value));
}

TYPED_TEST(
    ValueSemanticsByTagAccessorsTest, ProvideSameValuesAsNormalAccessorsForSet)
{
    const auto& l = this->level;
    auto valid_value = test_schema::types::options_set{}.A(true);

    l.set(valid_value);

    auto n = sbepp::get_by_tag<typename TestFixture::tag_t::set>(l);

    EXPECT_EQ(n, valid_value);
    IS_SAME_TYPE(
        decltype(l.set()),
        decltype(sbepp::get_by_tag<typename TestFixture::tag_t::set>(l)));
    IS_NOEXCEPT(sbepp::get_by_tag<typename TestFixture::tag_t::set>(l));

    valid_value.B(true);
    sbepp::set_by_tag<typename TestFixture::tag_t::set>(l, valid_value);

    n = l.set();
    EXPECT_EQ(n, valid_value);
    IS_SAME_TYPE(
        decltype(l.set(valid_value)),
        decltype(sbepp::set_by_tag<typename TestFixture::tag_t::set>(
            l, valid_value)));
    IS_NOEXCEPT(
        sbepp::set_by_tag<typename TestFixture::tag_t::set>(l, valid_value));
}

TYPED_TEST(ValueSemanticsByTagAccessorsTest, ByTagAccessorsDisabledForWrongTag)
{
    using set_t = test_schema::types::options_set;
    using wrong_tag_t = test_schema::schema::types::uint32_req;

    STATIC_ASSERT_V(
        !sbepp::test::utils::is_invocable<call_set_by_tag, set_t, wrong_tag_t>);
    STATIC_ASSERT_V(
        !sbepp::test::utils::is_invocable<call_get_by_tag, set_t, wrong_tag_t>);
}

TYPED_TEST(
    ReferenceSemanticsByTagAccessorsTestMsg, ProvideSameValuesAsNormalAccessors)
{
    const auto& l = this->level;

    EXPECT_EQ(
        sbepp::addressof(l.group()),
        sbepp::addressof(
            sbepp::get_by_tag<typename TestFixture::tag_t::group>(l)));
    IS_SAME_TYPE(
        decltype(l.group()),
        decltype(sbepp::get_by_tag<typename TestFixture::tag_t::group>(l)));
    IS_NOEXCEPT(sbepp::get_by_tag<typename TestFixture::tag_t::group>(l));

    EXPECT_EQ(
        sbepp::addressof(l.data()),
        sbepp::addressof(
            sbepp::get_by_tag<typename TestFixture::tag_t::data>(l)));
    IS_SAME_TYPE(
        decltype(l.data()),
        decltype(sbepp::get_by_tag<typename TestFixture::tag_t::data>(l)));
    IS_NOEXCEPT(sbepp::get_by_tag<typename TestFixture::tag_t::data>(l));
}

TYPED_TEST(ReferenceSemanticsByTagAccessorsTestMsg, SetByTagIsDisabled)
{
    STATIC_ASSERT_V(!sbepp::test::utils::is_invocable<
                    call_set_by_tag,
                    decltype(this->level),
                    typename TestFixture::tag_t::group>);
    STATIC_ASSERT_V(!sbepp::test::utils::is_invocable<
                    call_set_by_tag,
                    decltype(this->level),
                    typename TestFixture::tag_t::data>);
}

TYPED_TEST(
    ReferenceSemanticsByTagAccessorsTest, ProvideSameValuesAsNormalAccessors)
{
    const auto& l = this->level;

    EXPECT_EQ(
        sbepp::addressof(l.array()),
        sbepp::addressof(
            sbepp::get_by_tag<typename TestFixture::tag_t::array>(l)));
    IS_SAME_TYPE(
        decltype(l.array()),
        decltype(sbepp::get_by_tag<typename TestFixture::tag_t::array>(l)));
    IS_NOEXCEPT(sbepp::get_by_tag<typename TestFixture::tag_t::array>(l));

    EXPECT_EQ(
        sbepp::addressof(l.composite()),
        sbepp::addressof(
            sbepp::get_by_tag<typename TestFixture::tag_t::composite>(l)));
    IS_SAME_TYPE(
        decltype(l.composite()),
        decltype(sbepp::get_by_tag<typename TestFixture::tag_t::composite>(l)));
    IS_NOEXCEPT(sbepp::get_by_tag<typename TestFixture::tag_t::composite>(l));
}

TYPED_TEST(ReferenceSemanticsByTagAccessorsTest, SetByTagIsDisabled)
{
    STATIC_ASSERT_V(!sbepp::test::utils::is_invocable<
                    call_set_by_tag,
                    decltype(this->level),
                    typename TestFixture::tag_t::array>);
    STATIC_ASSERT_V(!sbepp::test::utils::is_invocable<
                    call_set_by_tag,
                    decltype(this->level),
                    typename TestFixture::tag_t::composite>);
}

TEST(SetChoiceTagAccessorsTest, ProvideSameValuesAsNormalAccessors)
{
    using set_t = test_schema::types::options_set;
    using tag_t = sbepp::traits_tag_t<set_t>;
    set_t s;

    s.A(true);

    EXPECT_EQ(s.A(), sbepp::get_by_tag<tag_t::A>(s));
    IS_SAME_TYPE(decltype(s.A()), decltype(sbepp::get_by_tag<tag_t::A>(s)));
    IS_NOEXCEPT(sbepp::get_by_tag<tag_t::A>(s));

    sbepp::set_by_tag<tag_t::B>(s, true);

    EXPECT_TRUE(s.B());
    IS_SAME_TYPE(
        decltype(s.B(true)), decltype(sbepp::set_by_tag<tag_t::B>(s, true)));
    IS_NOEXCEPT(sbepp::set_by_tag<tag_t::B>(s, true));
}

TEST(SetChoiceTagAccessorsTest, SetByTagIsDisabledForConstQualifiedObject)
{
    using set_t = test_schema::types::options_set;
    using tag_t = sbepp::traits_tag_t<set_t>;

    STATIC_ASSERT_V(!sbepp::test::utils::
                        is_invocable<call_set_by_tag, const set_t&, tag_t::B>);
}

TEST(SetChoiceTagAccessorsTest, ByTagAccessorsDisabledForWrongTag)
{
    using set_t = test_schema::types::options_set;
    using wrong_tag_t = test_schema::schema::types::uint32_req;

    STATIC_ASSERT_V(
        !sbepp::test::utils::is_invocable<call_set_by_tag, set_t, wrong_tag_t>);
    STATIC_ASSERT_V(
        !sbepp::test::utils::is_invocable<call_get_by_tag, set_t, wrong_tag_t>);
}

using ConstantLevels = ::testing::Types<
    level_tag_pair<
        test_schema::types::constants<char>,
        test_schema::schema::types::constants>,
    level_tag_pair<
        test_schema::messages::Msg1<char>,
        test_schema::schema::messages::Msg1>>;

template<typename T>
using ConstantsByTagAccessorsTest = LevelStorage<T>;

TYPED_TEST_SUITE(ConstantsByTagAccessorsTest, ConstantLevels);

TYPED_TEST(ConstantsByTagAccessorsTest, ProvideSameValuesAsNormalAccessors)
{
    const auto& l = this->level;

    EXPECT_EQ(
        l.num_const_value(),
        sbepp::get_by_tag<typename TestFixture::tag_t::num_const_value>(l));
    IS_SAME_TYPE(
        decltype(l.num_const_value()),
        decltype(sbepp::get_by_tag<
                 typename TestFixture::tag_t::num_const_value>(l)));
    IS_NOEXCEPT(
        sbepp::get_by_tag<typename TestFixture::tag_t::num_const_value>(l));

    EXPECT_EQ(
        sbepp::addressof(l.str_const1_value()),
        sbepp::addressof(
            sbepp::get_by_tag<typename TestFixture::tag_t::str_const1_value>(
                l)));
    IS_SAME_TYPE(
        decltype(l.str_const1_value()),
        decltype(sbepp::get_by_tag<
                 typename TestFixture::tag_t::str_const1_value>(l)));
    IS_NOEXCEPT(
        sbepp::get_by_tag<typename TestFixture::tag_t::str_const1_value>(l));
}

TYPED_TEST(ConstantsByTagAccessorsTest, SetByTagIsDisabled)
{
    STATIC_ASSERT_V(!sbepp::test::utils::is_invocable<
                    call_set_by_tag,
                    decltype(this->level),
                    typename TestFixture::tag_t::num_const_value>);

    STATIC_ASSERT_V(!sbepp::test::utils::is_invocable<
                    call_set_by_tag,
                    decltype(this->level),
                    typename TestFixture::tag_t::str_const1_value>);
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
template<typename Level, typename Tag, bool IsComposite>
constexpr auto constexpr_test()
{
    std::array<char, 512> buf{};
    Level l{buf.data(), buf.size()};

    sbepp::get_by_tag<typename Tag::number>(l);
    sbepp::get_by_tag<typename Tag::array>(l);
    sbepp::get_by_tag<typename Tag::enumeration>(l);
    sbepp::get_by_tag<typename Tag::set>(l);
    sbepp::get_by_tag<typename Tag::composite>(l);

    if constexpr(!IsComposite)
    {
        sbepp::get_by_tag<typename Tag::group>(l);
        sbepp::get_by_tag<typename Tag::data>(l);
    }

    sbepp::get_by_tag<typename Tag::number>(l, 1);
    sbepp::get_by_tag<typename Tag::enumeration>(
        l, test_schema::types::numbers_enum::One);
    sbepp::get_by_tag<typename Tag::set>(
        l, test_schema::types::options_set{}.A(true).B(true));

    return buf;
}

constexpr auto res1 = constexpr_test<
    test_schema::messages::msg2<char>,
    test_schema::schema::messages::msg2,
    false>();
constexpr auto res2 = constexpr_test<
    entry_wrapper<char>,
    test_schema::schema::messages::msg2::group,
    false>();
constexpr auto res3 = constexpr_test<
    test_schema::types::refs_composite<char>,
    test_schema::schema::types::refs_composite,
    true>();
#endif
} // namespace