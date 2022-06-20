#include <gtest/gtest.h>

#include <numeric>
#include <chrono>

#include <glm/gtc/integer.hpp>
#include <fmt/format.h>

#include <vren/context.hpp>
#include <vren/primitive/blelloch_scan.hpp>
#include <vren/vk_helpers/misc.hpp>

TEST(BlellochScan, Main)
{
    std::srand(std::time(nullptr));

    vren::context_info context_info{};
    vren::context context(context_info);

    vren::blelloch_scan scan(context);

    constexpr size_t N = 1 << 20; // 2^20 = 1MB

    // Init reference
    std::vector<uint32_t> reference(N);
    for (uint32_t i = 0; i < N; i++)
    {
        reference[i] = std::rand() % 10;
    }

    // Init input
    vren::vk_utils::buffer buffer = vren::vk_utils::alloc_host_visible_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, N * sizeof(uint32_t), true);
    uint32_t* values = reinterpret_cast<uint32_t*>(buffer.m_allocation_info.pMappedData);
    std::memcpy(values, reference.data(), N * sizeof(uint32_t));

    // GPU blelloch-scan
    VkBufferMemoryBarrier buffer_memory_barrier{};

    std::chrono::system_clock::time_point start_at, end_at;
    
    start_at = std::chrono::system_clock().now();

    vren::vk_utils::immediate_graphics_queue_submit(context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
    {
        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
            .buffer = buffer.m_buffer.m_handle,
            .offset = 0,
            .size = VK_WHOLE_SIZE
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        scan(command_buffer, resource_container, buffer, N, 1, 0);

        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT,
            .buffer = buffer.m_buffer.m_handle,
            .offset = 0,
            .size = VK_WHOLE_SIZE
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_HOST_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
    });

    end_at = std::chrono::system_clock().now();

    fmt::print("GPU Blelloch-scan - {} elements, {} ms\n", N, std::chrono::duration_cast<std::chrono::milliseconds>(end_at - start_at).count());

    // Host-side exclusive-scan
    start_at = std::chrono::system_clock().now();

    std::exclusive_scan(reference.begin(), reference.end(), reference.begin(), 0);

    end_at = std::chrono::system_clock().now();

    fmt::print("Host-side std::exclusive_scan - {} elements, {} ms\n", N, std::chrono::duration_cast<std::chrono::milliseconds>(end_at - start_at).count());

    // Check
    for (uint32_t i = 0; i < N; i++)
    {
        ASSERT_EQ(values[i], reference[i]);
    }

    __noop();
}
