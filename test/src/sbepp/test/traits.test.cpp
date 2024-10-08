// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#if defined(USE_TOP_FILE)
#    include <traits_test_schema/traits_test_schema.hpp>
#    include <traits_test_schema2/traits_test_schema2.hpp>
#else
#    include <traits_test_schema/types/messageHeader.hpp>
#    include <traits_test_schema/types/groupSizeEncoding.hpp>
#    include <traits_test_schema/types/customGroupSizeEncoding.hpp>
#    include <traits_test_schema/types/varDataEncoding.hpp>
#    include <traits_test_schema/types/enum_1.hpp>
#    include <traits_test_schema/types/enum_2.hpp>
#    include <traits_test_schema/types/composite_1.hpp>
#    include <traits_test_schema/types/set_1.hpp>
#    include <traits_test_schema/types/set_2.hpp>
#    include <traits_test_schema/types/composite_2.hpp>
#    include <traits_test_schema/types/composite_3.hpp>
#    include <traits_test_schema/types/composite_4.hpp>
#    include <traits_test_schema/types/composite_5.hpp>
#    include <traits_test_schema/types/composite_6.hpp>
#    include <traits_test_schema/types/composite_7.hpp>
#    include <traits_test_schema/types/composite_8.hpp>
#    include <traits_test_schema/types/str128.hpp>
#    include <traits_test_schema/types/str_const.hpp>
#    include <traits_test_schema/types/uint32_req.hpp>
#    include <traits_test_schema/types/uint32_opt.hpp>
#    include <traits_test_schema/types/uint32_const.hpp>
#    include <traits_test_schema/types/type_1.hpp>
#    include <traits_test_schema/messages/msg_1.hpp>
#    include <traits_test_schema/messages/msg_2.hpp>
#    include <traits_test_schema/messages/msg_3.hpp>
#    include <traits_test_schema/messages/msg_4.hpp>
#    include <traits_test_schema/messages/msg_5.hpp>
#    include <traits_test_schema/messages/msg_6.hpp>
#    include <traits_test_schema/messages/msg_7.hpp>
#    include <traits_test_schema/messages/msg_8.hpp>
#    include <traits_test_schema/messages/msg_9.hpp>
#    include <traits_test_schema/messages/msg_10.hpp>
#    include <traits_test_schema/messages/msg_11.hpp>
#    include <traits_test_schema/messages/msg_12.hpp>
#    include <traits_test_schema/messages/msg_13.hpp>
#    include <traits_test_schema/messages/msg_14.hpp>
#    include <traits_test_schema/messages/msg_15.hpp>
#    include <traits_test_schema/messages/msg_16.hpp>

#    include <traits_test_schema2/schema/schema.hpp>
#endif

#include <sbepp/test/utils.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace
{
constexpr auto g_added_since = 1;
constexpr auto g_deprecated_since = 10;
constexpr auto g_custom_offset = 20;

// `description` is a common static function for all the traits
template<
    template<typename> class Trait,
    typename Tag,
    typename = sbepp::test::utils::void_t<>>
struct has_description : std::false_type
{
};

template<template<typename> class Trait, typename Tag>
struct has_description<
    Trait,
    Tag,
    sbepp::test::utils::void_t<decltype(Trait<Tag>::description())>>
    : std::true_type
{
};

template<typename T, typename = void>
struct sbe_traits;

template<typename T>
struct sbe_traits<
    T,
    sbepp::test::utils::enable_if_t<
        has_description<sbepp::type_traits, T>::value>> : sbepp::type_traits<T>
{
};

template<typename T>
struct sbe_traits<
    T,
    sbepp::test::utils::enable_if_t<
        has_description<sbepp::schema_traits, T>::value>>
    : sbepp::schema_traits<T>
{
};

template<typename T>
struct sbe_traits<
    T,
    sbepp::test::utils::enable_if_t<
        has_description<sbepp::enum_traits, T>::value>> : sbepp::enum_traits<T>
{
};

template<typename T>
struct sbe_traits<
    T,
    sbepp::test::utils::enable_if_t<
        has_description<sbepp::enum_value_traits, T>::value>>
    : sbepp::enum_value_traits<T>
{
};

template<typename T>
struct sbe_traits<
    T,
    sbepp::test::utils::enable_if_t<
        has_description<sbepp::set_traits, T>::value>> : sbepp::set_traits<T>
{
};

template<typename T>
struct sbe_traits<
    T,
    sbepp::test::utils::enable_if_t<
        has_description<sbepp::set_choice_traits, T>::value>>
    : sbepp::set_choice_traits<T>
{
};

template<typename T>
struct sbe_traits<
    T,
    sbepp::test::utils::enable_if_t<
        has_description<sbepp::composite_traits, T>::value>>
    : sbepp::composite_traits<T>
{
};

template<typename T>
struct sbe_traits<
    T,
    sbepp::test::utils::enable_if_t<
        has_description<sbepp::message_traits, T>::value>>
    : sbepp::message_traits<T>
{
};

template<typename T>
struct sbe_traits<
    T,
    sbepp::test::utils::enable_if_t<
        has_description<sbepp::field_traits, T>::value>>
    : sbepp::field_traits<T>
{
};

template<typename T>
struct sbe_traits<
    T,
    sbepp::test::utils::enable_if_t<
        has_description<sbepp::group_traits, T>::value>>
    : sbepp::group_traits<T>
{
};

template<typename T>
struct sbe_traits<
    T,
    sbepp::test::utils::enable_if_t<
        has_description<sbepp::data_traits, T>::value>> : sbepp::data_traits<T>
{
};

TEST(SchemaTraitsTest, SchemaTraitsProvidesTheSameValuesAsSchemaXml)
{
    using schema_tag = traits_test_schema::schema;
    using schema_traits = sbepp::schema_traits<schema_tag>;

    ASSERT_STREQ(schema_traits::package(), "traits_test_schema");
    ASSERT_EQ(schema_traits::id(), 1);
    ASSERT_EQ(schema_traits::version(), 10);
    ASSERT_STREQ(schema_traits::semantic_version(), "5.2");
    ASSERT_EQ(schema_traits::byte_order(), sbepp::endian::little);
    ASSERT_STREQ(schema_traits::description(), "schema description");
    STATIC_ASSERT_V(std::is_same<
                    schema_traits::header_type<char>,
                    traits_test_schema::types::messageHeader<char>>);
    STATIC_ASSERT_V(std::is_same<
                    schema_traits::header_type_tag,
                    schema_tag::types::messageHeader>);
    IS_NOEXCEPT(schema_traits::package());
    IS_NOEXCEPT(schema_traits::id());
    IS_NOEXCEPT(schema_traits::version());
    IS_NOEXCEPT(schema_traits::semantic_version());
    IS_NOEXCEPT(schema_traits::byte_order());
    IS_NOEXCEPT(schema_traits::description());
}

TEST(SchemaTraitsTest, SchemaTraitsProvidesEmptyStringsForOptionalAttributes)
{
    using schema_tag = traits_test_schema2::schema;
    using schema_traits = sbepp::schema_traits<schema_tag>;

    ASSERT_STREQ(schema_traits::package(), "");
    ASSERT_STREQ(schema_traits::semantic_version(), "");
    ASSERT_STREQ(schema_traits::description(), "");
}

TEST(EnumTraitsTest, ProvidesTheSameValuesAsSchemaXml)
{
    using enum_tag = traits_test_schema::schema::types::enum_1;
    using enum_traits = sbepp::enum_traits<enum_tag>;

    ASSERT_STREQ(enum_traits::name(), "enum_1");
    ASSERT_STREQ(enum_traits::description(), "enum description");
    IS_SAME_TYPE(enum_traits::encoding_type, std::uint8_t);
    IS_SAME_TYPE(enum_traits::value_type, traits_test_schema::types::enum_1);
    IS_NOEXCEPT(enum_traits::name());
    IS_NOEXCEPT(enum_traits::description());
}

template<typename Tag>
class TraitsContainer : public ::testing::Test
{
public:
    using tag = Tag;
    using traits = sbe_traits<Tag>;
};

using DefaultVersionTags = ::testing::Types<
    traits_test_schema::schema::types::enum_2,
    traits_test_schema::schema::types::enum_1::b,
    traits_test_schema::schema::types::set_2,
    traits_test_schema::schema::types::composite_4,
    traits_test_schema::schema::messages::msg_2,
    traits_test_schema::schema::messages::msg_3::field_2,
    traits_test_schema::schema::messages::msg_4::group_2,
    traits_test_schema::schema::messages::msg_5::data_2,
    traits_test_schema::schema::types::uint32_req>;

template<typename T>
using DefaultVersionTraitTest = TraitsContainer<T>;

TYPED_TEST_SUITE(DefaultVersionTraitTest, DefaultVersionTags);

TYPED_TEST(DefaultVersionTraitTest, SinceVersionIsZeroWhenNotProvided)
{
    using traits = typename TestFixture::traits;
    ASSERT_EQ(traits::since_version(), 0);
    IS_NOEXCEPT(traits::since_version());
}

struct get_deprecated
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.deprecated())
    {
    }
};

TYPED_TEST(DefaultVersionTraitTest, DeprecatedVersionDoesNotExistIfNotProvided)
{
    using traits = typename TestFixture::traits;

    STATIC_ASSERT(
        !sbepp::test::utils::is_invocable<get_deprecated, traits>::value);
}

using CustomVersionTags = ::testing::Types<
    traits_test_schema::schema::types::enum_1,
    traits_test_schema::schema::types::enum_1::a,
    traits_test_schema::schema::types::set_1,
    traits_test_schema::schema::types::set_1::a,
    traits_test_schema::schema::types::composite_3,
    traits_test_schema::schema::messages::msg_1,
    traits_test_schema::schema::messages::msg_3::field_1,
    traits_test_schema::schema::messages::msg_4::group_1,
    traits_test_schema::schema::messages::msg_5::data_1,
    traits_test_schema::schema::types::type_1>;

template<typename T>
using CustomVersionTraitTest = TraitsContainer<T>;

TYPED_TEST_SUITE(CustomVersionTraitTest, CustomVersionTags);

TYPED_TEST(CustomVersionTraitTest, HasTheSameSinceVersionAsInSchemaXml)
{
    using traits = typename TestFixture::traits;

    ASSERT_EQ(traits::since_version(), g_added_since);
    IS_NOEXCEPT(traits::since_version());
}

TYPED_TEST(CustomVersionTraitTest, HasTheSameDeprecatedVersionAsInSchemaXml)
{
    using traits = typename TestFixture::traits;

    ASSERT_EQ(traits::deprecated(), g_deprecated_since);
    IS_NOEXCEPT(traits::deprecated());
}

using EmptyDescriptionTags = DefaultVersionTags;

template<typename T>
using EmptyDescriptionTraitTest = TraitsContainer<T>;

TYPED_TEST_SUITE(EmptyDescriptionTraitTest, DefaultVersionTags);

TYPED_TEST(EmptyDescriptionTraitTest, WhenNotProvidedDescriptionsIsEmptyString)
{
    using traits = typename TestFixture::traits;

    ASSERT_STREQ(traits::description(), "");
    IS_NOEXCEPT(traits::description());
}

// public types don't have offset because their location is not fixed
using NoOffsetTags = ::testing::Types<
    traits_test_schema::schema::types::enum_1,
    traits_test_schema::schema::types::set_1,
    traits_test_schema::schema::types::composite_3,
    traits_test_schema::schema::types::type_1>;

template<typename T>
using NoOffsetTraitTest = TraitsContainer<T>;

TYPED_TEST_SUITE(NoOffsetTraitTest, NoOffsetTags);

struct get_offset
{
    template<typename T>
    auto operator()(T obj) -> decltype(obj.offset())
    {
    }
};

TYPED_TEST(NoOffsetTraitTest, OffsetIsNotProvidedForPublicTypes)
{
    using traits = typename TestFixture::traits;

    STATIC_ASSERT(!sbepp::test::utils::is_invocable<get_offset, traits>::value);
}

using HasOffsetTags = ::testing::Types<
    traits_test_schema::schema::types::composite_1::enumeration,
    traits_test_schema::schema::types::composite_2::set,
    traits_test_schema::schema::types::composite_5::composite,
    traits_test_schema::schema::messages::msg_3::field_5,
    traits_test_schema::schema::types::composite_7::uint32_req>;

template<typename T>
using HasOffsetTraitTest = TraitsContainer<T>;

TYPED_TEST_SUITE(HasOffsetTraitTest, HasOffsetTags);

TYPED_TEST(HasOffsetTraitTest, OffsetIsProvidedWhenItIsFixed)
{
    // this means all fields, inline defined types and refs but not public types
    using traits = typename TestFixture::traits;

    ASSERT_EQ(traits::offset(), g_custom_offset);
    IS_NOEXCEPT(traits::offset());
}

TEST(EnumValueTraitsTest, ProvidesTheSameValuesAsSchemaXml)
{
    using traits =
        sbepp::enum_value_traits<traits_test_schema::schema::types::enum_1::a>;

    ASSERT_STREQ(traits::name(), "a");
    ASSERT_STREQ(traits::description(), "a description");
    ASSERT_EQ(traits::value(), traits_test_schema::types::enum_1::a);
    IS_NOEXCEPT(traits::name());
    IS_NOEXCEPT(traits::description());
    IS_NOEXCEPT(traits::value());
}

TEST(SetTraitsTest, ProvidesTheSameValuesAsSchemaXml)
{
    using traits = sbepp::set_traits<traits_test_schema::schema::types::set_1>;

    ASSERT_STREQ(traits::name(), "set_1");
    ASSERT_STREQ(traits::description(), "set description");
    IS_SAME_TYPE(traits::encoding_type, std::uint8_t);
    IS_SAME_TYPE(traits::value_type, traits_test_schema::types::set_1);
    IS_NOEXCEPT(traits::name());
    IS_NOEXCEPT(traits::description());
}

TEST(SetChoiceTraitsTest, ProvidesTheSameValuesAsSchemaXml)
{
    using traits =
        sbepp::set_choice_traits<traits_test_schema::schema::types::set_1::a>;

    ASSERT_STREQ(traits::name(), "a");
    ASSERT_STREQ(traits::description(), "choice description");
    ASSERT_EQ(traits::index(), 1);
    IS_NOEXCEPT(traits::name());
    IS_NOEXCEPT(traits::description());
    IS_NOEXCEPT(traits::index());
}

TEST(CompositeTraitsTest, ProvidesTheSameValuesAsSchemaXml)
{
    using traits =
        sbepp::composite_traits<traits_test_schema::schema::types::composite_3>;

    ASSERT_STREQ(traits::name(), "composite_3");
    ASSERT_STREQ(traits::description(), "composite description");
    ASSERT_STREQ(traits::semantic_type(), "composite semantic type");
    IS_SAME_TYPE(
        traits::value_type<char>, traits_test_schema::types::composite_3<char>);
    IS_NOEXCEPT(traits::name());
    IS_NOEXCEPT(traits::description());
    IS_NOEXCEPT(traits::semantic_type());
}

using NoSemanticTypeTags = ::testing::Types<
    traits_test_schema::schema::types::composite_4,
    traits_test_schema::schema::messages::msg_2,
    traits_test_schema::schema::messages::msg_4::group_2>;

template<typename T>
using NoSemanticTypeTraitTest = TraitsContainer<T>;

TYPED_TEST_SUITE(NoSemanticTypeTraitTest, NoSemanticTypeTags);

TYPED_TEST(
    NoSemanticTypeTraitTest, SemanticTypeReturnsEmptyStringWhenNotProvided)
{
    using traits = typename TestFixture::traits;

    ASSERT_STREQ(traits::semantic_type(), "");
    IS_NOEXCEPT(traits::semantic_type());
}

TEST(MessageTraitsTest, ProvidesTheSameValuesAsSchemaXml)
{
    using traits =
        sbepp::message_traits<traits_test_schema::schema::messages::msg_1>;

    ASSERT_STREQ(traits::name(), "msg_1");
    ASSERT_STREQ(traits::description(), "message description");
    ASSERT_EQ(traits::id(), 1);
    ASSERT_STREQ(traits::semantic_type(), "message semantic type");
    IS_SAME_TYPE(
        traits::value_type<char>, traits_test_schema::messages::msg_1<char>);
    IS_SAME_TYPE(traits::schema_tag, traits_test_schema::schema);
    IS_NOEXCEPT(traits::name());
    IS_NOEXCEPT(traits::description());
    IS_NOEXCEPT(traits::id());
    IS_NOEXCEPT(traits::semantic_type());
}

TEST(MessageTraitsTest, DefaultBlockLengthIsSumOfFieldSizes)
{
    using message_tag = traits_test_schema::schema::messages::msg_1;
    using traits = sbepp::message_traits<message_tag>;

    ASSERT_EQ(
        traits::block_length(),
        sizeof(sbepp::field_traits<message_tag::field>::value_type));
    IS_NOEXCEPT(traits::block_length());
}

TEST(MessageTraitsTest, BlockLengthReturnsValueFromSchemaIfProvided)
{
    using traits =
        sbepp::message_traits<traits_test_schema::schema::messages::msg_2>;

    ASSERT_EQ(traits::block_length(), 10);
    IS_NOEXCEPT(traits::block_length());
}

TEST(FieldTraitsTest, ProvidesTheSameValuesAsSchemaXml)
{
    using traits = sbepp::field_traits<
        traits_test_schema::schema::messages::msg_3::field_1>;

    ASSERT_STREQ(traits::name(), "field_1");
    ASSERT_EQ(traits::id(), 1);
    IS_NOEXCEPT(traits::name());
    IS_NOEXCEPT(traits::id());
}

TEST(FieldTraitsTest, ProvidesCorrectPresence)
{
    using message_tag = traits_test_schema::schema::messages::msg_3;

    ASSERT_EQ(
        sbepp::field_traits<message_tag::field_1>::presence(),
        sbepp::field_presence::required);
    ASSERT_EQ(
        sbepp::field_traits<message_tag::field_2>::presence(),
        sbepp::field_presence::required);
    ASSERT_EQ(
        sbepp::field_traits<message_tag::field_3>::presence(),
        sbepp::field_presence::optional);
    ASSERT_EQ(
        sbepp::field_traits<message_tag::field_4>::presence(),
        sbepp::field_presence::constant);
    ASSERT_EQ(
        sbepp::field_traits<message_tag::field_6>::presence(),
        sbepp::field_presence::required);
    ASSERT_EQ(
        sbepp::field_traits<message_tag::field_7>::presence(),
        sbepp::field_presence::required);
    ASSERT_EQ(
        sbepp::field_traits<message_tag::field_8>::presence(),
        sbepp::field_presence::required);
    ASSERT_EQ(
        sbepp::field_traits<message_tag::field_9>::presence(),
        sbepp::field_presence::required);
    ASSERT_EQ(
        sbepp::field_traits<message_tag::field_10>::presence(),
        sbepp::field_presence::required);
    ASSERT_EQ(
        sbepp::field_traits<message_tag::field_11>::presence(),
        sbepp::field_presence::optional);
    ASSERT_EQ(
        sbepp::field_traits<message_tag::field_12>::presence(),
        sbepp::field_presence::constant);
    IS_NOEXCEPT(sbepp::field_traits<message_tag::field_1>::presence());
}

TEST(FieldTraitsTest, ProvidesCorrectValueTypeAndItsTag)
{
    using message_tag = traits_test_schema::schema::messages::msg_3;
    using types_tag = traits_test_schema::schema::types;

    // built-in types don't have tags
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_1>::value_type, sbepp::uint32_t);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_3>::value_type,
        sbepp::uint32_opt_t);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_4>::value_type, std::uint32_t);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_6>::value_type,
        traits_test_schema::types::set_1);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_6>::value_type_tag,
        types_tag::set_1);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_7>::value_type,
        traits_test_schema::types::enum_1);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_7>::value_type_tag,
        types_tag::enum_1);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_8>::value_type<char>,
        traits_test_schema::types::composite_1<char>);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_8>::value_type_tag,
        types_tag::composite_1);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_9>::value_type<char>,
        traits_test_schema::types::str128<char>);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_9>::value_type_tag,
        types_tag::str128);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_10>::value_type,
        traits_test_schema::types::uint32_req);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_10>::value_type_tag,
        types_tag::uint32_req);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_11>::value_type,
        traits_test_schema::types::uint32_opt);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_11>::value_type_tag,
        types_tag::uint32_opt);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_12>::value_type,
        traits_test_schema::types::uint32_const);
    IS_SAME_TYPE(
        sbepp::field_traits<message_tag::field_12>::value_type, std::uint32_t);
    // constants don't have tags
}

TEST(GroupTraitsTest, ProvidesTheSameValuesAsSchemaXml)
{
    using traits = sbepp::group_traits<
        traits_test_schema::schema::messages::msg_4::group_1>;

    ASSERT_STREQ(traits::name(), "group_1");
    ASSERT_STREQ(traits::description(), "group description");
    ASSERT_EQ(traits::id(), 1);
    ASSERT_EQ(traits::block_length(), 10);
    ASSERT_STREQ(traits::semantic_type(), "group semantic type");
    (void)traits::value_type<char>{};
    IS_SAME_TYPE(
        traits::dimension_type<char>,
        traits_test_schema::types::customGroupSizeEncoding<char>);
    IS_SAME_TYPE(
        traits::dimension_type_tag,
        traits_test_schema::schema::types::customGroupSizeEncoding);
    (void)traits::entry_type<char>{};
    IS_NOEXCEPT(traits::name());
    IS_NOEXCEPT(traits::description());
    IS_NOEXCEPT(traits::id());
    IS_NOEXCEPT(traits::block_length());
    IS_NOEXCEPT(traits::semantic_type());
}

TEST(GroupTraitsTest, DefaultDimensionIsGroupSizeEncoding)
{
    IS_SAME_TYPE(
        sbepp::group_traits<traits_test_schema::schema::messages::msg_4::
                                group_2>::dimension_type<char>,
        traits_test_schema::types::groupSizeEncoding<char>);
    IS_SAME_TYPE(
        sbepp::group_traits<traits_test_schema::schema::messages::msg_4::
                                group_2>::dimension_type_tag,
        traits_test_schema::schema::types::groupSizeEncoding);
}

TEST(GroupTraitsTest, DefaultBlockLengthIsSumOfFieldSizes)
{
    using traits = sbepp::group_traits<
        traits_test_schema::schema::messages::msg_4::group_2>;
    ASSERT_EQ(
        traits::block_length(),
        sizeof(std::declval<traits::entry_type<char>>().field_1()));
    IS_NOEXCEPT(traits::block_length());
}

TEST(DataTraitsTest, ProvidesTheSameValuesAsSchemaXml)
{
    using traits =
        sbepp::data_traits<traits_test_schema::schema::messages::msg_5::data_1>;

    ASSERT_STREQ(traits::name(), "data_1");
    ASSERT_EQ(traits::id(), 1);
    ASSERT_STREQ(traits::description(), "data description");
    (void)traits::value_type<char>{};
    IS_SAME_TYPE(
        traits::length_type,
        sbepp::type_traits<traits_test_schema::schema::types::varDataEncoding::
                               length>::value_type);
    IS_SAME_TYPE(
        traits::length_type_tag,
        traits_test_schema::schema::types::varDataEncoding::length);
    IS_NOEXCEPT(traits::name());
    IS_NOEXCEPT(traits::id());
    IS_NOEXCEPT(traits::description());
}

TEST(TypeTraitsTest, ProvidesTheSameValuesAsSchemaXml)
{
    using traits =
        sbepp::type_traits<traits_test_schema::schema::types::type_1>;

    ASSERT_STREQ(traits::name(), "type_1");
    IS_SAME_TYPE(traits::primitive_type, std::uint32_t);
    ASSERT_EQ(traits::min_value(), 0);
    ASSERT_EQ(traits::max_value(), 10);
    ASSERT_EQ(traits::null_value(), 11);
    ASSERT_STREQ(traits::semantic_type(), "type semantic type");
    ASSERT_STREQ(traits::character_encoding(), "character encoding");
    ASSERT_EQ(traits::length(), 1);
    ASSERT_EQ(traits::presence(), sbepp::field_presence::optional);
    IS_NOEXCEPT(traits::name());
    IS_NOEXCEPT(traits::min_value());
    IS_NOEXCEPT(traits::max_value());
    IS_NOEXCEPT(traits::null_value());
    IS_NOEXCEPT(traits::semantic_type());
    IS_NOEXCEPT(traits::character_encoding());
    IS_NOEXCEPT(traits::length());
    IS_NOEXCEPT(traits::presence());
}

TEST(TypeTraitsTest, ProvidesCorrectPresence)
{
    using types_tag = traits_test_schema::schema::types;

    ASSERT_EQ(
        sbepp::type_traits<types_tag::uint32_req>::presence(),
        sbepp::field_presence::required);
    ASSERT_EQ(
        sbepp::type_traits<types_tag::uint32_opt>::presence(),
        sbepp::field_presence::optional);
    ASSERT_EQ(
        sbepp::type_traits<types_tag::uint32_const>::presence(),
        sbepp::field_presence::constant);
}

TEST(TypeTraitsTest, LengthIsOneByDefault)
{
    using traits =
        sbepp::type_traits<traits_test_schema::schema::types::uint32_req>;

    ASSERT_EQ(traits::length(), 1);
}

TEST(TypeTraitsTest, ProvidesCorrectLengthForArrays)
{
    using traits =
        sbepp::type_traits<traits_test_schema::schema::types::str128>;

    ASSERT_EQ(traits::length(), 128);
}

TEST(TypeTraitsTest, InheritsMinMaxNullFromPrimitiveType)
{
    using traits =
        sbepp::type_traits<traits_test_schema::schema::types::uint32_opt>;

    ASSERT_EQ(traits::min_value(), sbepp::uint32_opt_t::min_value());
    ASSERT_EQ(traits::max_value(), sbepp::uint32_opt_t::max_value());
    ASSERT_EQ(traits::null_value(), sbepp::uint32_opt_t::null_value());
}

TEST(RefTraitsTests, ProvideTheSameTraitsAsReferredType)
{
    using composite_tag = traits_test_schema::schema::types::composite_6;

    (void)sbepp::type_traits<composite_tag::ref_1>{};
    (void)sbepp::enum_traits<composite_tag::ref_2>{};
    (void)sbepp::set_traits<composite_tag::ref_3>{};
    (void)sbepp::composite_traits<composite_tag::ref_4>{};
}

using FieldContainerTags = ::testing::Types<
    traits_test_schema::schema::types::composite_8,
    traits_test_schema::schema::messages::msg_6,
    traits_test_schema::schema::messages::msg_6::group_1>;

template<typename T>
using DefaultOffsetTraitTest = TraitsContainer<T>;

TYPED_TEST_SUITE(DefaultOffsetTraitTest, FieldContainerTags);

TYPED_TEST(DefaultOffsetTraitTest, ProvidesCorrectOffsetForConsecutiveFields)
{
    using tag = typename TestFixture::tag;
    using field_1_traits = sbe_traits<typename tag::field_1>;
    using field_2_traits = sbe_traits<typename tag::field_2>;
    using field_3_traits = sbe_traits<typename tag::field_3>;

    ASSERT_EQ(field_1_traits::offset(), 0);
    ASSERT_EQ(
        field_2_traits::offset(),
        field_1_traits::offset() + sizeof(typename field_1_traits::value_type));
    ASSERT_EQ(
        field_3_traits::offset(),
        field_2_traits::offset() + sizeof(typename field_2_traits::value_type));
}

TEST(DataTraitsTest, SizeBytesReturnsSumOfSizeOfLengthTypeAndGivenSize)
{
    using tag = traits_test_schema::schema::messages::msg_9::data_1;
    using traits = sbepp::data_traits<tag>;
    constexpr auto data_size = 2;

    STATIC_ASSERT(
        traits::size_bytes(data_size)
        == sizeof(traits::length_type) + data_size);
    IS_NOEXCEPT(traits::size_bytes(data_size));
}

TEST(GroupTraitsTest, EmptyGroupSizeIsHeaderSize)
{
    using tag = traits_test_schema::schema::messages::msg_10::group_2;
    using traits = sbepp::group_traits<tag>;
    constexpr auto group_size = 2;
    STATIC_ASSERT(traits::block_length() == 0);

    STATIC_ASSERT(
        traits::size_bytes(group_size)
        == sbepp::composite_traits<traits::dimension_type_tag>::size_bytes());
    // group size doesn't matter because group has no fields
    STATIC_ASSERT(
        traits::size_bytes(group_size) == traits::size_bytes(group_size + 1));
    IS_NOEXCEPT(traits::size_bytes(group_size));
}

TEST(GroupTraitsTest, FlatGroupSizeIsHeaderAndBlockLengthBySize)
{
    using tag = traits_test_schema::schema::messages::msg_10::group_1;
    using traits = sbepp::group_traits<tag>;
    constexpr auto group_size = 2;
    STATIC_ASSERT(traits::block_length() != 0);
    constexpr auto valid_size =
        sbepp::composite_traits<traits::dimension_type_tag>::size_bytes()
        + group_size * traits::block_length();

    STATIC_ASSERT(traits::size_bytes(group_size) == valid_size);
    IS_NOEXCEPT(traits::size_bytes(group_size));
}

TEST(GroupTraitsTest, GroupWithDataSizeBytesHasTotalDataSizeParameter)
{
    using tag = traits_test_schema::schema::messages::msg_14::group_1::group_2;
    using traits = sbepp::group_traits<tag>;
    constexpr auto group_size = 2;
    constexpr auto total_data_size = 10;
    constexpr auto valid_size =
        sbepp::composite_traits<traits::dimension_type_tag>::size_bytes()
        + group_size
              * (traits::block_length()
                 + sbepp::data_traits<tag::data>::size_bytes(0))
        + total_data_size;

    STATIC_ASSERT(
        traits::size_bytes(group_size, total_data_size) == valid_size);
    IS_NOEXCEPT(traits::size_bytes(group_size, total_data_size));
}

TEST(GroupTraitsTest, NestedGroupSizeBytesHasSizeParameterForEachGroup)
{
    using tag = traits_test_schema::schema::messages::msg_14::group_1;
    using traits = sbepp::group_traits<tag>;
    constexpr auto root_group_size = 2;
    constexpr auto child_group_size = 3;
    constexpr auto total_data_size = 10;
    constexpr auto valid_size =
        sbepp::composite_traits<traits::dimension_type_tag>::size_bytes()
        + root_group_size
              * (traits::block_length()
                 + sbepp::composite_traits<sbepp::group_traits<
                     tag::group_2>::dimension_type_tag>::size_bytes())
        + child_group_size
              * (sbepp::group_traits<tag::group_2>::block_length()
                 + sbepp::data_traits<tag::group_2::data>::size_bytes(0))
        + total_data_size;

    STATIC_ASSERT(
        traits::size_bytes(root_group_size, child_group_size, total_data_size)
        == valid_size);
    IS_NOEXCEPT(
        traits::size_bytes(root_group_size, child_group_size, total_data_size));
}

TEST(MessageTraitsTest, EmptyMessageSizeIsHeaderSize)
{
    using tag = traits_test_schema::schema::messages::msg_7;
    using traits = sbepp::message_traits<tag>;
    STATIC_ASSERT(traits::block_length() == 0);
    constexpr auto valid_size = sbepp::composite_traits<sbepp::schema_traits<
        traits::schema_tag>::header_type_tag>::size_bytes();

    STATIC_ASSERT(traits::size_bytes() == valid_size);
    IS_NOEXCEPT(traits::size_bytes());
}

TEST(MessageTraitsTest, FlatMessageSizeIsHeaderSizeAndBlockLength)
{
    using tag = traits_test_schema::schema::messages::msg_8;
    using traits = sbepp::message_traits<tag>;
    STATIC_ASSERT(traits::block_length() != 0);
    constexpr auto valid_size =
        sbepp::composite_traits<sbepp::schema_traits<
            traits::schema_tag>::header_type_tag>::size_bytes()
        + traits::block_length();

    STATIC_ASSERT(traits::size_bytes() == valid_size);
    IS_NOEXCEPT(traits::size_bytes());
}

TEST(MessageTraitsTest, MessageWithDataSizeBytesHasTotalSizeBytesParameter)
{
    using tag = traits_test_schema::schema::messages::msg_9;
    using traits = sbepp::message_traits<tag>;
    constexpr auto data_1_size = 7;
    constexpr auto data_2_size = 27;
    constexpr auto total_data_size = data_1_size + data_2_size;
    constexpr auto valid_size =
        sbepp::composite_traits<sbepp::schema_traits<
            traits::schema_tag>::header_type_tag>::size_bytes()
        + traits::block_length()
        + sbepp::data_traits<tag::data_1>::size_bytes(data_1_size)
        + sbepp::data_traits<tag::data_2>::size_bytes(data_2_size);

    STATIC_ASSERT(traits::size_bytes(total_data_size) == valid_size);
    IS_NOEXCEPT(traits::size_bytes(total_data_size));
}

TEST(MessageTraitsTest, HasSizeParameterForEachGroupFromTopToBottom1)
{
    using tag = traits_test_schema::schema::messages::msg_10;
    using traits = sbepp::message_traits<tag>;
    constexpr auto group_1_size = 2;
    constexpr auto group_2_size = 3;
    constexpr auto valid_size =
        sbepp::composite_traits<sbepp::schema_traits<
            traits::schema_tag>::header_type_tag>::size_bytes()
        + traits::block_length()
        + sbepp::group_traits<tag::group_1>::size_bytes(group_1_size)
        + sbepp::group_traits<tag::group_2>::size_bytes(group_2_size);

    STATIC_ASSERT(traits::size_bytes(group_1_size, group_2_size) == valid_size);
    IS_NOEXCEPT(traits::size_bytes(group_1_size, group_2_size));
}

TEST(MessageTraitsTest, HasSizeParameterForEachGroupFromTopToBottom2)
{
    using tag = traits_test_schema::schema::messages::msg_12;
    using traits = sbepp::message_traits<tag>;
    constexpr auto group_1_size = 2;
    constexpr auto group_2_size = 3;
    constexpr auto valid_size =
        sbepp::composite_traits<sbepp::schema_traits<
            traits::schema_tag>::header_type_tag>::size_bytes()
        + traits::block_length()
        + sbepp::group_traits<tag::group_1>::size_bytes(group_1_size)
        + sbepp::group_traits<tag::group_2>::size_bytes(group_2_size);

    STATIC_ASSERT(traits::size_bytes(group_1_size, group_2_size) == valid_size);
    IS_NOEXCEPT(traits::size_bytes(group_1_size, group_2_size));
}

TEST(MessageTraitsTest, HasTrailingTotalDataSizeParameterIfHasDataAtAnyLevel1)
{
    using tag = traits_test_schema::schema::messages::msg_11;
    using traits = sbepp::message_traits<tag>;
    constexpr auto total_data_size = 10;
    constexpr auto group_size = 4;
    constexpr auto valid_size =
        sbepp::composite_traits<sbepp::schema_traits<
            traits::schema_tag>::header_type_tag>::size_bytes()
        + traits::block_length()
        + sbepp::group_traits<tag::group>::size_bytes(group_size)
        + sbepp::data_traits<tag::data>::size_bytes(total_data_size);

    STATIC_ASSERT(
        traits::size_bytes(group_size, total_data_size) == valid_size);
    IS_NOEXCEPT(traits::size_bytes(group_size, total_data_size));
}

TEST(MessageTraitsTest, HasSizeParameterForEachGroupFromTopToBottom3)
{
    using tag = traits_test_schema::schema::messages::msg_13;
    using traits = sbepp::message_traits<tag>;
    constexpr auto group_1_size = 2;
    constexpr auto group_1_group_2_size = 3;
    constexpr auto valid_size =
        sbepp::composite_traits<sbepp::schema_traits<
            traits::schema_tag>::header_type_tag>::size_bytes()
        + traits::block_length()
        + sbepp::group_traits<tag::group_1>::size_bytes(
            group_1_size, group_1_group_2_size);

    STATIC_ASSERT(
        traits::size_bytes(group_1_size, group_1_group_2_size) == valid_size);
    IS_NOEXCEPT(traits::size_bytes(group_1_size, group_1_group_2_size));
}

TEST(MessageTraitsTest, HasTrailingTotalDataSizeParameterIfHasDataAtAnyLevel2)
{
    using tag = traits_test_schema::schema::messages::msg_14;
    using traits = sbepp::message_traits<tag>;
    constexpr auto total_data_size = 10;
    constexpr auto group_1_size = 2;
    constexpr auto group_1_group_2_size = 3;
    constexpr auto valid_size =
        sbepp::composite_traits<sbepp::schema_traits<
            traits::schema_tag>::header_type_tag>::size_bytes()
        + traits::block_length()
        + sbepp::group_traits<tag::group_1>::size_bytes(
            group_1_size, group_1_group_2_size, total_data_size);

    STATIC_ASSERT(
        traits::size_bytes(group_1_size, group_1_group_2_size, total_data_size)
        == valid_size);
    IS_NOEXCEPT(traits::size_bytes(
        group_1_size, group_1_group_2_size, total_data_size));
}

TEST(MessageTraitsTest, HasTrailingTotalDataSizeParameterIfHasDataAtAnyLevel3)
{
    using tag = traits_test_schema::schema::messages::msg_15;
    using traits = sbepp::message_traits<tag>;
    constexpr auto group_1_group_2_data_size = 3;
    constexpr auto group_1_data_size = 6;
    constexpr auto msg_data_size = 10;
    constexpr auto total_data_size =
        group_1_group_2_data_size + group_1_data_size + msg_data_size;
    constexpr auto group_1_size = 2;
    constexpr auto group_1_group_2_size = 3;
    constexpr auto valid_size =
        sbepp::composite_traits<sbepp::schema_traits<
            traits::schema_tag>::header_type_tag>::size_bytes()
        + traits::block_length()
        + sbepp::group_traits<tag::group_1>::size_bytes(
            group_1_size,
            group_1_group_2_size,
            group_1_group_2_data_size + group_1_data_size)
        + sbepp::data_traits<tag::data>::size_bytes(msg_data_size);

    STATIC_ASSERT(
        traits::size_bytes(group_1_size, group_1_group_2_size, total_data_size)
        == valid_size);
    IS_NOEXCEPT(traits::size_bytes(
        group_1_size, group_1_group_2_size, total_data_size));
}

TEST(MessageTraitsTest, AddsLevelDepthSuffixToAmbiguousSizeBytesParameters)
{
    // here, we mostly test that code compiles, without adding the `_<depth>`
    // suffix, `size_bytes` would have multiple parameters with the same name
    // and compilation error
    using tag = traits_test_schema::schema::messages::msg_16;
    using traits = sbepp::message_traits<tag>;
    constexpr auto group_1_size = 1;
    constexpr auto group_1_group_2_size = 2;
    constexpr auto group_1_group_2_size_0 = 3;
    constexpr auto group_3_size = 4;
    constexpr auto group_3_group_1_group_2_size = 5;
    constexpr auto group_3_group_1_size = 6;
    constexpr auto group_3_group_1_group_2_size_2 = 7;
    constexpr auto group_3_group_1_data_size = 9;
    constexpr auto total_data_size = group_3_group_1_data_size;

    constexpr auto valid_size =
        sbepp::composite_traits<sbepp::schema_traits<
            traits::schema_tag>::header_type_tag>::size_bytes()
        + traits::block_length()
        + sbepp::group_traits<tag::group_1>::size_bytes(
            group_1_size, group_1_group_2_size)
        + sbepp::group_traits<tag::group_1_group_2>::size_bytes(
            group_1_group_2_size)
        + sbepp::group_traits<tag::group_3>::size_bytes(
            group_3_size,
            group_3_group_1_group_2_size,
            group_3_group_1_size,
            group_3_group_1_group_2_size_2,
            group_3_group_1_data_size);

    STATIC_ASSERT(
        traits::size_bytes(
            group_1_size,
            group_1_group_2_size,
            group_1_group_2_size_0,
            group_3_size,
            group_3_group_1_group_2_size,
            group_3_group_1_size,
            group_3_group_1_group_2_size_2,
            total_data_size)
        == valid_size);
}

TEST(MessageTraitsTest, SizeBytesEqualToSbeppSizeBytes)
{
    using tag = traits_test_schema::schema::messages::msg_14;
    using traits = sbepp::message_traits<tag>;
    constexpr auto total_data_size = 16;
    constexpr auto group_1_size = 2;
    constexpr auto group_1_group_2_size = 4;
    constexpr auto predicted_size =
        traits::size_bytes(group_1_size, group_1_group_2_size, total_data_size);
    std::array<char, predicted_size> buf{};

    auto m = sbepp::make_view<traits::value_type>(buf.data(), buf.size());
    sbepp::fill_message_header(m);
    auto g1 = m.group_1();
    sbepp::fill_group_header(g1, group_1_size);
    for(auto entry1 : g1)
    {
        auto g2 = entry1.group_2();
        // 2 `group_2` entries in each `group_1` entry
        sbepp::fill_group_header(g2, group_1_group_2_size / g1.size());
        for(auto entry2 : g2)
        {
            auto d = entry2.data();
            // 4 bytes of payload for each `data`
            d.resize(total_data_size / group_1_group_2_size);
        }
    }

    ASSERT_EQ(predicted_size, sbepp::size_bytes(m));
}

using TemplatedValueTypeTags = ::testing::Types<
    traits_test_schema::schema::messages::msg_1,
    traits_test_schema::schema::messages::msg_4::group_1,
    traits_test_schema::schema::types::messageHeader,
    traits_test_schema::schema::types::str128>;

using SimpleValueTypeTags = ::testing::Types<
    traits_test_schema::schema::types::enum_1,
    traits_test_schema::schema::types::set_1,
    traits_test_schema::schema::types::str_const,
    traits_test_schema::schema::types::uint32_opt>;

template<typename T>
using TemplatedValueTypeTraitsTagTest = TraitsContainer<T>;
template<typename T>
using SimpleValueTypeTraitsTagTest = TraitsContainer<T>;

TYPED_TEST_SUITE(TemplatedValueTypeTraitsTagTest, TemplatedValueTypeTags);
TYPED_TEST_SUITE(SimpleValueTypeTraitsTagTest, SimpleValueTypeTags);

TYPED_TEST(TemplatedValueTypeTraitsTagTest, ProvidesCorrectTraitsTag)
{
    using tag = typename TestFixture::tag;
    using representation_type =
        typename TestFixture::traits::template value_type<char>;

    IS_SAME_TYPE(sbepp::traits_tag_t<representation_type>, tag);
}

TYPED_TEST(SimpleValueTypeTraitsTagTest, ProvidesCorrectTraitsTag)
{
    using tag = typename TestFixture::tag;
    using representation_type = typename TestFixture::traits::value_type;

    IS_SAME_TYPE(sbepp::traits_tag_t<representation_type>, tag);
}

namespace constexpr_tests
{
namespace schema_traits
{
using traits = sbepp::schema_traits<traits_test_schema::schema>;

constexpr auto t1 = traits::package();
constexpr auto t2 = traits::id();
constexpr auto t3 = traits::version();
constexpr auto t4 = traits::semantic_version();
constexpr auto t5 = traits::byte_order();
constexpr auto t6 = traits::description();
} // namespace schema_traits

namespace type_traits
{
using traits = sbepp::type_traits<traits_test_schema::schema::types::type_1>;

constexpr auto t1 = traits::name();
constexpr auto t2 = traits::description();
constexpr auto t3 = traits::presence();
constexpr auto t4 = traits::min_value();
constexpr auto t5 = traits::max_value();
constexpr auto t6 = traits::null_value();
constexpr auto t7 = traits::length();
constexpr auto t8 = traits::semantic_type();
constexpr auto t9 = traits::since_version();
constexpr auto t10 = traits::character_encoding();
constexpr auto t11 = traits::deprecated();

using traits2 = sbepp::type_traits<
    traits_test_schema::schema::types::composite_7::uint32_req>;

constexpr auto t12 = traits2::offset();
} // namespace type_traits

namespace enum_traits
{
using traits = sbepp::enum_traits<traits_test_schema::schema::types::enum_1>;

constexpr auto t1 = traits::name();
constexpr auto t2 = traits::description();
constexpr auto t3 = traits::since_version();
constexpr auto t4 = traits::deprecated();

using traits2 = sbepp::enum_traits<
    traits_test_schema::schema::types::composite_1::enumeration>;

constexpr auto t5 = traits2::offset();
} // namespace enum_traits

namespace enum_value_traits
{
using traits =
    sbepp::enum_value_traits<traits_test_schema::schema::types::enum_1::a>;

constexpr auto t1 = traits::name();
constexpr auto t2 = traits::description();
constexpr auto t3 = traits::since_version();
constexpr auto t4 = traits::deprecated();
constexpr auto t5 = traits::value();
} // namespace enum_value_traits

namespace set_traits
{
using traits = sbepp::set_traits<traits_test_schema::schema::types::set_1>;

constexpr auto t1 = traits::name();
constexpr auto t2 = traits::description();
constexpr auto t3 = traits::since_version();
constexpr auto t4 = traits::deprecated();

using traits2 =
    sbepp::set_traits<traits_test_schema::schema::types::composite_2::set>;

constexpr auto t5 = traits2::offset();
} // namespace set_traits

namespace set_choice_traits
{
using traits =
    sbepp::set_choice_traits<traits_test_schema::schema::types::set_1::a>;

constexpr auto t1 = traits::name();
constexpr auto t2 = traits::description();
constexpr auto t3 = traits::since_version();
constexpr auto t4 = traits::deprecated();
constexpr auto t5 = traits::index();
} // namespace set_choice_traits

namespace composite_traits
{
using traits =
    sbepp::composite_traits<traits_test_schema::schema::types::composite_3>;

constexpr auto t1 = traits::name();
constexpr auto t2 = traits::description();
constexpr auto t3 = traits::since_version();
constexpr auto t4 = traits::deprecated();
constexpr auto t5 = traits::semantic_type();

using traits2 = sbepp::composite_traits<
    traits_test_schema::schema::types::composite_5::composite>;

constexpr auto t6 = traits2::offset();
} // namespace composite_traits

namespace message_traits
{
using traits =
    sbepp::message_traits<traits_test_schema::schema::messages::msg_1>;

constexpr auto t1 = traits::name();
constexpr auto t2 = traits::description();
constexpr auto t3 = traits::id();
constexpr auto t4 = traits::block_length();
constexpr auto t5 = traits::semantic_type();
constexpr auto t6 = traits::since_version();
constexpr auto t7 = traits::deprecated();
} // namespace message_traits

namespace field_traits
{
using traits =
    sbepp::field_traits<traits_test_schema::schema::messages::msg_3::field_1>;

constexpr auto t1 = traits::name();
constexpr auto t2 = traits::description();
constexpr auto t3 = traits::id();
constexpr auto t4 = traits::presence();
constexpr auto t5 = traits::offset();
constexpr auto t6 = traits::since_version();
constexpr auto t7 = traits::deprecated();
} // namespace field_traits

namespace group_traits
{
using traits =
    sbepp::group_traits<traits_test_schema::schema::messages::msg_4::group_1>;

constexpr auto t1 = traits::name();
constexpr auto t2 = traits::description();
constexpr auto t3 = traits::id();
constexpr auto t4 = traits::block_length();
constexpr auto t5 = traits::semantic_type();
constexpr auto t6 = traits::since_version();
constexpr auto t7 = traits::deprecated();
} // namespace group_traits

namespace data_traits
{
using traits =
    sbepp::data_traits<traits_test_schema::schema::messages::msg_5::data_1>;

constexpr auto t1 = traits::name();
constexpr auto t2 = traits::description();
constexpr auto t3 = traits::id();
constexpr auto t4 = traits::since_version();
constexpr auto t5 = traits::deprecated();
} // namespace data_traits
} // namespace constexpr_tests
} // namespace
