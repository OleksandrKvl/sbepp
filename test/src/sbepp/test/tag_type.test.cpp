// SPDX-License-Identifier: MIT
// Copyright (c) 2025, Oleksandr Koval

#include <sbepp/test/utils.hpp>
#include <test_schema/test_schema.hpp>

#if SBEPP_HAS_CONCEPTS
#    define CHECK_TAG_TYPE(Tag, Type) STATIC_ASSERT(sbepp::Type##_tag<Tag>)
#elif SBEPP_HAS_INLINE_VARS
#    define CHECK_TAG_TYPE(Tag, Type) \
        STATIC_ASSERT(sbepp::is_##Type##_tag_v<Tag>)
#else
#    define CHECK_TAG_TYPE(Tag, Type) \
        STATIC_ASSERT_V(sbepp::is_##Type##_tag<Tag>)
#endif

CHECK_TAG_TYPE(test_schema::schema::types::uint32_req, type);
CHECK_TAG_TYPE(test_schema::schema::types::numbers_enum, enum);
CHECK_TAG_TYPE(test_schema::schema::types::numbers_enum::One, enum_value);
CHECK_TAG_TYPE(test_schema::schema::types::options_set, set);
CHECK_TAG_TYPE(test_schema::schema::types::options_set::A, set_choice);
CHECK_TAG_TYPE(test_schema::schema::types::composite_1, composite);
CHECK_TAG_TYPE(test_schema::schema::messages::msg2::composite, field);
CHECK_TAG_TYPE(test_schema::schema::messages::msg2::group, group);
CHECK_TAG_TYPE(test_schema::schema::messages::msg2::data, data);
CHECK_TAG_TYPE(test_schema::schema::messages::msg2, message);
CHECK_TAG_TYPE(test_schema::schema, schema);