#include "ResizableBuffer.hpp"

#include <glm/glm.hpp>

using namespace vren;

ResizableBuffer::ResizableBuffer(VkBufferUsageFlags buffer_usage) :
    m_buffer_usage(buffer_usage)
{
}

void ResizableBuffer::clear()
{
    m_offset = 0;
}

void ResizableBuffer::append_data(void const* data, size_t length)
{
    if (length == 0) return;

    size_t required_size = m_offset + length;
    if (required_size > m_size)
    {
        size_t alloc_size = (size_t) glm::ceil(required_size / (float) k_block_size) * k_block_size;

        std::shared_ptr<Buffer> new_buffer =
            std::make_shared<Buffer>(alloc_host_visible_buffer(
                m_buffer_usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                alloc_size,
                false
            ));

        if (m_buffer)
        {
            copy_buffer(
                m_buffer->m_buffer.m_handle, new_buffer->m_buffer.m_handle, m_size, 0, 0
            );
        }

        m_buffer = new_buffer;
        m_size = alloc_size;
    }

    if (data)
    {
        update_host_visible_buffer(*m_context, *m_buffer, data, length, m_offset);
    }

    m_offset += length;
}

void ResizableBuffer::set_data(void const* data, size_t length)
{
    clear();
    append_data(data, length);
}
