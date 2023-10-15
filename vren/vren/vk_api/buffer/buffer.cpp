#include "buffer.hpp"

#include "Context.hpp"

using namespace vren;

Buffer::Buffer(VkBuffer buffer, size_t size, VmaAllocation allocation) :
    m_buffer(buffer),
    m_size(size),
    m_allocation(allocation)
{
}

Buffer::Buffer(VkBuffer buffer, size_t size, VkDeviceMemory device_memory) :
    m_buffer(buffer),
    m_size(size),
    m_device_memory(device_memory)
{
}

VmaAllocationInfo Buffer::vma_allocation_info() const
{
    assert(m_allocation);
    VmaAllocationInfo allocation_info{};
    vmaGetAllocationInfo(Context::get().vma_allocator(), m_allocation->get(), &allocation_info);
    return allocation_info;
}

VmaAllocation Buffer::vma_allocation() const
{
    assert(m_allocation);
    return m_allocation->get();
}

VkDeviceMemory Buffer::device_memory() const
{
    assert(m_device_memory);
    return m_device_memory->get();
}

void* Buffer::mapped_pointer() const
{
    return vma_allocation_info().pMappedData;
}

// ------------------------------------------------------------------------------------------------

// References:
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html

namespace
{
    uint32_t find_memory_type_bits(VkMemoryPropertyFlags required_memory_flags)
    { // TODO
        Context::get().physical_device().memory_properties();

        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(context.m_physical_device, &memory_properties);

        uint32_t memory_type_bits = 0;
        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
        {
            VkMemoryPropertyFlags flags = memory_properties.memoryTypes[i].propertyFlags;

            bool match = true;
            match &= ((flags & required_flags) ^ required_flags) == 0;
            match &= (flags & forbidden_flags) == 0;

            if (match)
            {
                memory_type_bits |= 1 << i;
            }
        }
        return memory_type_bits;
    }
} // namespace

Buffer vren::allocate_buffer(VkMemoryPropertyFlags memory_properties, VkBufferUsageFlags buffer_usage, size_t size, bool persistently_mapped)
{
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = buffer_usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.memoryTypeBits = find_memory_type_bits(memory_properties);
    allocation_create_info.flags = persistently_mapped ? VMA_ALLOCATION_CREATE_MAPPED_BIT : NULL;

    VkBuffer buffer{};
    VmaAllocation allocation{};
    VmaAllocationInfo allocation_info{};
    VREN_CHECK(vmaCreateBuffer(Context::get().vma_allocator(), &buffer_create_info, &allocation_create_info, &buffer, &allocation, &allocation_info));
    return Buffer(buffer, size, allocation);
}

void vren::record_copy_buffer(
    std::shared_ptr<Buffer>& src_buffer,
    std::shared_ptr<Buffer>& dst_buffer,
    size_t size,
    size_t src_offset,
    size_t dst_offset,
    VkCommandBuffer command_buffer,
    vren::ResourceContainer& resource_container
)
{
    VkBufferCopy copy_region{};
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer->buffer(), dst_buffer->buffer(), 1, &copy_region);

    resource_container.add_resources(src_buffer, dst_buffer);
}

void vren::copy_buffer(Buffer& src_buffer, Buffer& dst_buffer, size_t size, size_t src_offset, size_t dst_offset)
{ // TODO
}

void vren::record_update_buffer(
    std::shared_ptr<Buffer>& buffer,
    void const* data,
    size_t size,
    size_t offset,
    VkCommandBuffer command_buffer,
    ResourceContainer& resource_container
)
{ // TODO can't import_buffer, data could be deleted in the meanwhile
}

void vren::update_buffer(Buffer& buffer, void const* data, size_t size, size_t offset)
{
    std::shared_ptr<Buffer> update_buffer = std::make_shared<Buffer>(import_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, data, size));

    // TODO
    //copy_buffer_deferred(update_buffer, buffer, size, 0, offset, command_buffer, resource_container);
}

Buffer vren::import_buffer(VkBufferUsageFlags buffer_usage, void const* data, size_t size)
{
    VkImportMemoryHostPointerInfoEXT import_memory_host_pointer_info{};
    import_memory_host_pointer_info.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT;
    import_memory_host_pointer_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
    import_memory_host_pointer_info.pHostPointer = const_cast<void*>(data);

    VkMemoryHostPointerPropertiesEXT memory_host_pointer_properties{};
    vkGetMemoryHostPointerPropertiesEXT(
        Context::get().device().handle(), VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT, data, &memory_host_pointer_properties
    );

    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.pNext = &import_memory_host_pointer_info;
    memory_allocate_info.memoryTypeIndex = memory_host_pointer_properties.memoryTypeBits;

    VkDeviceMemory device_memory{};
    VREN_CHECK(vkAllocateMemory(Context::get().device().handle(), &memory_allocate_info, nullptr, &device_memory));

    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = buffer_usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer{};
    VREN_CHECK(vkCreateBuffer(Context::get().device().handle(), &buffer_create_info, nullptr, &buffer));

    return Buffer(buffer, device_memory);
}

