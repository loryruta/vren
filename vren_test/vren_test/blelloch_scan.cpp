#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

#include <numeric>
#include <memory>

#include <glm/gtc/integer.hpp>
#include <fmt/format.h>

#include <vren/context.hpp>
#include <vren/primitive/blelloch_scan.hpp>
#include <vren/vk_helpers/misc.hpp>
#include <vren/pipeline/profiler.hpp>

#include "app.hpp"

class blelloch_scan_context
{
public:
    vren::blelloch_scan m_blelloch_scan;
    size_t m_length = 0;
    std::vector<uint32_t> m_reference;
    vren::vk_utils::buffer m_staging_buffer, m_buffer;

    blelloch_scan_context(size_t length) :
        m_blelloch_scan(VREN_TEST_APP()->m_context),
        m_length(length),
        m_reference(m_length),
        m_staging_buffer([&]() {
            return vren::vk_utils::alloc_buffer(
                VREN_TEST_APP()->m_context,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                m_length * sizeof(uint32_t),
                VMA_ALLOCATION_CREATE_MAPPED_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            );
        }()),
        m_buffer([&]() {
            return vren::vk_utils::alloc_buffer(
                VREN_TEST_APP()->m_context,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                m_length * sizeof(uint32_t),
                NULL,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
        }())
    {
    }

    void fill_with_random_data()
    {
        std::srand(std::time(nullptr));

        for (uint32_t i = 0; i < m_length; i++)
        {
            m_reference[i] = std::rand() % 10;
        }

        std::memcpy(m_staging_buffer.m_allocation_info.pMappedData, m_reference.data(), m_length * sizeof(uint32_t));

        vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
        {
            VkBufferCopy buffer_copy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = m_length * sizeof(uint32_t)
            };
            vkCmdCopyBuffer(command_buffer, m_staging_buffer.m_buffer.m_handle, m_buffer.m_buffer.m_handle, 1, &buffer_copy);
        });
    }

    void run_parallel_blelloch_scan(uint64_t* elapsed_time = nullptr)
    {
        VkBufferMemoryBarrier buffer_memory_barrier{};

        vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
        {
            buffer_memory_barrier = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                .buffer = m_buffer.m_buffer.m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

            auto sample = [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
            {
                m_blelloch_scan(command_buffer, resource_container, m_buffer, m_length, 1, 0);
            };

            if (elapsed_time)
            {
                VREN_TEST_APP()->m_profiler.profile(command_buffer, resource_container, 0, sample);
            }
            else
            {
                sample(command_buffer, resource_container);
            }

            buffer_memory_barrier = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT,
                .buffer = m_buffer.m_buffer.m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_HOST_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
        });

        if (elapsed_time)
        {
            *elapsed_time = VREN_TEST_APP()->m_profiler.read_elapsed_time(0);
        }
    }

    void run_std_exclusive_scan()
    {
        std::exclusive_scan(m_reference.begin(), m_reference.end(), m_reference.begin(), 0);
    }

    void assert_eq()
    {
        vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
        {
            VkBufferCopy buffer_copy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = m_length * sizeof(uint32_t)
            };
            vkCmdCopyBuffer(command_buffer, m_buffer.m_buffer.m_handle, m_staging_buffer.m_buffer.m_handle, 1, &buffer_copy);
        });

        for (uint32_t i = 0; i < m_length; i++)
        {
            uint32_t v = reinterpret_cast<uint32_t*>(m_staging_buffer.m_allocation_info.pMappedData)[i];
            uint32_t r = m_reference[i];

            ASSERT_EQ(v, r);
        }
    }
};

// ------------------------------------------------------------------------------------------------
// Benchmarking
// ------------------------------------------------------------------------------------------------

class blelloch_scan_benchmark_fixture : public benchmark::Fixture
{
public:
    std::unique_ptr<blelloch_scan_context> m_context;

    void SetUp(benchmark::State const& state)
    {
        m_context = std::make_unique<blelloch_scan_context>(state.range(0));
    }

    void TearDown(benchmark::State const& state)
    {
        m_context.reset();
    }
};

BENCHMARK_DEFINE_F(blelloch_scan_benchmark_fixture, parallel_blelloch_scan)(benchmark::State& state)
{
    for (auto _ : state)
    {
        m_context->fill_with_random_data();

        uint64_t elapsed_time;
        m_context->run_parallel_blelloch_scan(&elapsed_time);
        state.SetIterationTime(elapsed_time / (double) 1e9);
    }
}

BENCHMARK_DEFINE_F(blelloch_scan_benchmark_fixture, std_exclusive_scan)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        m_context->fill_with_random_data();
        state.ResumeTiming();

        m_context->run_std_exclusive_scan();
    }
}

BENCHMARK_REGISTER_F(blelloch_scan_benchmark_fixture, parallel_blelloch_scan)
    ->Unit(benchmark::kMicrosecond)
    ->Range(1 << 16, 1 << 28)
    ->UseManualTime();

BENCHMARK_REGISTER_F(blelloch_scan_benchmark_fixture, std_exclusive_scan)
    ->Unit(benchmark::kMicrosecond)
    ->Range(1 << 16, 1 << 28);

// ------------------------------------------------------------------------------------------------
// Unit testing
// ------------------------------------------------------------------------------------------------

TEST(blelloch_scan_context, main)
{
    blelloch_scan_context test(1 << 10);

    test.fill_with_random_data();

    test.run_parallel_blelloch_scan();
    test.run_std_exclusive_scan();

    test.assert_eq();
}
