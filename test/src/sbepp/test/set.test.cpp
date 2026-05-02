// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <test_schema/types/options_set.hpp>
#include <test_schema/types/options_set64.hpp>

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace
{
// Generic test fixture for the following set types:
// - `test_schema::types::options_set` (uint8_t)
// - `test_schema::types::options_set64` (uint64_t)
template<typename T>
struct SetTest : public ::testing::Test
{
    using set_type = T;
    using encoding_type =
        typename sbepp::set_traits<sbepp::traits_tag_t<T>>::encoding_type;
};

using SetTypes = ::testing::
    Types<test_schema::types::options_set, test_schema::types::options_set64>;
TYPED_TEST_SUITE(SetTest, SetTypes);

TYPED_TEST(SetTest, StaticChecks)
{
    using set_t = typename TestFixture::set_type;
    using encoding_t = typename TestFixture::encoding_type;

    STATIC_ASSERT(sizeof(set_t) == sizeof(encoding_t));
    STATIC_ASSERT(sbepp::is_set<set_t>::value);

    STATIC_ASSERT_V(std::is_nothrow_default_constructible<set_t>);
    STATIC_ASSERT_V(std::is_trivially_copy_constructible<set_t>);
    STATIC_ASSERT_V(std::is_trivially_copy_assignable<set_t>);
    STATIC_ASSERT_V(std::is_trivially_move_constructible<set_t>);
    STATIC_ASSERT_V(std::is_trivially_move_assignable<set_t>);
    STATIC_ASSERT_V(std::is_trivially_destructible<set_t>);

#if SBEPP_HAS_INLINE_VARS
    STATIC_ASSERT(sbepp::is_set_v<set_t>);
#endif

#if SBEPP_HAS_CONCEPTS
    STATIC_ASSERT(sbepp::set<set_t>);
#endif
}

TYPED_TEST(SetTest, ZeroByDefault)
{
    using set_t = typename TestFixture::set_type;
    set_t s;

    ASSERT_EQ(*s, 0);
}

TYPED_TEST(SetTest, ExplicitlyConstructibleFromEncodingType)
{
    using set_t = typename TestFixture::set_type;
    using encoding_t = typename TestFixture::encoding_type;

    static constexpr encoding_t value{1};
    set_t s{value};

    ASSERT_EQ(*s, value);
}

TYPED_TEST(SetTest, DereferenceReturnsCurrentValue)
{
    using set_t = typename TestFixture::set_type;
    using encoding_t = typename TestFixture::encoding_type;

    static constexpr encoding_t value{1};
    set_t s{value};
    const auto& crs = s;

    STATIC_ASSERT_V(std::is_lvalue_reference<decltype(*s)>);
    STATIC_ASSERT_V(!std::is_reference<decltype(*crs)>);

    ASSERT_EQ(*s, value);
    ASSERT_EQ(*s, *crs);
}

TYPED_TEST(SetTest, CanGetChoiceViaNamedGetter)
{
    using set_t = typename TestFixture::set_type;
    set_t s;

    // since by default all choices are unset
    EXPECT_FALSE(s.A());
    EXPECT_FALSE(s.B());
    EXPECT_FALSE(s.C());
}

TYPED_TEST(SetTest, CanSetChoiceViaNamedSetter)
{
    using set_t = typename TestFixture::set_type;
    set_t s;

    s.A(true);
    EXPECT_EQ(s.A(), true);
    EXPECT_EQ(s.B(), false);
    EXPECT_EQ(s.C(), false);

    s.B(true);
    s.A(false);
    EXPECT_EQ(s.A(), false);
    EXPECT_EQ(s.B(), true);
    EXPECT_EQ(s.C(), false);

    s.C(true);
    s.B(false);
    ASSERT_EQ(s.A(), false);
    ASSERT_EQ(s.B(), false);
    ASSERT_EQ(s.C(), true);

    s.C(false);
    EXPECT_EQ(s.A(), false);
    EXPECT_EQ(s.B(), false);
    EXPECT_EQ(s.C(), false);
}

TYPED_TEST(SetTest, AccessorsUseCorrectBits)
{
    using set_t = typename TestFixture::set_type;
    using encoding_t = typename TestFixture::encoding_type;

    static constexpr encoding_t a_choice_set = 0x01; // 0b001;
    static constexpr encoding_t b_choice_set = 0x04; // 0b100;
    set_t s;

    s.A(true);

    ASSERT_EQ(*s, a_choice_set);

    s.A(false);
    s.B(true);

    ASSERT_EQ(*s, b_choice_set);
}

TYPED_TEST(SetTest, CopyAndMoveCopyEncoding)
{
    using set_t = typename TestFixture::set_type;
    auto s1 = set_t{}.A(true).C(true);
    auto s2{s1};

    ASSERT_EQ(s1.A(), s2.A());
    ASSERT_EQ(s1.B(), s2.B());
    ASSERT_EQ(s1.C(), s2.C());

    s1.B(true);
    s2 = s1;

    ASSERT_EQ(s1.A(), s2.A());
    ASSERT_EQ(s1.B(), s2.B());
    ASSERT_EQ(s1.C(), s2.C());

    // NOLINTNEXTLINE: move constructor test
    set_t s3{std::move(s1)};

    ASSERT_EQ(s1.A(), s3.A());
    ASSERT_EQ(s1.B(), s3.B());
    ASSERT_EQ(s1.C(), s3.C());

    s2.B(false);
    // NOLINTNEXTLINE: move assignment test
    s3 = std::move(s2);

    ASSERT_EQ(s2.A(), s3.A());
    ASSERT_EQ(s2.B(), s3.B());
    ASSERT_EQ(s2.C(), s3.C());
}

TYPED_TEST(SetTest, HasEncodingBasedComparisonOps)
{
    using set_t = typename TestFixture::set_type;

    // choice `A` occupies 0th bit, choice `B` occupies 2nd bit so `a` (=`1`) is
    // less than `b` (=`0b100`)
    auto a = set_t{}.A(true);
    auto b = set_t{}.B(true);

    ASSERT_EQ(a, a);
    ASSERT_NE(a, b);
}

TYPED_TEST(SetTest, VisitSetVisitsChoices)
{
    using set_t = typename TestFixture::set_type;
    set_t s{};
    s.A(true);
    s.B(false);
    s.C(true);

    std::size_t choice_index{};
    sbepp::visit_set(
        s,
        // NOLINTNEXTLINE: don't care about cognitive complexity here
        [&choice_index](const bool value, const char* name)
        {
            if(choice_index == 0)
            {
                ASSERT_EQ(value, true);
                ASSERT_STREQ(name, "A");
                choice_index++;
            }
            else if(choice_index == 1)
            {
                ASSERT_EQ(value, false);
                ASSERT_STREQ(name, "B");
                choice_index++;
            }
            else if(choice_index == 2)
            {
                ASSERT_EQ(value, true);
                ASSERT_STREQ(name, "C");
                choice_index++;
            }
            else
            {
                ASSERT_TRUE(false);
            }
        });
}

// tests that `A == true` and `B == false` and `C == true`
template<typename T>
class options_set_visitor
{
public:
    void on_set_choice(bool value, typename sbepp::traits_tag_t<T>::A)
    {
        EXPECT_EQ(choice_index, 0);
        EXPECT_EQ(value, true);
        valid &= ((choice_index == 0) && (value == true));
        choice_index++;
    }

    void on_set_choice(bool value, typename sbepp::traits_tag_t<T>::B)
    {
        EXPECT_EQ(choice_index, 1);
        EXPECT_EQ(value, false);
        valid &= ((choice_index == 1) && (value == false));
        choice_index++;
    }

    void on_set_choice(bool value, typename sbepp::traits_tag_t<T>::C)
    {
        EXPECT_EQ(choice_index, 2);
        EXPECT_EQ(value, true);
        valid &= ((choice_index == 2) && (value == true));
        choice_index++;
    }

    template<typename Tag>
    void on_set_choice(bool /*value*/, Tag)
    {
        FAIL() << "should not be called";
        valid = false;
    }

    bool is_valid() const
    {
        return valid && (choice_index == 3);
    }

private:
    bool valid{true};
    std::size_t choice_index{};
};

TYPED_TEST(SetTest, VisitSetVisitsChoices2)
{
    using set_t = typename TestFixture::set_type;
    set_t s{};
    s.A(true);
    s.B(false);
    s.C(true);

    auto visitor = sbepp::visit<options_set_visitor<set_t>>(s);

    ASSERT_TRUE(visitor.is_valid());
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
template<typename T>
constexpr T constexpr_test()
{
    using set_t = T;
    set_t s;
    s = set_t{0};
    *s;
    s.A(true);

    (s == s);
    (s != s);

    return s;
}

TYPED_TEST(SetTest, ConstExpr)
{
    using T = typename TestFixture::set_type;
    constexpr auto res = constexpr_test<T>();
}
#endif
} // namespace
