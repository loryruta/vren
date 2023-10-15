#pragma once

#include <memory>
#include <vector>
#include <cstdint>

#include "vren/vk_api/buffer/buffer.hpp"
#include "vren/vk_api/misc_utils.hpp"

#include "app.hpp"

namespace vren_test
{
    class gpu_test_bench
    {
    public:
        std::unique_ptr<vren::vk_utils::buffer> m_host_buffer, m_gpu_buffer;
        uint32_t* m_host_buffer_mapped_pointer = nullptr;

        std::vector<uint32_t> m_reference_buffer;

        size_t m_length = 0, m_alloc_length = 0;

    private:
        void realloc_buffers(size_t length)
        {
            m_length = length;

            if (length < m_alloc_length)
            {
                return;
            }

            m_reference_buffer.resize(length);

            m_host_buffer = std::make_unique<vren::vk_utils::buffer>(
                vren::vk_utils::alloc_host_only_buffer(
                    VREN_TEST_APP()->m_context,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    length * sizeof(uint32_t),
                    true
                )
            );
            m_host_buffer_mapped_pointer = reinterpret_cast<uint32_t*>(m_host_buffer->m_allocation_info.pMappedData);

            m_gpu_buffer = std::make_unique<vren::vk_utils::buffer>(
                vren::vk_utils::alloc_device_only_buffer(
                    VREN_TEST_APP()->m_context,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    length * sizeof(uint32_t)
                )
            );

            m_alloc_length = length;
        }

        void copy_host_buffer_to_gpu_buffer()
        {
            vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
            {
                VkBufferCopy buffer_copy{
                    .srcOffset = 0,
                    .dstOffset = 0,
                    .size = m_length * sizeof(uint32_t)
                };
                vkCmdCopyBuffer(command_buffer, m_host_buffer->m_buffer.m_handle, m_gpu_buffer->m_buffer.m_handle, 1, &buffer_copy);
            });
        }

    public:
        void set_fixed_data(size_t length, uint32_t value)
        {
            realloc_buffers(length);

            for (uint32_t i = 0; i < length; i++)
            {
                m_reference_buffer[i] = value;
            }

            std::memcpy(m_host_buffer_mapped_pointer, m_reference_buffer.data(), length * sizeof(uint32_t));

            copy_host_buffer_to_gpu_buffer();
        }

        void set_random_data(size_t length, uint32_t max_value = 100, uint32_t min_value = 0)
        {
            realloc_buffers(length);

            std::srand(std::time(nullptr));

            for (uint32_t i = 0; i < length; i++)
            {
                m_reference_buffer[i] = std::rand() % (max_value - min_value) + min_value;
            }

            std::memcpy(m_host_buffer_mapped_pointer, m_reference_buffer.data(), length * sizeof(uint32_t));

            copy_host_buffer_to_gpu_buffer();
        }

        void print_data(uint32_t* data, uint32_t length)
        {
            for (uint32_t i = 0; i < length; i++)
            {
                printf("%d, ", data[i]);
            }

            printf("\n");
        }

        void print_reference_buffer()
        {
            print_data(m_reference_buffer.data(), m_length);
        }

        void print_host_buffer()
        {
            print_data(m_host_buffer_mapped_pointer, m_length);
        }

        void copy_gpu_buffer_to_host_buffer()
        {
            vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
            {
                VkBufferCopy buffer_copy{
                    .srcOffset = 0,
                    .dstOffset = 0,
                    .size = m_length * sizeof(uint32_t)
                };
                vkCmdCopyBuffer(command_buffer, m_gpu_buffer->m_buffer.m_handle, m_host_buffer->m_buffer.m_handle, 1, &buffer_copy);
            });
        }

        void assert_eq()
        {
            for (uint32_t i = 0; i < m_length; i++)
            {
                ASSERT_EQ(m_host_buffer_mapped_pointer[i], m_reference_buffer[i]);
            }
        }
    };
}
