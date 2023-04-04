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
namespace sbepp_cursor_reader
{
template<typename Level, typename Cursor>
inline std::uint64_t get_level_fields_checksum(Level l, Cursor& c)
{
    std::uint64_t res{};
    res += *l.field1(c);
    res += *l.field2(c);
    res += *l.field3(c);
    res += *l.field4(c);
    res += *l.field5(c);

    return res;
}

template<typename Data>
inline std::uint64_t get_data_checksum(Data d)
{
    return std::accumulate(std::begin(d), std::end(d), std::uint64_t{});
}

template<typename Level, typename Cursor>
inline std::uint64_t get_flat_group_checksum(Level l, Cursor& c)
{
    auto res = get_level_fields_checksum(l, c);
    for(const auto entry : l.flat_group(c).cursor_range(c))
    {
        res += get_level_fields_checksum(entry, c);
    }

    return res;
}

template<typename Level, typename Cursor>
inline std::uint64_t get_nested_group_checksum(Level l, Cursor& c)
{
    auto res = get_flat_group_checksum(l, c);
    for(const auto entry : l.nested_group(c).cursor_range(c))
    {
        res += get_level_fields_checksum(entry, c);
        res += get_data_checksum(entry.data(c));
    }

    return res;
}

template<typename Level, typename Cursor>
inline std::uint64_t get_nested_group2_checksum(Level l, Cursor& c)
{
    auto res = get_nested_group_checksum(l, c);
    for(const auto entry : l.nested_group2(c).cursor_range(c))
    {
        res += get_level_fields_checksum(entry, c);
        for(const auto entry2 : entry.nested_group(c).cursor_range(c))
        {
            res += get_level_fields_checksum(entry2, c);
            res += get_data_checksum(entry2.data(c));
        }
    }

    return res;
}

template<typename Level, typename Cursor>
inline std::uint64_t get_whole_message_checksum(Level l, Cursor& c)
{
    auto res = get_nested_group2_checksum(l, c);
    res += get_data_checksum(l.data(c));

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
            auto c = sbepp::init_cursor(msg);
            const auto checksum = get_level_fields_checksum(msg, c);
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
            auto c = sbepp::init_cursor(msg);
            const auto checksum = get_flat_group_checksum(msg, c);
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
            auto c = sbepp::init_cursor(msg);
            const auto checksum = get_nested_group_checksum(msg, c);
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
            auto c = sbepp::init_cursor(msg);
            const auto checksum = get_nested_group2_checksum(msg, c);
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
            auto c = sbepp::init_cursor(msg);
            const auto checksum = get_whole_message_checksum(msg, c);
            assert(checksum == test.data_checksum);
            sum += checksum;
        }
        ::benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(sbepp_cursor_reader::top_level_fields_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(sbepp_cursor_reader::flat_group_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(sbepp_cursor_reader::nested_group_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(sbepp_cursor_reader::nested_group2_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(sbepp_cursor_reader::whole_message_benchmark)
    ->Apply(config::configure_benchmark);
} // namespace sbepp_cursor_reader
} // namespace benchmark
} // namespace sbepp