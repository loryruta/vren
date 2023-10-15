#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "vk_api/vk_raii.hpp"
#include "base/ResourceContainer.hpp"

namespace vren
{
    /// A RAII wrapper for VkBuffer associated to its VmaAllocation, if it was allocated using VmaAllocator; or its VkDeviceMemory (if e.g. was imported).
    class Buffer
    {
    private:
        vk_buffer m_buffer;
        size_t m_size;

        std::optional<vma_allocation> m_allocation;
        std::optional<vk_device_memory> m_device_memory;

    public:
        /// Constructs a buffer associated with a VMA allocation (common case).
        explicit Buffer(VkBuffer buffer, size_t size, VmaAllocation allocation);

        /// Constructs a buffer associated with a dedicated device memory (used e.g. for imported buffers).
        explicit Buffer(VkBuffer buffer, size_t size, VkDeviceMemory memory);

        Buffer(Buffer const& other) = delete;
        Buffer(Buffer&& other) = default;
        ~Buffer() = default;

        VkBuffer buffer() const { return m_buffer.get(); }
        size_t size() const { return m_size; };

        VmaAllocationInfo vma_allocation_info() const;

        bool has_vma_allocation() const { return m_allocation.has_value(); }
        VmaAllocation vma_allocation() const;

        bool has_device_memory() const  { return m_device_memory.has_value(); }
        VkDeviceMemory device_memory() const;

        void* mapped_pointer() const;
    };

    // ------------------------------------------------------------------------------------------------

    Buffer allocate_buffer(
        VkMemoryPropertyFlags memory_properties,
        VkBufferUsageFlags buffer_usage,
        size_t size,
        bool persistently_mapped = false
    );

    void record_update_buffer(
        std::shared_ptr<Buffer>& buffer,
        void const* data,
        size_t size,
        size_t offset,
        VkCommandBuffer command_buffer,
        ResourceContainer& resource_container
        );

    void update_buffer(
        Buffer& buffer,
        void const* data,
        size_t size,
        size_t offset
        );

    void record_copy_buffer(
        std::shared_ptr<Buffer>& src_buffer,
        std::shared_ptr<Buffer>& dst_buffer,
        size_t size,
        size_t src_offset,
        size_t dst_offset,
        VkCommandBuffer command_buffer,
        ResourceContainer& resource_container
        );

    void copy_buffer(
        Buffer& src_buffer,
        Buffer& dst_buffer,
        size_t size,
        size_t src_offset,
        size_t dst_offset
        );

    Buffer import_buffer(
        VkBufferUsageFlags buffer_usage,
        void const* data,
        size_t size
        );
} // namespace vren
