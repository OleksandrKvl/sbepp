// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <benchmark_schema/benchmark_schema.hpp>
#include <sbepp/benchmark/test_data.hpp>
#include <sbepp/benchmark/message_generator.hpp>
#include <sbepp/benchmark/config.hpp>

#include <benchmark/benchmark.h>

#include <cstdint>
#include <numeric>
#include <cassert>

namespace sbepp
{
namespace benchmark
{
namespace sbepp_reader
{
template<typename Level>
std::uint64_t get_level_fields_checksum(Level l)
{
    std::uint64_t res{};
    res += *l.field1();
    res += *l.field2();
    res += *l.field3();
    res += *l.field4();
    res += *l.field5();

    return res;
}

template<typename Data>
std::uint64_t get_data_checksum(Data d)
{
    return std::accumulate(std::begin(d), std::end(d), std::uint64_t{});
}

template<typename Level>
std::uint64_t get_flat_group_checksum(Level l)
{
    auto res = get_level_fields_checksum(l);
    for(const auto entry : l.flat_group())
    {
        res += get_level_fields_checksum(entry);
    }

    return res;
}

template<typename Level>
std::uint64_t get_nested_group_checksum(Level l)
{
    auto res = get_flat_group_checksum(l);
    for(const auto entry : l.nested_group())
    {
        res += get_level_fields_checksum(entry);
        res += get_data_checksum(entry.data());
    }

    return res;
}

template<typename Level>
std::uint64_t get_nested_group2_checksum(Level l)
{
    auto res = get_nested_group_checksum(l);
    for(const auto entry : l.nested_group2())
    {
        res += get_level_fields_checksum(entry);
        for(const auto entry2 : entry.nested_group())
        {
            res += get_level_fields_checksum(entry2);
            res += get_data_checksum(entry2.data());
        }
    }

    return res;
}

template<typename Level>
std::uint64_t get_whole_message_checksum(Level l)
{
    auto res = get_nested_group2_checksum(l);
    res += get_data_checksum(l.data());

    return res;
}

void top_level_fields_benchmark(::benchmark::State& state)
{
    message_generator msg_generator{
        config::get_min_group_size(state),
        config::get_max_group_size(state),
        config::get_min_data_size(state),
        config::get_max_data_size(state)};
    const auto test_data =
        msg_generator.generate(config::get_number_of_messages(state));

    for(auto _ : state)
    {
        std::uint64_t sum{};
        for(const auto& test : test_data)
        {
            auto msg = sbepp::make_view<benchmark_schema::messages::msg1>(
                test.buffer.data(), test.buffer.size());
            const auto checksum = get_level_fields_checksum(msg);
            assert(checksum == test.top_level_checksum);
            sum += checksum;
        }
        ::benchmark::DoNotOptimize(sum);
    }
}

void flat_group_benchmark(::benchmark::State& state)
{
    message_generator msg_generator{
        config::get_min_group_size(state),
        config::get_max_group_size(state),
        config::get_min_data_size(state),
        config::get_max_data_size(state)};
    const auto test_data =
        msg_generator.generate(config::get_number_of_messages(state));

    for(auto _ : state)
    {
        std::uint64_t sum{};
        for(const auto& test : test_data)
        {
            auto msg = sbepp::make_view<benchmark_schema::messages::msg1>(
                test.buffer.data(), test.buffer.size());
            const auto checksum = get_flat_group_checksum(msg);
            assert(checksum == test.flat_group_checksum);
            sum += checksum;
        }
        ::benchmark::DoNotOptimize(sum);
    }
}

void nested_group_benchmark(::benchmark::State& state)
{
    message_generator msg_generator{
        config::get_min_group_size(state),
        config::get_max_group_size(state),
        config::get_min_data_size(state),
        config::get_max_data_size(state)};
    const auto test_data =
        msg_generator.generate(config::get_number_of_messages(state));

    for(auto _ : state)
    {
        std::uint64_t sum{};
        for(const auto& test : test_data)
        {
            auto msg = sbepp::make_view<benchmark_schema::messages::msg1>(
                test.buffer.data(), test.buffer.size());
            const auto checksum = get_nested_group_checksum(msg);
            assert(checksum == test.nested_group_checksum);
            sum += checksum;
        }
        ::benchmark::DoNotOptimize(sum);
    }
}

void nested_group2_benchmark(::benchmark::State& state)
{
    message_generator msg_generator{
        config::get_min_group_size(state),
        config::get_max_group_size(state),
        config::get_min_data_size(state),
        config::get_max_data_size(state)};
    const auto test_data =
        msg_generator.generate(config::get_number_of_messages(state));

    for(auto _ : state)
    {
        std::uint64_t sum{};
        for(const auto& test : test_data)
        {
            auto msg = sbepp::make_view<benchmark_schema::messages::msg1>(
                test.buffer.data(), test.buffer.size());
            const auto checksum = get_nested_group2_checksum(msg);
            assert(checksum == test.nested_group2_checksum);
            sum += checksum;
        }
        ::benchmark::DoNotOptimize(sum);
    }
}

void whole_message_benchmark(::benchmark::State& state)
{
    message_generator msg_generator{
        config::get_min_group_size(state),
        config::get_max_group_size(state),
        config::get_min_data_size(state),
        config::get_max_data_size(state)};
    const auto test_data =
        msg_generator.generate(config::get_number_of_messages(state));

    for(auto _ : state)
    {
        std::uint64_t sum{};
        for(const auto& test : test_data)
        {
            auto msg = sbepp::make_view<benchmark_schema::messages::msg1>(
                test.buffer.data(), test.buffer.size());
            const auto checksum = get_whole_message_checksum(msg);
            assert(checksum == test.data_checksum);
            sum += checksum;
        }
        ::benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(sbepp_reader::top_level_fields_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(sbepp_reader::flat_group_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(sbepp_reader::nested_group_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(sbepp_reader::nested_group2_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(sbepp_reader::whole_message_benchmark)
    ->Apply(config::configure_benchmark);
} // namespace sbepp_reader
} // namespace benchmark
} // namespace sbepp