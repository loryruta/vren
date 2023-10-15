#pragma once

#include <memory>

#include <volk.h>

#include "Buffer.hpp"

namespace vren
{
    /// A class that immediately performs the operations on the underlying buffer.
    class ImmediateBuffer
    {
    public:
        static constexpr size_t k_block_size = 1024 * 1024; // 1MB

    private:
        VkMemoryPropertyFlags m_memory_properties;
        VkBufferUsageFlags m_buffer_usage;

        std::shared_ptr<Buffer> m_buffer;
        size_t m_size = 0;

    public:
        explicit ImmediateBuffer(VkMemoryPropertyFlags memory_properties, VkBufferUsageFlags buffer_usage, size_t init_size);
        ImmediateBuffer(ImmediateBuffer const& other) = delete;
        ImmediateBuffer(ImmediateBuffer&& other) = default;
        ~ImmediateBuffer() = default;

        std::shared_ptr<Buffer> buffer() const { return m_buffer; }
        size_t size() const { return m_size; }

        void write(void const* data, size_t size, size_t offset);
        void resize(size_t size, bool keep_data = true);
    };
}
