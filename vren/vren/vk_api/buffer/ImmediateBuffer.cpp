#include "ImmediateBuffer.hpp"

#include <cassert>

#include "utils.hpp"

using namespace vren;

ImmediateBuffer::ImmediateBuffer(VkMemoryPropertyFlags memory_properties, VkBufferUsageFlags buffer_usage, size_t init_size) :
    m_memory_properties(memory_properties),
    m_buffer_usage(buffer_usage)
{
    resize(init_size);
}

void ImmediateBuffer::write(void const* data, size_t size, size_t offset)
{
    assert(data);
    assert(size);
    assert(m_size > offset); // TODO Add a message

    update_buffer(*m_buffer, data, size, offset);
}

void ImmediateBuffer::resize(size_t size, bool keep_data)
{
    assert(size);

    std::shared_ptr<Buffer> new_buffer = std::make_shared<Buffer>(allocate_buffer(m_memory_properties, m_buffer_usage, size));

    if (keep_data)
        copy_buffer(*m_buffer, *new_buffer, std::min(size, m_size), 0, 0);

    m_buffer = new_buffer;
    m_size = size;
}
