// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <benchmark/benchmark.h>

#include <cstdint>

namespace sbepp
{
namespace benchmark
{
struct config
{
    static void configure_benchmark(::benchmark::internal::Benchmark* b)
    {
        // number of messages, min/max group size, min/max data size
        b->Args({1000, 0, 20, 0, 32});
        b->Args({1000, 10, 10, 10, 10});
    }

    static std::size_t get_number_of_messages(const ::benchmark::State& state)
    {
        return state.range(0);
    }

    static std::size_t get_min_group_size(const ::benchmark::State& state)
    {
        return state.range(1);
    }

    static std::size_t get_max_group_size(const ::benchmark::State& state)
    {
        return state.range(2);
    }

    static std::size_t get_min_data_size(const ::benchmark::State& state)
    {
        return state.range(3);
    }

    static std::size_t get_max_data_size(const ::benchmark::State& state)
    {
        return state.range(4);
    }
};
} // namespace benchmark
} // namespace sbepp