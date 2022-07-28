

#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

#include <numeric>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/integer.hpp>
#include <fmt/format.h>

#include <vren/context.hpp>
#include <vren/toolbox.hpp>
#include <vren/primitives/blelloch_scan.hpp>
#include <vren/vk_helpers/misc.hpp>
#include <vren/pipeline/profiler.hpp>

#include "app.hpp"
#include "gpu_test_bench.hpp"

// ------------------------------------------------------------------------------------------------
// Benchmark
// ------------------------------------------------------------------------------------------------

static void BM_gpu_bucket_sort(benchmark::State& state)
{
    vren::bucket_sort& bucket_sort = VREN_TEST_APP()->m_context.m_toolbox->m_bucket_sort;

    size_t length = state.range(0);

    vren::vk_utils::buffer input_buffer = vren::vk_utils::alloc_device_only_buffer(
        VREN_TEST_APP()->m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        length * sizeof(uint16_t)
    );

    vren::vk_utils::buffer output_buffer = vren::vk_utils::alloc_device_only_buffer(
        VREN_TEST_APP()->m_context,
        vren::bucket_sort::get_required_output_buffer_usage_flags(),
        vren::bucket_sort::get_required_output_buffer_size(length)
    );

    for (auto _ : state)
    {
        vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
        {
            VREN_TEST_APP()->m_profiler.profile(command_buffer, resource_container, 0, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
            {
                bucket_sort(command_buffer, resource_container, input_buffer, length, 0, output_buffer, 0);
            });
        });

        uint32_t elapsed_time = VREN_TEST_APP()->m_profiler.read_elapsed_time(0);

        state.SetIterationTime(elapsed_time / (double)1e9);
    }
}

BENCHMARK(BM_gpu_bucket_sort)
    ->Unit(benchmark::kMicrosecond)
    ->Range(1 << 10, 1 << 29 /* < maxStorageBufferRange */)
    ->Iterations(1)
    ->UseManualTime();

// ------------------------------------------------------------------------------------------------
// Unit testing
// ------------------------------------------------------------------------------------------------

void run_bucket_sort_test(uint32_t sample_length, bool verbose)
{
    vren::bucket_sort& bucket_sort = VREN_TEST_APP()->m_context.m_toolbox->m_bucket_sort;

    std::vector<glm::uvec2> cpu_buffer(sample_length);

    vren::vk_utils::buffer gpu_input_buffer = vren::vk_utils::alloc_host_only_buffer(
        VREN_TEST_APP()->m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        sample_length * sizeof(glm::uvec2),
        true
    );

    vren::vk_utils::buffer gpu_output_buffer = vren::vk_utils::alloc_host_only_buffer(
        VREN_TEST_APP()->m_context,
        vren::bucket_sort::get_required_output_buffer_usage_flags(),
        vren::bucket_sort::get_required_output_buffer_size(sample_length),
        true
    );

    // Initialization
    for (uint32_t i = 0; i < sample_length; i++)
    {
        cpu_buffer[i].x = sample_length - i;
        cpu_buffer[i].y = i; // This can't be checked as for the same key it'll be unsorted
    }

    std::memcpy(gpu_input_buffer.m_allocation_info.pMappedData, cpu_buffer.data(), sample_length * sizeof(glm::uvec2));

    // Sort CPU array
    std::sort(cpu_buffer.begin(), cpu_buffer.end(), [&](glm::uvec2 const& a, glm::uvec2 const& b) {
        return (a.x & vren::bucket_sort::k_key_mask) < (b.x & vren::bucket_sort::k_key_mask);
    });

    // Sort GPU array
    vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
    {
        VkBufferMemoryBarrier buffer_memory_barrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .buffer = gpu_input_buffer.m_buffer.m_handle,
            .offset = 0,
            .size = sample_length * sizeof(glm::uvec2)
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        bucket_sort(
            command_buffer,
            resource_container,
            gpu_input_buffer,
            sample_length,
            0,
            gpu_output_buffer,
            0
        );
    });

    glm::uvec2* gpu_output_buffer_ptr = reinterpret_cast<glm::uvec2*>(gpu_output_buffer.m_allocation_info.pMappedData);

    if (verbose)
    {
        // CPU buffer
        fmt::print("CPU buffer:\n");
        for (uint32_t i = 0; i < sample_length; i++)
        {
            fmt::print("{}, ", cpu_buffer.at(i).x);
        }
        fmt::print("\n");

        // GPU buffer
        fmt::print("GPU buffer:\n");
        for (uint32_t i = 0; i < sample_length; i++)
        {
            fmt::print("{}, ", gpu_output_buffer_ptr[i].x);
        }
        fmt::print("\n");
    }

    for (uint32_t i = 0; i < sample_length; i++)
    {
        ASSERT_EQ(cpu_buffer.at(i).x & vren::bucket_sort::k_key_mask, gpu_output_buffer_ptr[i].x & vren::bucket_sort::k_key_mask);
    }
}

TEST(bucket_sort, main)
{
    /*
    run_bucket_sort_test(1 << 18, false);
    run_bucket_sort_test(1 << 19, false);
    run_bucket_sort_test(1 << 20, false);
    run_bucket_sort_test(1 << 21, false);
    */
    uint32_t length = 1 << 10;
    while (length <= (1 << 20))
    {
        fmt::print("[bucket_sort] Length: {}\n", length);

        run_bucket_sort_test(length, false);
        length <<= 1;
    }
}
