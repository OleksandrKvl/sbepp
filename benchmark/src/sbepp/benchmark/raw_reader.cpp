// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

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
namespace raw_reader
{
inline std::uint16_t get_message_block_length(const byte_type* header)
{
    return *(reinterpret_cast<const std::uint16_t*>(header));
}

inline std::uint16_t get_group_block_length(const byte_type* header)
{
    return *(reinterpret_cast<const std::uint16_t*>(header));
}

inline std::uint16_t get_num_in_group(const byte_type* header)
{
    return *(
        reinterpret_cast<const std::uint16_t*>(header + sizeof(std::uint16_t)));
}

constexpr std::size_t get_message_header_size()
{
    return sizeof(std::uint16_t) * 4;
}

constexpr std::size_t get_group_header_size()
{
    return sizeof(std::uint16_t) * 2;
}

constexpr std::size_t get_data_length_size()
{
    return sizeof(std::uint32_t);
}

inline std::uint64_t get_fields_checksum(const byte_type* block)
{
    std::uint64_t res{};

    res += *(reinterpret_cast<const std::uint32_t*>(block));
    res += *(
        reinterpret_cast<const std::uint32_t*>(block + sizeof(std::uint32_t)));
    res += *(reinterpret_cast<const std::uint32_t*>(
        block + sizeof(std::uint32_t) * 2));
    res += *(reinterpret_cast<const std::uint32_t*>(
        block + sizeof(std::uint32_t) * 3));
    res += *(reinterpret_cast<const std::uint32_t*>(
        block + sizeof(std::uint32_t) * 4));

    return res;
}

inline std::uint32_t get_data_length(const byte_type* data)
{
    return *(reinterpret_cast<const std::uint32_t*>(data));
}

inline std::uint64_t get_data_checksum(const byte_type* data)
{
    const auto length = get_data_length(data);
    data += get_data_length_size();

    return std::accumulate(data, data + length, std::uint64_t{});
}

inline std::uint64_t get_top_level_checksum(const byte_type*& msg)
{
    const auto block_length = get_message_block_length(msg);
    msg += get_message_header_size();
    const auto res = get_fields_checksum(msg);
    msg += block_length;
    return res;
}

inline std::uint64_t get_flat_group_checksum(const byte_type*& msg)
{
    auto res = get_top_level_checksum(msg);
    auto block_length = get_group_block_length(msg);
    auto num_in_group = get_num_in_group(msg);
    msg += get_group_header_size();

    for(std::size_t i = 0; i != num_in_group; i++)
    {
        res += get_fields_checksum(msg);
        msg += block_length;
    }

    return res;
}

inline std::uint64_t get_nested_group_checksum(const byte_type*& msg)
{
    auto res = get_flat_group_checksum(msg);
    auto block_length = get_group_block_length(msg);
    auto num_in_group = get_num_in_group(msg);
    msg += get_group_header_size();

    for(std::size_t i = 0; i != num_in_group; i++)
    {
        res += get_fields_checksum(msg);
        msg += block_length;
        res += get_data_checksum(msg);
        msg += get_data_length(msg) + get_data_length_size();
    }

    return res;
}

inline std::uint64_t get_nested_group2_checksum(const byte_type*& msg)
{
    auto res = get_nested_group_checksum(msg);
    auto block_length = get_group_block_length(msg);
    auto num_in_group = get_num_in_group(msg);
    msg += get_group_header_size();

    for(std::size_t i = 0; i != num_in_group; i++)
    {
        res += get_fields_checksum(msg);
        msg += block_length;

        auto block_length = get_group_block_length(msg);
        auto num_in_group = get_num_in_group(msg);
        msg += get_group_header_size();

        for(std::size_t i = 0; i != num_in_group; i++)
        {
            res += get_fields_checksum(msg);
            msg += block_length;
            res += get_data_checksum(msg);
            msg += get_data_length(msg) + get_data_length_size();
        }
    }

    return res;
}

inline std::uint64_t get_whole_message_checksum(const byte_type*& msg)
{
    auto res = get_nested_group2_checksum(msg);

    res += get_data_checksum(msg);
    msg += get_data_length(msg) + get_data_length_size();

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
            auto msg = test.buffer.data();
            const auto checksum = get_top_level_checksum(msg);
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
            auto msg = test.buffer.data();
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
            auto msg = test.buffer.data();
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
            auto msg = test.buffer.data();
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
            auto msg = test.buffer.data();
            const auto checksum = get_whole_message_checksum(msg);
            assert(checksum == test.data_checksum);
            sum += checksum;
        }
        ::benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(raw_reader::top_level_fields_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(raw_reader::flat_group_benchmark)->Apply(config::configure_benchmark);
BENCHMARK(raw_reader::nested_group_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(raw_reader::nested_group2_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(raw_reader::whole_message_benchmark)
    ->Apply(config::configure_benchmark);
} // namespace raw_reader
} // namespace benchmark
} // namespace sbepp