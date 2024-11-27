// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <test_schema/types/options_set.hpp>

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace
{
using set_t = test_schema::types::options_set;
// sets don't have any additional public names except choice names, however,
// encoding type is available via `set_traits`
using encoding_t =
    sbepp::set_traits<test_schema::schema::types::options_set>::encoding_type;

STATIC_ASSERT(sizeof(set_t) == sizeof(encoding_t));

STATIC_ASSERT_V(std::is_nothrow_default_constructible<set_t>);
STATIC_ASSERT_V(std::is_trivially_copy_constructible<set_t>);
STATIC_ASSERT_V(std::is_trivially_copy_assignable<set_t>);
STATIC_ASSERT_V(std::is_trivially_move_constructible<set_t>);
STATIC_ASSERT_V(std::is_trivially_move_assignable<set_t>);
STATIC_ASSERT_V(std::is_trivially_destructible<set_t>);

STATIC_ASSERT(sbepp::is_set<set_t>::value);

#if SBEPP_HAS_INLINE_VARS
STATIC_ASSERT(sbepp::is_set_v<set_t>);
#endif

#if SBEPP_HAS_CONCEPTS
STATIC_ASSERT(sbepp::set<set_t>);
#endif

TEST(SetTest, ZeroByDefault)
{
    set_t s;

    ASSERT_EQ(*s, 0);
}

TEST(SetTest, ExplicitlyConstructibleFromEncodingType)
{
    static constexpr encoding_t value{1};
    set_t s{value};

    ASSERT_EQ(*s, value);
}

TEST(SetTest, DereferenceReturnsCurrentValue)
{
    static constexpr encoding_t value{1};
    set_t s{value};
    const auto& crs = s;

    STATIC_ASSERT_V(std::is_lvalue_reference<decltype(*s)>);
    STATIC_ASSERT_V(!std::is_reference<decltype(*crs)>);

    ASSERT_EQ(*s, value);
    ASSERT_EQ(*s, *crs);
}

TEST(SetTest, CanGetChoiceViaNamedGetter)
{
    set_t s;

    // since by default all choices are unset
    ASSERT_FALSE(s.A());
}

TEST(SetTest, CanSetChoiceViaNamedSetter)
{
    static constexpr bool choice_value = true;
    set_t s;

    s.B(choice_value);

    ASSERT_EQ(s.B(), choice_value);
}

TEST(SetTest, AccessorsUseCorrectBits)
{
    static constexpr encoding_t a_choice_set = 0x01; // 0b001;
    static constexpr encoding_t b_choice_set = 0x04; // 0b100;
    set_t s;

    s.A(true);

    ASSERT_EQ(*s, a_choice_set);

    s.A(false);
    s.B(true);

    ASSERT_EQ(*s, b_choice_set);
}

TEST(SetTest, CopyAndMoveCopyEncoding)
{
    auto s1 = set_t{}.A(true);
    auto s2{s1};

    ASSERT_EQ(s1.A(), s2.A());
    ASSERT_EQ(s1.B(), s2.B());

    s1.B(true);
    s2 = s1;

    ASSERT_EQ(s1.A(), s2.A());
    ASSERT_EQ(s1.B(), s2.B());

    // NOLINTNEXTLINE: move constructor test
    set_t s3{std::move(s1)};

    ASSERT_EQ(s1.A(), s3.A());
    ASSERT_EQ(s1.B(), s3.B());

    s2.B(false);
    // NOLINTNEXTLINE: move assignment test
    s3 = std::move(s2);

    ASSERT_EQ(s2.A(), s3.A());
    ASSERT_EQ(s2.B(), s3.B());
}

TEST(SetTest, HasEncodingBasedComparisonOps)
{
    // choice `A` occupies 0th bit, choice `B` occupies 2nd bit so `a` (=`1`) is
    // less than `b` (=`0b100`)
    auto a = set_t{}.A(true);
    auto b = set_t{}.B(true);

    ASSERT_EQ(a, a);
    ASSERT_NE(a, b);
}

TEST(SetTest, VisitSetVisitsChoices)
{
    set_t s{};
    s.A(true);
    s.B(false);

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
            else
            {
                ASSERT_TRUE(false);
            }
        });
}

// tests that `A == true` and `B == false`
class options_set_visitor
{
public:
    void on_set_choice(bool value, test_schema::schema::types::options_set::A)
    {
        valid &= ((choice_index == 0) && (value == true));
        choice_index++;
    }

    void on_set_choice(bool value, test_schema::schema::types::options_set::B)
    {
        valid &= ((choice_index == 1) && (value == false));
        choice_index++;
    }

    template<typename Tag>
    void on_set_choice(bool /*value*/, Tag)
    {
        // should not be called
        valid = false;
    }

    bool is_valid() const
    {
        return valid && (choice_index == 2);
    }

private:
    bool valid{true};
    std::size_t choice_index{};
};

TEST(SetTest, VisitSetVisitsChoices2)
{
    set_t s{};
    s.A(true);
    s.B(false);

    auto visitor = sbepp::visit<options_set_visitor>(s);

    ASSERT_TRUE(visitor.is_valid());
}

#if SBEPP_HAS_CONSTEXPR_ACCESSORS
constexpr set_t constexpr_test()
{
    set_t s;
    s = set_t{0};
    *s;
    s.A(true);

    (s == s);
    (s != s);

    return s;
}

constexpr auto res = constexpr_test();
#endif
} // namespace
