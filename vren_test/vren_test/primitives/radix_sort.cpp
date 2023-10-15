

#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

#include <numeric>
#include <memory>

#include <glm/gtc/integer.hpp>
#include <fmt/format.h>

#include "vren/vk_api/misc_utils.hpp"
#include <vren/Context.hpp>
#include <vren/Toolbox.hpp>
#include <vren/pipeline/profiler.hpp>
#include <vren/primitives/BlellochScan.hpp>

#include "app.hpp"
#include "gpu_test_bench.hpp"

// ------------------------------------------------------------------------------------------------
// Benchmark
// ------------------------------------------------------------------------------------------------

static void BM_gpu_radix_sort(benchmark::State& state)
{
    vren::radix_sort& radix_sort = VREN_TEST_APP()->m_context.m_toolbox->m_radix_sort;

    size_t length = state.range(0);

    vren::vk_utils::buffer buffer = vren::vk_utils::alloc_device_only_buffer(
        VREN_TEST_APP()->m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        length * sizeof(uint32_t)
    );

    // The radix-sort algorithm is not data-dependant! So we don't need to initialize it to obtain realistic results

    vren::vk_utils::buffer scratch_buffer_1 = radix_sort.create_scratch_buffer_1(length);
    vren::vk_utils::buffer scratch_buffer_2 = radix_sort.create_scratch_buffer_2(length);

    for (auto _ : state)
    {
        vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
        {
            VREN_TEST_APP()->m_profiler.profile(command_buffer, resource_container, 0, [&](VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
            {
                radix_sort(command_buffer, resource_container, buffer, length, scratch_buffer_1, scratch_buffer_2);
            });
        });

        uint32_t elapsed_time = VREN_TEST_APP()->m_profiler.read_elapsed_time(0);

        state.SetIterationTime(elapsed_time / (double) 1e9);
    }
}

BENCHMARK(BM_gpu_radix_sort)
    ->Unit(benchmark::kMicrosecond)
    ->Range(1 << 10, 1 << 29 /* < maxStorageBufferRange */)
    ->Iterations(1)
    ->UseManualTime();

// ------------------------------------------------------------------------------------------------
// Unit testing
// ------------------------------------------------------------------------------------------------

void run_radix_sort_test(uint32_t sample_length, bool verbose)
{
    vren::radix_sort& radix_sort = VREN_TEST_APP()->m_context.m_toolbox->m_radix_sort;

    std::vector<uint32_t> cpu_buffer(sample_length);
    
    vren::vk_utils::buffer gpu_buffer =
        vren::vk_utils::alloc_host_visible_buffer(VREN_TEST_APP()->m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sample_length * sizeof(uint32_t), true);

    vren::vk_utils::buffer scratch_buffer_1 = radix_sort.create_scratch_buffer_1(sample_length);
    vren::vk_utils::buffer scratch_buffer_2 = radix_sort.create_scratch_buffer_2(sample_length);

    uint32_t* gpu_buffer_ptr = reinterpret_cast<uint32_t*>(gpu_buffer.m_allocation_info.pMappedData);

    std::iota(cpu_buffer.begin(), cpu_buffer.end(), 0);
    std::reverse(cpu_buffer.begin(), cpu_buffer.end());
    //vren_test::fill_with_random_int_values(cpu_buffer.begin(), cpu_buffer.end(), 0, 100);

    std::memcpy(gpu_buffer_ptr, cpu_buffer.data(), sample_length * sizeof(uint32_t));

    std::sort(cpu_buffer.begin(), cpu_buffer.end());

    vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
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

        radix_sort(
            command_buffer,
            resource_container,
            gpu_buffer,
            sample_length,
            scratch_buffer_1,
            scratch_buffer_2
        );
    });

    if (verbose)
    {
        //fmt::print("Scratch buffer 1:\n"); vren_test::print_gpu_buffer<uint32_t>(VREN_TEST_APP()->m_context, scratch_buffer_1, (1 * 16 + 16));
        //fmt::print("Scratch buffer 2:\n"); vren_test::print_gpu_buffer<uint32_t>(VREN_TEST_APP()->m_context, scratch_buffer_2, sample_length,"{:08x}");

        //fmt::print("GPU buffer:\n"); vren_test::print_buffer<uint32_t>(gpu_buffer_ptr, sample_length, "{:08x}");
        //fmt::print("CPU buffer:\n"); vren_test::print_buffer<uint32_t>(cpu_buffer.data(), sample_length, "{:08x}");
    }

    ASSERT_EQ(cpu_buffer.at(sample_length - 1), gpu_buffer_ptr[sample_length - 1]);

    for (uint32_t i = 0; i < sample_length; i++)
    {
        ASSERT_EQ(cpu_buffer.at(i), gpu_buffer_ptr[i]);
    }
}

TEST(radix_sort, main)
{
    run_radix_sort_test(1 << 10, true);

    /*
    uint32_t length = 1 << 10;
    while (length <= (1 << 20))
    {
        fmt::print("[radix_sort] Length: {}\n", length);

        run_radix_sort_test(length, false);
        length <<= 1;
    }*/
}
