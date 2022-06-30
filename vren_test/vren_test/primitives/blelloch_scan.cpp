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

class blelloch_scan_test_bench : public vren_test::gpu_test_bench
{
public:
    void run_gpu_blelloch_scan(uint64_t* elapsed_time = nullptr)
    {
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
                VREN_TEST_APP()->m_blelloch_scan(command_buffer, resource_container, *m_gpu_buffer, m_length, 0, 1);
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
    }

    void run_std_exclusive_scan()
    {
        std::exclusive_scan(m_reference_buffer.begin(), m_reference_buffer.end(), m_reference_buffer.begin(), 0);
    }
};

// ------------------------------------------------------------------------------------------------
// Benchmarking
// ------------------------------------------------------------------------------------------------

static void BM_gpu_blelloch_scan(benchmark::State& state)
{
    blelloch_scan_test_bench m_test_bench{};

    for (auto _ : state)
    {
        m_test_bench.set_random_data(state.range(0));

        uint64_t elapsed_time;
        m_test_bench.run_gpu_blelloch_scan(&elapsed_time);
        state.SetIterationTime(elapsed_time / (double)1e9);
    }
}

BENCHMARK(BM_gpu_blelloch_scan)
    ->Unit(benchmark::kMicrosecond)
    ->Range(1 << 10, 1 << 29 /* < maxStorageBufferRange */)
    ->UseManualTime();

static void BM_std_exclusive_scan(benchmark::State& state)
{
    blelloch_scan_test_bench m_test_bench{};

    for (auto _ : state)
    {
        state.PauseTiming();
        m_test_bench.set_random_data(state.range(0));
        state.ResumeTiming();

        m_test_bench.run_std_exclusive_scan();
    }
}

BENCHMARK(BM_std_exclusive_scan)
    ->Unit(benchmark::kMicrosecond)
    ->Range(1 << 16, 1 << 28);

// ------------------------------------------------------------------------------------------------
// Unit testing
// ------------------------------------------------------------------------------------------------

void run_blelloch_scan_test(uint32_t sample_length, bool verbose)
{
    std::vector<uint32_t> cpu_buffer(sample_length);
    vren::vk_utils::buffer gpu_buffer =
        vren::vk_utils::alloc_host_visible_buffer(VREN_TEST_APP()->m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sample_length * sizeof(uint32_t), true);

    uint32_t* gpu_buffer_ptr = reinterpret_cast<uint32_t*>(gpu_buffer.m_allocation_info.pMappedData);

    vren_test::fill_with_random_int_values(cpu_buffer.begin(), cpu_buffer.end(), 0, 100);
    //std::fill(cpu_buffer.begin(), cpu_buffer.end(), 1);

    std::memcpy(gpu_buffer_ptr, cpu_buffer.data(), sample_length * sizeof(uint32_t));

    if (verbose)
    {
        VREN_DEBUG("Before reduction:\n");
        VREN_DEBUG("CPU buffer:\n"); vren_test::print_buffer<uint32_t>(cpu_buffer.data(), sample_length);
        VREN_DEBUG("GPU buffer:\n"); vren_test::print_buffer<uint32_t>(gpu_buffer_ptr, sample_length);
    }

    std::exclusive_scan(cpu_buffer.begin(), cpu_buffer.end(), cpu_buffer.begin(), 0);

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

        VREN_TEST_APP()->m_blelloch_scan(command_buffer, resource_container, gpu_buffer, sample_length, 0, 1);
    });

    if (verbose)
    {
        VREN_DEBUG("After reduction:\n");
        VREN_DEBUG("CPU buffer:\n"); vren_test::print_buffer<uint32_t>(cpu_buffer.data(), sample_length);
        VREN_DEBUG("GPU buffer:\n"); vren_test::print_buffer<uint32_t>(gpu_buffer_ptr, sample_length);
    }

    for (uint32_t i = 0; i < sample_length; i++)
    {
        ASSERT_EQ(cpu_buffer.at(i), gpu_buffer_ptr[i]);
    }
}

TEST(blelloch_scan, main)
{
    run_blelloch_scan_test(1 << 20, false);

    uint32_t length = vren::blelloch_scan::k_min_buffer_length;
    while (length <= (1 << 20))
    {
        fmt::print("LENGTH: {}\n", length);

        run_blelloch_scan_test(length, false);
        length <<= 1;
    }
}

