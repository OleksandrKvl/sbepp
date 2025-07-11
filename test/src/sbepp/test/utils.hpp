// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <sbepp/sbepp.hpp>

#include <type_traits>
#include <utility>

#define STATIC_ASSERT(...) static_assert(__VA_ARGS__, "")
#define STATIC_ASSERT_V(...) static_assert(__VA_ARGS__::value, "")
#define IS_SAME_TYPE(T1, T2) STATIC_ASSERT_V(std::is_same<T1, T2>)
#define IS_NOEXCEPT(op) STATIC_ASSERT((noexcept(op)))

namespace sbepp
{
namespace test
{
namespace utils
{
template<typename...>
using void_t = void;

template<bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template<typename F, typename... Args>
struct is_invocable_impl
{
    template<typename...>
    struct always_true : std::true_type
    {
    };

    template<typename F2, typename... Args2>
    static always_true<decltype(std::declval<F2>()(std::declval<Args2>()...))>
        test(int);

    template<typename...>
    static std::false_type test(...);

    using type = decltype(test<F, Args...>(0));
};

template<typename F, typename... Args>
struct is_invocable : is_invocable_impl<F, Args...>::type
{
};

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
} // namespace utils
} // namespace test
} // namespace sbepp
