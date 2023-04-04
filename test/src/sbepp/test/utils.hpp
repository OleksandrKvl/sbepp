// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

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
} // namespace utils
} // namespace test
} // namespace sbepp
