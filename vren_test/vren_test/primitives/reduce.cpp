#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

#include <numeric>
#include <memory>

#include <glm/gtc/integer.hpp>
#include <fmt/format.h>

#include <vren/context.hpp>
#include <vren/primitives/blelloch_scan.hpp>
#include <vren/vk_helpers/misc.hpp>
#include <vren/pipeline/profiler.hpp>

#include "app.hpp"
#include "gpu_test_bench.hpp"

// ------------------------------------------------------------------------------------------------
// Benchmarking
// ------------------------------------------------------------------------------------------------

static void BM_gpu_reduce(benchmark::State& state)
{
    size_t length = state.range(0);

    vren::vk_utils::buffer buffer = vren::vk_utils::alloc_device_only_buffer(
        VREN_TEST_APP()->m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        length * sizeof(uint32_t)
    );

    for (auto _ : state)
    {
        vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
        {
            vkCmdFillBuffer(command_buffer, buffer.m_buffer.m_handle, 0, length * sizeof(uint32_t), 1);

            VkBufferMemoryBarrier buffer_memory_barrier{
               .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
               .pNext = nullptr,
               .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
               .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
               .buffer = buffer.m_buffer.m_handle,
               .offset = 0,
               .size = length * sizeof(uint32_t)
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

            VREN_TEST_APP()->m_profiler.profile(command_buffer, resource_container, 0, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
            {
                VREN_TEST_APP()->m_reduce(command_buffer, resource_container, buffer, length, 0, 1, nullptr);
            });
        });

        uint32_t elapsed_time = VREN_TEST_APP()->m_profiler.read_elapsed_time(0);

        state.SetIterationTime(elapsed_time / (double) 1e9);
    }
}

BENCHMARK(BM_gpu_reduce)
    ->Unit(benchmark::kMicrosecond)
    ->Range(1 << 10, 1 << 29 /* < maxStorageBufferRange */)
    ->Iterations(1)
    ->UseManualTime();

// ------------------------------------------------------------------------------------------------
// Unit testing
// ------------------------------------------------------------------------------------------------

void run_cpu_reduce(uint32_t* data, size_t length, size_t stride, size_t offset)
{
    for (uint32_t i = 0; i < glm::log2<uint32_t>(length); i++)
    {
        for (uint32_t j = 0; j < (length >> (i + 1)); j++)
        {
            uint32_t a = offset + (1 << i) - 1 + (j << (i + 1)) * stride;
            uint32_t b = offset + (1 << i) - 1 + (j << (i + 1)) * stride + (stride << i);

            data[b] = data[a] + data[b];
        }
    }
}

void run_reduce_test(uint32_t sample_length, bool verbose)
{
    std::vector<uint32_t> cpu_buffer(sample_length);
    vren::vk_utils::buffer gpu_buffer =
        vren::vk_utils::alloc_host_visible_buffer(VREN_TEST_APP()->m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sample_length * sizeof(uint32_t), true);

    uint32_t* gpu_buffer_ptr = reinterpret_cast<uint32_t*>(gpu_buffer.m_allocation_info.pMappedData);

    std::fill(cpu_buffer.begin(), cpu_buffer.end(), 1);

    std::memcpy(gpu_buffer_ptr, cpu_buffer.data(), sample_length * sizeof(uint32_t));

    if (verbose)
    {
        /*
        fmt::print("Before reduction:\n");
        fmt::print("CPU buffer:\n"); vren_test::print_buffer<uint32_t>(cpu_buffer.data(), sample_length);
        fmt::print("GPU buffer:\n"); vren_test::print_buffer<uint32_t>(gpu_buffer_ptr, sample_length);
        */
    }

    run_cpu_reduce(cpu_buffer.data(), sample_length, 1, 0);

    vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
    {
        VkBufferMemoryBarrier buffer_memory_barrier{
           .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
           .pNext = nullptr,
           .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
           .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
           .buffer = gpu_buffer.m_buffer.m_handle,
           .offset = 0,
           .size = sample_length * sizeof(uint32_t)
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        VREN_TEST_APP()->m_reduce(command_buffer, resource_container, gpu_buffer, sample_length, 0, 1, nullptr);
    });

    if (verbose)
    {
        fmt::print("After reduction:\n");
        //fmt::print("CPU buffer:\n"); vren_test::print_buffer<uint32_t>(cpu_buffer.data(), sample_length);
        fmt::print("GPU buffer:\n"); vren_test::print_buffer<uint32_t>(gpu_buffer_ptr, sample_length);
    }

    ASSERT_EQ(cpu_buffer.at(sample_length - 1), gpu_buffer_ptr[sample_length - 1]);

    for (uint32_t i = 0; i < sample_length; i++)
    {
        ASSERT_EQ(cpu_buffer.at(i), gpu_buffer_ptr[i]);
    }
}

TEST(reduce, main)
{
    //run_reduce_test(1 << 6, true);

    for (uint32_t length = 1; length >> 20 == 0; length <<= 1)
    {
        fmt::print("[reduce] Length: {}\n", length);

        run_reduce_test(length, false);
    }
}
