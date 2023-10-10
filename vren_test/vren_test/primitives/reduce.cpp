#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

#include <numeric>
#include <memory>

#include <glm/gtc/integer.hpp>
#include <fmt/format.h>

#include "vren/vk_api/utils.hpp"
#include <vren/Context.hpp>
#include <vren/pipeline/profiler.hpp>
#include <vren/primitives/blelloch_scan.hpp>

#include "Toolbox.hpp"
#include "app.hpp"
#include "gpu_test_bench.hpp"

// ------------------------------------------------------------------------------------------------
// Benchmark
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
        vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
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

            VREN_TEST_APP()->m_profiler.profile(command_buffer, resource_container, 0, [&](VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
            {
                VREN_TEST_APP()->m_context.m_toolbox->m_reduce_uint_add(command_buffer, resource_container, buffer, length, 0, 1);
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

template<typename _t>
void run_cpu_reduce(_t* data, uint32_t length, std::function<_t(_t const& a, _t const& b)> const& operation)
{
    assert(vren::is_power_of_2(length));

    for (uint32_t i = 0; i < glm::log2<uint32_t>(length); i++)
    {
        for (uint32_t j = 0; j < (length >> (i + 1)); j++)
        {
            uint32_t a = (1 << i) - 1 + (j << (i + 1));
            uint32_t b = (1 << i) - 1 + (j << (i + 1)) + (1 << i);

            data[b] = operation(data[a], data[b]);
        }
    }
}

template<typename _t, vren::reduce_operation _operation>
void run_cpu_reduce(_t* data, uint32_t length)
{
    run_cpu_reduce<_t>(data, length, [](_t const& a, _t const& b) -> _t
    {
        if constexpr (_operation == vren::ReduceOperationAdd)      return a + b;
        else if constexpr (_operation == vren::ReduceOperationMin) return glm::min(a, b);
        else if constexpr (_operation == vren::ReduceOperationMax) return glm::max(a, b);
    });
}

template<typename _t, vren::reduce_operation _operation>
void run_gpu_reduce(VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container, vren::vk_utils::buffer const& in, vren::vk_utils::buffer const& out, uint32_t length)
{
}

template<>
void run_gpu_reduce<uint32_t, vren::ReduceOperationAdd>(VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container, vren::vk_utils::buffer const& in, vren::vk_utils::buffer const& out, uint32_t length)
{
    VREN_TEST_APP()->m_context.m_toolbox->m_reduce_uint_add(command_buffer, resource_container, in, length, 0, out, 0, 1);
}

template<>
void run_gpu_reduce<uint32_t, vren::ReduceOperationMin>(VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container, vren::vk_utils::buffer const& in, vren::vk_utils::buffer const& out, uint32_t length)
{
    VREN_TEST_APP()->m_context.m_toolbox->m_reduce_uint_min(command_buffer, resource_container, in, length, 0, out, 0, 1);
}

template<>
void run_gpu_reduce<uint32_t, vren::ReduceOperationMax>(VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container, vren::vk_utils::buffer const& in, vren::vk_utils::buffer const& out, uint32_t length)
{
    VREN_TEST_APP()->m_context.m_toolbox->m_reduce_uint_max(command_buffer, resource_container, in, length, 0, out, 0, 1);
}

template<>
void run_gpu_reduce<glm::vec4, vren::ReduceOperationAdd>(VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container, vren::vk_utils::buffer const& in, vren::vk_utils::buffer const& out, uint32_t length)
{
    VREN_TEST_APP()->m_context.m_toolbox->m_reduce_vec4_add(command_buffer, resource_container, in, length, 0, out, 0, 1);
}

template<>
void run_gpu_reduce<glm::vec4, vren::ReduceOperationMin>(VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container, vren::vk_utils::buffer const& in, vren::vk_utils::buffer const& out, uint32_t length)
{
    VREN_TEST_APP()->m_context.m_toolbox->m_reduce_vec4_min(command_buffer, resource_container, in, length, 0, out, 0, 1);
}

template<>
void run_gpu_reduce<glm::vec4, vren::ReduceOperationMax>(VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container, vren::vk_utils::buffer const& in, vren::vk_utils::buffer const& out, uint32_t length)
{
    VREN_TEST_APP()->m_context.m_toolbox->m_reduce_vec4_max(command_buffer, resource_container, in, length, 0, out, 0, 1);
}

template<typename _t>
void fill_reduce_cpu_buffer(_t* buffer, uint32_t length, std::function<_t()> const& init_func, _t out_of_bound_value)
{
    for (uint32_t i = 0; i < vren::round_to_next_power_of_2(length); i++)
    {
        buffer[i] = i < length ? init_func() : out_of_bound_value;
    }

}

template<typename _t, vren::reduce_operation operation>
void fill_reduce_cpu_buffer(_t* buffer, uint32_t length)
{
}

template<>
void fill_reduce_cpu_buffer<uint32_t, vren::reduce_operation::ReduceOperationAdd>(uint32_t* buffer, uint32_t length)
{
    fill_reduce_cpu_buffer<uint32_t>(buffer, length, []() { return 1; }, 0);
}

template<>
void fill_reduce_cpu_buffer<uint32_t, vren::reduce_operation::ReduceOperationMin>(uint32_t* buffer, uint32_t length)
{
    fill_reduce_cpu_buffer<uint32_t>(buffer, length, []() { return std::rand() % 100; }, UINT32_MAX);
}

template<>
void fill_reduce_cpu_buffer<uint32_t, vren::reduce_operation::ReduceOperationMax>(uint32_t* buffer, uint32_t length)
{
    fill_reduce_cpu_buffer<uint32_t>(buffer, length, []() { return std::rand() % 100; }, 0);
}

template<>
void fill_reduce_cpu_buffer<glm::vec4, vren::reduce_operation::ReduceOperationAdd>(glm::vec4* buffer, uint32_t length)
{
    fill_reduce_cpu_buffer<glm::vec4>(buffer, length, []() {
        return glm::vec4(std::rand() % 100, std::rand() % 100, std::rand() % 100, std::rand() % 100);
    }, glm::vec4(0));
}

template<>
void fill_reduce_cpu_buffer<glm::vec4, vren::reduce_operation::ReduceOperationMin>(glm::vec4* buffer, uint32_t length)
{
    fill_reduce_cpu_buffer<glm::vec4>(buffer, length, []() {
        return glm::vec4(std::rand() % 100, std::rand() % 100, std::rand() % 100, std::rand() % 100);
    }, glm::vec4(1e35));
}

template<>
void fill_reduce_cpu_buffer<glm::vec4, vren::reduce_operation::ReduceOperationMax>(glm::vec4* buffer, uint32_t length)
{
    fill_reduce_cpu_buffer<glm::vec4>(buffer, length, []() {
        return glm::vec4(std::rand() % 100, std::rand() % 100, std::rand() % 100, std::rand() % 100);
    }, glm::vec4(-1e35));
}

template<typename _t, vren::reduce_operation _operation>
void run_reduce_test(uint32_t length, bool verbose)
{
    uint32_t length_power_of_2 = vren::round_to_next_power_of_2(length);

    vren::vk_utils::buffer cpu_buffer = vren::vk_utils::alloc_host_visible_buffer( // Input buffer
        VREN_TEST_APP()->m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        length_power_of_2 * sizeof(_t),
        true
    );

    vren::vk_utils::buffer gpu_buffer = vren::vk_utils::alloc_host_visible_buffer( // Output buffer
        VREN_TEST_APP()->m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        length_power_of_2 * sizeof(_t),
        true
    );

    _t* cpu_buffer_ptr = reinterpret_cast<_t*>(cpu_buffer.m_allocation_info.pMappedData);
    _t* gpu_buffer_ptr = reinterpret_cast<_t*>(gpu_buffer.m_allocation_info.pMappedData);

    fill_reduce_cpu_buffer<_t, _operation>(cpu_buffer_ptr, length);

    if (verbose)
    {
        //fmt::print("Before reduction:\n");
        //fmt::print("CPU buffer:\n"); vren_test::print_buffer<uint32_t>((uint32_t*) cpu_buffer_ptr, length_power_of_2);
        //fmt::print("GPU buffer:\n"); vren_test::print_buffer<uint32_t>(gpu_buffer_ptr, length);
    }

    // Run GPU reduction
    vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
    {
        run_gpu_reduce<_t, _operation>(command_buffer, resource_container, cpu_buffer, gpu_buffer, length);
    });

    // Run CPU reduction
    run_cpu_reduce<_t, _operation>(cpu_buffer_ptr, length_power_of_2);

    if (verbose)
    {
        //fmt::print("After reduction:\n");
        //fmt::print("CPU buffer:\n"); vren_test::print_buffer<uint32_t>((uint32_t*) cpu_buffer_ptr, length_power_of_2);
        //fmt::print("GPU buffer:\n"); vren_test::print_buffer<uint32_t>((uint32_t*) gpu_buffer_ptr, length_power_of_2);
    }

    for (uint32_t i = 0; i < length_power_of_2; i++)
    {
        if (cpu_buffer_ptr[i] != gpu_buffer_ptr[i])
        {
            fmt::print("ERROR: {}\n", i);
        }

        ASSERT_EQ(cpu_buffer_ptr[i], gpu_buffer_ptr[i]);
    }
}

TEST(reduce, type_uint)
{
    uint32_t length = 10000;

    fmt::print("[reduce.type_uint] Reduce uint add, length {}\n", length);
    run_reduce_test<uint32_t, vren::ReduceOperationAdd>(length, false);

    fmt::print("[reduce.type_uint] Reduce uint min, length {}\n", length);
    run_reduce_test<uint32_t, vren::ReduceOperationMin>(length, false);

    fmt::print("[reduce.type_uint] Reduce uint max, length {}\n", length);
    run_reduce_test<uint32_t, vren::ReduceOperationMax>(length, false);
}

TEST(reduce, type_vec4)
{
    uint32_t length = 10000;

    fmt::print("[reduce.type_vec4] Reduce vec4 add, length {}\n", length);
    run_reduce_test<glm::vec4, vren::ReduceOperationAdd>(length, false);

    fmt::print("[reduce.type_vec4] Reduce vec4 min, length {}\n", length);
    run_reduce_test<glm::vec4, vren::ReduceOperationMin>(length, false);

    fmt::print("[reduce.type_vec4] Reduce vec4 max, length {}\n", length);
    run_reduce_test<glm::vec4, vren::ReduceOperationMax>(length, false);
}

TEST(reduce, var_length)
{
    for (uint32_t length = 1; length < 10e5; length *= 10)
    {
        fmt::print("[reduce.var_length] Reduce uint add, length {}\n", length);
        run_reduce_test<uint32_t, vren::ReduceOperationAdd>(length, false);
    }
}
