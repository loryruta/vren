#pragma once

#include <memory>

#include <volk.h>

#include "Buffer.hpp"

namespace vren
{
    class ResizableBuffer // todo rename to ImmediateBuffer
    {
    public:
        static constexpr size_t k_block_size = 1024 * 1024; // 1MB

    private:
        std::shared_ptr<Buffer> m_buffer;
        size_t m_size = 0;
        size_t m_offset = 0;

        VkBufferUsageFlags m_buffer_usage;

    public:
        explicit ResizableBuffer(VkBufferUsageFlags buffer_usage);
        ResizableBuffer(ResizableBuffer const& other) = delete;
        ResizableBuffer(ResizableBuffer&& other) = default;
        ~ResizableBuffer() = default;

        void clear();
        void append_data(void const* data, size_t length);
        void set_data(void const* data, size_t length);
    };
}
