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
#include "gpu_test_bench.hpp"

class reduce_test_bench : public vren_test::gpu_test_bench
{
public:
    uint32_t run_gpu_reduce(uint32_t stride, uint32_t offset, uint64_t* elapsed_time)
    {
        uint32_t result = 0;

        VkBufferMemoryBarrier buffer_memory_barrier{};

        vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
        {
            buffer_memory_barrier = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                .buffer = m_gpu_buffer->m_buffer.m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

            auto sample = [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
            {
                VREN_TEST_APP()->m_reduce(command_buffer, resource_container, *m_gpu_buffer, m_length, stride, offset, &result);
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
                .buffer = m_gpu_buffer->m_buffer.m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_HOST_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
        });

        if (elapsed_time)
        {
            *elapsed_time = VREN_TEST_APP()->m_profiler.read_elapsed_time(0);
        }

        return result;
    }

    uint32_t run_std_reduce(uint32_t stride, uint32_t offset)
    {
        uint32_t* data = m_reference_buffer.data();

        for (uint32_t i = 0; i < glm::log2<uint32_t>(m_length); i++)
        {
            //printf("-> i: %d\n", i);

            for (uint32_t j = 0; j < (m_length >> (i + 1)); j++)
            {
                uint32_t a = offset + (1 << i) - 1 + (j << (i + 1)) * stride;
                uint32_t b = offset + (1 << i) - 1 + (j << (i + 1)) * stride + (stride << i);

                //printf("a: %d, b: %d\n", a, b);

                data[b] = data[a] + data[b];
            }
        }

        return data[m_length - 1];
    }
};

// ------------------------------------------------------------------------------------------------
// Benchmarking
// ------------------------------------------------------------------------------------------------

static void BM_gpu_reduce(benchmark::State& state)
{
    reduce_test_bench m_test_bench{};

    for (auto _ : state)
    {
        m_test_bench.set_random_data(state.range(0));

        uint64_t elapsed_time;
        m_test_bench.run_gpu_reduce(1, 0, &elapsed_time);
        state.SetIterationTime(elapsed_time / (double)1e9);
    }
}

BENCHMARK(BM_gpu_reduce)
    ->Unit(benchmark::kMicrosecond)
    ->Range(1 << 16, 1 << 28)
    ->UseManualTime();

static void BM_std_reduce(benchmark::State& state)
{
    reduce_test_bench m_test_bench{};

    for (auto _ : state)
    {
        state.PauseTiming();
        m_test_bench.set_random_data(state.range(0));
        state.ResumeTiming();

        m_test_bench.run_std_reduce(1, 0);
    }
}

BENCHMARK(BM_std_reduce)
    ->Unit(benchmark::kMicrosecond)
    ->Range(1 << 16, 1 << 28);

// ------------------------------------------------------------------------------------------------
// Unit testing
// ------------------------------------------------------------------------------------------------

TEST(reduce, main)
{
    const size_t k_data_length = vren::reduce::k_workgroup_size * vren::reduce::k_num_iterations;

    reduce_test_bench test_bench{};

    test_bench.set_fixed_data(k_data_length, 1);

    printf("Reference buffer:\n"); test_bench.print_reference_buffer();
    printf("GPU buffer:\n"); test_bench.print_host_buffer();

    test_bench.run_gpu_reduce(1, 0, nullptr);
    test_bench.run_std_reduce(1, 0);

    test_bench.copy_gpu_buffer_to_host_buffer();

    printf("Reduced reference buffer:\n"); test_bench.print_reference_buffer();
    printf("Reduced GPU buffer:\n"); test_bench.print_host_buffer();

    test_bench.assert_eq();
}
