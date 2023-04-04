// SPDX-License-Identifier: MIT
// Copyright (c) 2023, Oleksandr Koval

#pragma once

#include <benchmark_schema/benchmark_schema.hpp>
#include <sbepp/benchmark/test_data.hpp>

#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

namespace sbepp
{
namespace benchmark
{
class message_generator
{
public:
    message_generator(
        std::size_t min_group_size,
        std::size_t max_group_size,
        std::size_t min_data_size,
        std::size_t max_data_size)
        : min_group_size{min_group_size},
          max_group_size{max_group_size},
          min_data_size{min_data_size},
          max_data_size{max_data_size}
    {
        if((max_group_size < min_group_size) || (max_data_size < min_data_size))
        {
            throw std::runtime_error{
                "Max group/data size cannot be less than min size"};
        }
    }

    std::vector<test_data> generate(const std::size_t n)
    {
        std::vector<test_data> res;
        res.reserve(n);

        std::generate_n(
            std::back_inserter(res),
            n,
            [this]()
            {
                test_data data{};
                benchmark_schema::messages::msg1<byte_type> msg{
                    data.buffer.data(), data.buffer.size()};
                sbepp::fill_message_header(msg);
                data.top_level_checksum = fill_level_fields(msg);
                data.flat_group_checksum =
                    data.top_level_checksum
                    + fill_group(
                        msg.flat_group(),
                        [this](auto entry)
                        {
                            return fill_level_fields(entry);
                        });
                data.nested_group_checksum =
                    data.flat_group_checksum
                    + fill_nested_group(msg.nested_group());
                data.nested_group2_checksum =
                    data.nested_group_checksum
                    + fill_group(
                        msg.nested_group2(),
                        [this](auto entry)
                        {
                            return fill_level_fields(entry)
                                   + fill_nested_group(entry.nested_group());
                        });
                data.data_checksum =
                    data.nested_group2_checksum + fill_data_field(msg.data());

                if(sbepp::size_bytes(msg) > data.buffer.size())
                {
                    throw std::runtime_error{
                        "Message buffer is not big enough"};
                }

                return data;
            });

        return res;
    }

private:
    std::mt19937 mt{std::random_device{}()};
    std::uniform_int_distribution<std::uint32_t> dist;
    std::size_t min_group_size{};
    std::size_t max_group_size{};
    std::size_t min_data_size{};
    std::size_t max_data_size{};

    std::uint32_t get_random_value()
    {
        return dist(mt);
    }

    std::size_t get_random_group_size()
    {
        return min_group_size
               + (get_random_value() % (1 + max_group_size - min_group_size));
    }

    std::size_t get_random_data_size()
    {
        return min_data_size
               + (get_random_value() % (1 + max_data_size - min_data_size));
    }

    byte_type get_random_byte()
    {
        return get_random_value() % std::numeric_limits<byte_type>::max();
    }

    template<typename Group>
    std::uint64_t fill_nested_group(Group g)
    {
        return fill_group(
            g,
            [this](auto entry)
            {
                return fill_level_fields(entry) + fill_data_field(entry.data());
            });
    }

    template<typename Data>
    std::uint64_t fill_data_field(Data d)
    {
        d.resize(get_random_data_size());
        std::uint64_t checksum{};

        for(auto& byte : d)
        {
            byte = get_random_byte();
            checksum += byte;
        }

        return checksum;
    }

    template<typename Group, typename EntryFiller>
    std::uint64_t fill_group(Group g, EntryFiller filler)
    {
        sbepp::fill_group_header(g, get_random_group_size());
        std::uint64_t checksum{};

        for(const auto entry : g)
        {
            checksum += filler(entry);
        }

        return checksum;
    }

    template<typename Level>
    std::uint64_t fill_level_fields(Level l)
    {
        std::uint64_t res{};
        l.field1(get_random_value());
        res += *l.field1();
        l.field2(get_random_value());
        res += *l.field2();
        l.field3(get_random_value());
        res += *l.field3();
        l.field4(get_random_value());
        res += *l.field4();
        l.field5(get_random_value());
        res += *l.field5();

        return res;
    }
};
} // namespace benchmark
} // namespace sbepp