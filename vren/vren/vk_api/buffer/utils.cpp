#include "utils.hpp"

#include "Context.hpp"

// References:
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html

using namespace vren;

uint32_t find_memory_type_bits(uint32_t required_flags, uint32_t forbidden_flags)
{
    Context::get().physical_device();

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

std::string to_string(VkMemoryPropertyFlags memory_property_flags)
{
    std::string result = "";
    uint32_t i = 1;
    while (i)
    {
        VkMemoryPropertyFlags memory_property_flag = memory_property_flags & i;

        if (memory_property_flag != 0)
        {
            switch (memory_property_flag)
            {
            case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT:
                result += "DEVICE_LOCAL";
                break;
            case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT:
                result += "HOST_VISIBLE";
                break;
            case VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:
                result += "HOST_COHERENT";
                break;
            case VK_MEMORY_PROPERTY_HOST_CACHED_BIT:
                result += "HOST_CACHED";
                break;
            default:
                result += "UNKNOWN";
                break;
            }

            if (memory_property_flags & ~((1 << (i << 1)) - 1))
            {
                result += ", ";
            }
        }

        i <<= 1;
    }
    return result;
}

Buffer&& vren::allocate_buffer(
    VkMemoryPropertyFlags memory_properties,
    VkBufferUsageFlags buffer_usage,
    size_t size,
    bool persistently_mapped
)
{
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = buffer_usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_flags.memoryTypeBits = memory_type_bits;
        .pool = VK_NULL_HANDLE,
        .pUserData = nullptr,
        .priority = 0.0f};

    for (uint32_t i = 0; i < std::size(required_flags_attempts); i++)
    {
        uint32_t memory_type_bits = find_memory_type_bits(context, required_flags_attempts.at(i), forbidden_flags);

        VkBuffer buffer{};
        VmaAllocation allocation{};
        VmaAllocationInfo allocation_info{};
        VkResult result = vmaCreateBuffer(
            context.m_vma_allocator,
            &buffer_create_info,
            &allocation_create_info,
            &buffer,
            &allocation,
            &allocation_info
        );

        if (result == VK_SUCCESS)
        {
            VkPhysicalDeviceMemoryProperties memory_properties{};
            vkGetPhysicalDeviceMemoryProperties(context.m_physical_device, &memory_properties);

            VkMemoryType memory_type = memory_properties.memoryTypes[allocation_info.memoryType];
            VREN_DEBUG0(
                "[buffer] Buffer allocated - Size: {}, Memory type: {}, Memory property flags: {}, Heap: {}\n",
                allocation_info.size,
                allocation_info.memoryType,
                to_string(memory_type.propertyFlags),
                memory_type.heapIndex
            );

            return vren::vk_utils::buffer{
                .m_buffer = vren::vk_buffer(context, buffer),
                .m_allocation = vren::vma_allocation(context, allocation),
                .m_allocation_info = allocation_info};
        }
        else if (i == std::size(required_flags_attempts) - 1)
        {
            VREN_ERROR(
                "[buffer] Failed to allocate buffer - Size: {} - Memory type bits: {:x}\n", size, memory_type_bits
            );

            // If even the last attempt fails then we can't allocate the buffer
            VREN_CHECK(result, &context);
        }
    }
}

vren::vk_utils::buffer vren::vk_utils::alloc_host_visible_buffer(
    vren::context const& context, VkBufferUsageFlags buffer_usage, size_t size, bool persistently_mapped
)
{
    return alloc_buffer(
        context,
        buffer_usage,
        size,
        {VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT},
        NULL,
        persistently_mapped ? VMA_ALLOCATION_CREATE_MAPPED_BIT : NULL
    );
}

void vren::vk_utils::update_host_visible_buffer(
    vren::context const& ctx, vren::vk_utils::buffer& buf, void const* data, size_t size, size_t dst_offset
)
{
    void* mapped_data;

    VmaAllocation alloc = buf.m_allocation.m_handle;

    if (!alloc->IsPersistentMap())
    {
        vmaMapMemory(ctx.m_vma_allocator, alloc, &mapped_data);
    }
    else
    {
        mapped_data = alloc->GetMappedData();
    }

    std::memcpy(static_cast<uint8_t*>(mapped_data) + dst_offset, data, size);

    if (!alloc->IsPersistentMap())
    {
        vmaUnmapMemory(ctx.m_vma_allocator, alloc);
    }

    VkMemoryPropertyFlags mem_flags;
    vmaGetMemoryTypeProperties(ctx.m_vma_allocator, alloc->GetMemoryTypeIndex(), &mem_flags);
    if ((mem_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
    {
        vmaFlushAllocation(ctx.m_vma_allocator, alloc, dst_offset, size);
    }
}

void vren::vk_utils::copy_buffer(
    vren::context const& context,
    VkBuffer src_buffer,
    VkBuffer dst_buffer,
    size_t size,
    size_t src_offset,
    size_t dst_offset
)
{
    vren::vk_utils::immediate_transfer_queue_submit(
        context,
        [&](VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
        {
            VkBufferCopy copy_region{.srcOffset = src_offset, .dstOffset = dst_offset, .size = size};
            vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
            vkCmdSetCheckpointNV(command_buffer, "Buffer copied");
        }
    );
}

vren::vk_utils::buffer vren::vk_utils::create_device_only_buffer(
    vren::context const& ctx, VkBufferUsageFlags buffer_usage, void const* data, size_t size
)
{
    auto buf = vren::vk_utils::alloc_device_only_buffer(ctx, buffer_usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size);
    if (data != nullptr)
    {
        vren::vk_utils::update_device_only_buffer(ctx, buf, data, size, 0);
    }
    return buf;
}
