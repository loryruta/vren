#pragma once

#include <memory>

#include <volk.h>

#include "Buffer.hpp"
#include "base/ResourceContainer.hpp"

namespace vren
{
    class DeferredBuffer
    {
    public:
        struct CopyOp
        {
            std::shared_ptr<Buffer> m_src_buffer;
            std::shared_ptr<Buffer> m_dst_buffer;
            uint64_t m_src_offset;
            uint64_t m_dst_offset;
            uint64_t m_size;
        };

    private:
        VkMemoryPropertyFlags m_memory_properties;
        VkBufferUsageFlags m_buffer_usage;

        std::shared_ptr<Buffer> m_buffer;
        size_t m_size = 0;

        std::vector<CopyOp> m_operations;

    public:
        explicit DeferredBuffer(
            VkMemoryPropertyFlags memory_properties,
            VkBufferUsageFlags buffer_usage,
            size_t init_size
            );
        ~DeferredBuffer();

        std::shared_ptr<Buffer> buffer() const { return m_buffer; }
        size_t size() const { return m_size; }
        size_t get_pending_operations_count() const { return m_operations.size(); }

        void write(void *data, size_t data_size, size_t offset);
        void resize(size_t init_size);

        void record(VkCommandBuffer command_buffer, ResourceContainer& resource_container);

    private:
        Buffer&& create_buffer(size_t size);

        void perform_copy(VkCommandBuffer command_buffer, ResourceContainer& resource_container, CopyOp const &copy_op);
    };

}  // namespace explo
