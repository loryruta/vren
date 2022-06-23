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

    std::vector<uint32_t> m_host_buffer;
    std::unique_ptr<vren::vk_utils::buffer> m_staging_buffer, m_buffer;

    size_t m_current_length = 0;

    blelloch_scan_context() :
        m_blelloch_scan(VREN_TEST_APP()->m_context)
    {
    }
    
private:
    void recreate_buffers(size_t length)
    {
        if (length <= m_current_length)
        {
            return;
        }

        m_host_buffer.resize(length);

        m_staging_buffer = std::make_unique<vren::vk_utils::buffer>(
            vren::vk_utils::alloc_buffer(
                VREN_TEST_APP()->m_context,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                length * sizeof(uint32_t),
                VMA_ALLOCATION_CREATE_MAPPED_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            )
        );

        m_buffer = std::make_unique<vren::vk_utils::buffer>(
            vren::vk_utils::alloc_buffer(
                VREN_TEST_APP()->m_context,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                length * sizeof(uint32_t),
                NULL,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            )
        );

       m_current_length = length;
    }

    void set_gpu_data(uint32_t* data, size_t length)
    {
        recreate_buffers(length);

        std::memcpy(m_staging_buffer->m_allocation_info.pMappedData, data, length * sizeof(uint32_t));

        vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
        {
            VkBufferCopy buffer_copy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = length * sizeof(uint32_t)
            };
            vkCmdCopyBuffer(command_buffer, m_staging_buffer->m_buffer.m_handle, m_buffer->m_buffer.m_handle, 1, &buffer_copy);
        });
    }

public:
    void set_fixed_data(size_t length)
    {
        m_host_buffer.resize(length);

        for (uint32_t i = 0; i < length; i++)
        {
            m_host_buffer[i] = 1;
        }

        set_gpu_data(m_host_buffer.data(), m_host_buffer.size());
    }

    void set_random_data(size_t length)
    {
        m_host_buffer.resize(length);

        std::srand(std::time(nullptr));

        for (uint32_t i = 0; i < length; i++)
        {
            m_host_buffer[i] = std::rand() % 10;
        }

        set_gpu_data(m_host_buffer.data(), m_host_buffer.size());
    }

    void print_data(uint32_t* data, uint32_t length)
    {
        for (uint32_t i = 0; i < length; i++)
        {
            printf("%d, ", data[i]);
        }

        printf("\n");
    }

    void copy_gpu_buffer_to_staging_buffer()
    {
        vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
        {
            VkBufferCopy buffer_copy{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = m_current_length * sizeof(uint32_t)
            };
            vkCmdCopyBuffer(command_buffer, m_buffer->m_buffer.m_handle, m_staging_buffer->m_buffer.m_handle, 1, &buffer_copy);
        });

        return;
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
                .buffer = m_buffer->m_buffer.m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

            auto sample = [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
            {
                m_blelloch_scan(command_buffer, resource_container, *m_buffer, m_current_length, 1, 0);
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
                .buffer = m_buffer->m_buffer.m_handle,
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
        std::exclusive_scan(m_host_buffer.begin(), m_host_buffer.end(), m_host_buffer.begin(), 0);
    }

    void assert_eq()
    {
        copy_gpu_buffer_to_staging_buffer();

        uint32_t* data = reinterpret_cast<uint32_t*>(m_staging_buffer->m_allocation_info.pMappedData);
   
        for (uint32_t i = 0; i < m_current_length; i++)
        {
            uint32_t v = data[i];
            uint32_t r = m_host_buffer[i];

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
        m_context = std::make_unique<blelloch_scan_context>();
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
        m_context->set_random_data(state.range(0));

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
        m_context->set_random_data(state.range(0));
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
    const size_t k_data_length = 1 << 8; // 2^8 = 256

    blelloch_scan_context test{};

    test.set_fixed_data(k_data_length);

    printf("Host buffer:\n"); test.print_data(test.m_host_buffer.data(), k_data_length);
    printf("GPU buffer:\n");  test.print_data(reinterpret_cast<uint32_t*>(test.m_staging_buffer->m_allocation_info.pMappedData), k_data_length);

    test.run_parallel_blelloch_scan();
    test.run_std_exclusive_scan();

    test.copy_gpu_buffer_to_staging_buffer();

    printf("Scanned host buffer:\n"); test.print_data(test.m_host_buffer.data(), k_data_length);
    printf("Scanned GPU buffer:\n");  test.print_data(reinterpret_cast<uint32_t*>(test.m_staging_buffer->m_allocation_info.pMappedData), k_data_length);

    test.assert_eq();
}
