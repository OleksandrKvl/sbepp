// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#include <sbepp/benchmark/test_data.hpp>
#include <sbepp/benchmark/message_generator.hpp>
#include <sbepp/benchmark/config.hpp>

#define SBE_NO_BOUNDS_CHECK
#include <sbepp/benchmark/real_logic/benchmark_schema/Msg1.h>
#include <sbepp/benchmark/real_logic/benchmark_schema/MessageHeader.h>

#include <benchmark/benchmark.h>

#include <cstdint>
#include <numeric>
#include <cassert>

namespace sbepp
{
namespace benchmark
{
namespace real_logic_reader
{
inline benchmark_schema::Msg1 init_message(const buffer_type& buf)
{
    benchmark_schema::MessageHeader header;
    // weird API requires non-const `char*` even for decoder
    auto casted_ptr =
        reinterpret_cast<char*>(const_cast<byte_type*>(buf.data()));
    header.wrap(casted_ptr, 0, 0, buf.size());

    const auto block_length = header.blockLength();
    const auto template_id = header.templateId();
    const auto schema_id = header.schemaId();
    const auto version = header.version();

    benchmark_schema::Msg1 msg;
    msg.wrapForDecode(
        casted_ptr, header.encodedLength(), block_length, version, buf.size());

    return msg;
}

template<typename Level>
inline std::uint64_t get_level_fields_checksum(const Level& l)
{
    std::uint64_t res{};
    res += l.field1();
    res += l.field2();
    res += l.field3();
    res += l.field4();
    res += l.field5();

    return res;
}

inline std::uint64_t get_data_checksum(const std::string_view d)
{
    return std::accumulate(
        std::begin(d),
        std::end(d),
        std::uint64_t{},
        [](auto acc, auto ch)
        {
            return acc + static_cast<std::uint8_t>(ch);
        });
}

inline std::uint64_t get_top_level_checksum(const buffer_type& buf)
{
    auto msg = init_message(buf);
    return get_level_fields_checksum(msg);
}

template<typename Level>
inline std::uint64_t get_flat_group_checksum_impl(Level& l)
{
    std::uint64_t res{};
    auto& g = l.flat_group();
    while(g.hasNext())
    {
        g.next();
        res += get_level_fields_checksum(g);
    }

    return res;
}

inline std::uint64_t get_flat_group_checksum(const buffer_type& buf)
{
    auto msg = init_message(buf);
    auto res = get_level_fields_checksum(msg);
    res += get_flat_group_checksum_impl(msg);

    return res;
}

template<typename Level>
inline std::uint64_t get_nested_group_checksum_impl(Level& l)
{
    std::uint64_t res{};
    auto& g = l.nested_group();
    while(g.hasNext())
    {
        g.next();
        res += get_level_fields_checksum(g);
        res += get_data_checksum(g.getDataAsStringView());
    }

    return res;
}

inline std::uint64_t get_nested_group_checksum(const buffer_type& buf)
{
    auto msg = init_message(buf);
    auto res = get_level_fields_checksum(msg);
    res += get_flat_group_checksum_impl(msg);
    res += get_nested_group_checksum_impl(msg);

    return res;
}

inline std::uint64_t
    get_nested_group2_checksum_impl(benchmark_schema::Msg1& msg)
{
    std::uint64_t res{};
    auto g = msg.nested_group2();
    while(g.hasNext())
    {
        g.next();
        res += get_level_fields_checksum(g);
        res += get_nested_group_checksum_impl(g);
    }

    return res;
}

inline std::uint64_t get_nested_group2_checksum(const buffer_type& buf)
{
    auto msg = init_message(buf);
    auto res = get_level_fields_checksum(msg);
    res += get_flat_group_checksum_impl(msg);
    res += get_nested_group_checksum_impl(msg);
    res += get_nested_group2_checksum_impl(msg);

    return res;
}

inline std::uint64_t get_whole_message_checksum(const buffer_type& buf)
{
    auto msg = init_message(buf);
    auto res = get_level_fields_checksum(msg);
    res += get_flat_group_checksum_impl(msg);
    res += get_nested_group_checksum_impl(msg);
    res += get_nested_group2_checksum_impl(msg);
    res += get_data_checksum(msg.getDataAsStringView());

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
            const auto checksum = get_top_level_checksum(test.buffer);
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
            const auto checksum = get_flat_group_checksum(test.buffer);
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
            const auto checksum = get_nested_group_checksum(test.buffer);
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
            const auto checksum = get_nested_group2_checksum(test.buffer);
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
            const auto checksum = get_whole_message_checksum(test.buffer);
            assert(checksum == test.data_checksum);
            sum += checksum;
        }
        ::benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(real_logic_reader::top_level_fields_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(real_logic_reader::flat_group_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(real_logic_reader::nested_group_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(real_logic_reader::nested_group2_benchmark)
    ->Apply(config::configure_benchmark);
BENCHMARK(real_logic_reader::whole_message_benchmark)
    ->Apply(config::configure_benchmark);
} // namespace real_logic_reader
} // namespace benchmark
} // namespace sbepp
