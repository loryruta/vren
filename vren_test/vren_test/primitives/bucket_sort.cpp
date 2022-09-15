

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
    vren::context const& context = VREN_TEST_APP()->m_context;

    vren::bucket_sort& bucket_sort = VREN_TEST_APP()->m_context.m_toolbox->m_bucket_sort;

    size_t length = state.range(0);

    vren::vk_utils::buffer input_buffer = vren::vk_utils::alloc_device_only_buffer(
        context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        length * sizeof(glm::uvec2)
    );

    vren::vk_utils::buffer output_buffer = vren::vk_utils::alloc_device_only_buffer(
        context,
        vren::bucket_sort::get_required_output_buffer_usage_flags(),
        vren::bucket_sort::get_required_output_buffer_size(length)
    );

    vren::vk_utils::set_name(context, input_buffer, "input_buffer");
    vren::vk_utils::set_name(context, output_buffer, "output_buffer");

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
    ->Range(1 << 10, 1 << 28)
    ->Iterations(1)
    ->UseManualTime();

// ------------------------------------------------------------------------------------------------
// Unit testing
// ------------------------------------------------------------------------------------------------

void run_bucket_sort_test(uint32_t length, bool verbose, bool check_values)
{
    vren::bucket_sort& bucket_sort = VREN_TEST_APP()->m_context.m_toolbox->m_bucket_sort;

    vren::vk_utils::buffer input_buffer = vren::vk_utils::alloc_host_only_buffer(
        VREN_TEST_APP()->m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        length * sizeof(glm::uvec2),
        true
    );

    vren::vk_utils::buffer output_buffer = vren::vk_utils::alloc_host_only_buffer(
        VREN_TEST_APP()->m_context,
        vren::bucket_sort::get_required_output_buffer_usage_flags(),
        vren::bucket_sort::get_required_output_buffer_size(length),
        true
    );

    glm::uvec2* input_buffer_ptr = reinterpret_cast<glm::uvec2*>(input_buffer.get_mapped_pointer());
    glm::uvec2* output_buffer_ptr = reinterpret_cast<glm::uvec2*>(output_buffer.get_mapped_pointer());

    // Init
    for (uint32_t i = 0; i < length; i++)
    {
        input_buffer_ptr[i].x = std::rand() % vren::bucket_sort::k_key_size;
        input_buffer_ptr[i].y = i;
    }

    if (verbose)
    {
        // CPU buffer
        fmt::print("CPU buffer:\n");
        for (uint32_t i = 0; i < length; i++)
        {
            fmt::print("{} {}, ", input_buffer_ptr[i].x, input_buffer_ptr[i].y);
        }
        fmt::print("\n");

        // GPU buffer
        fmt::print("GPU buffer:\n");
        for (uint32_t i = 0; i < length; i++)
        {
            fmt::print("{} {}, ", output_buffer_ptr[i].x, output_buffer_ptr[i].y);
        }
        fmt::print("\n");
    }

    // Sort GPU array
    vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
    {
        bucket_sort(command_buffer, resource_container, input_buffer, length, 0, output_buffer, 0);
    });

    // Sort CPU array
    std::sort(input_buffer_ptr, input_buffer_ptr + length, [&](glm::uvec2 const& a, glm::uvec2 const& b) -> bool
    {
        return (a.x & vren::bucket_sort::k_key_mask) < (b.x & vren::bucket_sort::k_key_mask);
    });

    if (verbose)
    {
        // CPU buffer
        fmt::print("CPU buffer:\n");
        for (uint32_t i = 0; i < length; i++)
        {
            fmt::print("{} {}, ", input_buffer_ptr[i].x, input_buffer_ptr[i].y);
        }
        fmt::print("\n");

        // GPU buffer
        fmt::print("GPU buffer:\n");
        for (uint32_t i = 0; i < length; i++)
        {
            fmt::print("{} {}, ", output_buffer_ptr[i].x, output_buffer_ptr[i].y);
        }
        fmt::print("\n");
    }

    // Check keys
    for (uint32_t i = 0; i < length; i++)
    {
        ASSERT_EQ(input_buffer_ptr[i].x, output_buffer_ptr[i].x);
    }

    // Check values
    if (check_values)
    {
        for (uint32_t i = 0; i < length; i++)
        {
            bool found = false;
            for (uint32_t j = 0; j < length; j++)
            {
                if (output_buffer_ptr[j].y == i) // Every value must be present at least one time in the output buffer because the way it's initialized
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                FAIL();
            }
        }
    }
}

TEST(bucket_sort, main)
{
    //run_bucket_sort_test(2, true);

    uint32_t length = 1;
    for (; length < 10e6; length *= (std::rand() % 3 + 4))
    {
        fmt::print("[bucket_sort] Length: {}\n", length);
        run_bucket_sort_test(length, false, length < 10000);
    }
}
