#include "DeferredBuffer.hpp"

#include "Context.hpp"

using namespace vren;

DeferredBuffer::DeferredBuffer(
    VkMemoryPropertyFlags memory_properties,
    VkBufferUsageFlags buffer_usage,
    size_t init_size
    ) :
    m_memory_properties(memory_properties),
    m_buffer_usage(buffer_usage),
    m_size(init_size)
{
    m_buffer = std::make_shared<Buffer>(create_buffer(init_size));
}

void DeferredBuffer::write(void const* data, size_t data_size, size_t offset)
{
    // TODO IMPROVEMENT: Cache and re-use the staging buffers; avoid allocating each write

    std::shared_ptr<Buffer> staging_buffer = std::make_shared<Buffer>(allocate_buffer(
        m_buffer_usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        data_size,
        false  // Persistently mapped
    ));

    vren::vk_utils::set_name(*staging_buffer, "DeviceBuffer-Write-StagingBuffer");

    void *staging_buffer_data;
    vmaMapMemory(Context::get().vma_allocator(), staging_buffer->m_allocation.m_handle, &staging_buffer_data);
    {
        std::memcpy(staging_buffer_data, data, data_size);
    }
    vmaUnmapMemory(m_renderer.m_context.m_vma_allocator, staging_buffer->m_allocation.m_handle);

    // Enqueue a copy operation to copy data from the staging buffer to the actual device buffer
    // TODO this is the same as registering the copy on a secondary command buffer; we have to review vren's command_pool/command_buffer API first
    CopyOp copy_op{};
    copy_op.m_src_buffer = staging_buffer;
    copy_op.m_dst_buffer = m_buffer;
    copy_op.m_src_offset = 0;
    copy_op.m_dst_offset = offset;
    copy_op.m_size = data_size;

    m_operations.push_back(copy_op);
}

void DeferredBuffer::resize(size_t size)
{
    if (size <= m_size) return;

    const size_t k_grow_factor = 4096;  // TODO Change this in order to minimize resizes
    size_t new_size = glm::ceil(double(size) / double(k_grow_factor)) * k_grow_factor;

    std::shared_ptr<vren::vk_utils::buffer> new_buffer = std::make_shared<vren::vk_utils::buffer>(create_buffer(new_size));

    // Enqueue a copy operation to copy data from the old buffer to the new buffer
    CopyOp copy_op{};
    copy_op.m_src_buffer = m_buffer;
    copy_op.m_dst_buffer = new_buffer;
    copy_op.m_src_offset = 0;
    copy_op.m_dst_offset = 0;
    copy_op.m_size = std::min(m_size, new_size);

    m_operations.push_back(copy_op);

    // Update the state
    m_buffer = new_buffer;
    m_size = new_size;
}

void DeferredBuffer::record(VkCommandBuffer command_buffer, ResourceContainer& resource_container)
{
    // TODO IMPROVEMENT: this could be replaced with a compute shader that performs all the copies in parallel
    // TODO we only have OP_WRITE, don't make an enum and simplify Op

    for (CopyOp const &copy_op : m_operations) perform_copy(command_buffer, resource_container, copy_op);

    m_operations.clear();
}

void DeferredBuffer::perform_copy(VkCommandBuffer command_buffer, ResourceContainer& resource_container, CopyOp const &copy_op)
{
    VkBufferCopy buffer_copy{};
    buffer_copy.srcOffset = copy_op.m_src_offset;
    buffer_copy.dstOffset = copy_op.m_dst_offset;
    buffer_copy.size = copy_op.m_size;

    vkCmdCopyBuffer(
        command_buffer,
        copy_op.m_src_buffer->buffer(), // srcBuffer
        copy_op.m_dst_buffer->buffer(), // dstBuffer
        1,
        &buffer_copy
    );

    resource_container.add_resources(copy_op.m_src_buffer, copy_op.m_dst_buffer, m_buffer);
}

Buffer&& DeferredBuffer::create_buffer(size_t size)
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = m_buffer_usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_create_info{};
    alloc_create_info.requiredFlags = m_memory_properties;

    VkBuffer buffer;
    VmaAllocation allocation;
    vmaCreateBuffer(Context::get().vma_allocator(), &buffer_info, &alloc_create_info, &buffer, &allocation, nullptr);

    return Buffer(buffer, size, allocation);
}
