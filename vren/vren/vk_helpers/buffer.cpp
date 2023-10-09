#include "buffer.hpp"

#include <span>

#include <volk.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "Context.hpp"
#include "vk_helpers/misc.hpp"

#include "log.hpp"

#ifdef VREN_LOG_BUFFER_DETAILED
#define VREN_DEBUG0(m, ...) VREN_DEBUG(m, __VA_ARGS__)
#else
#define VREN_DEBUG0
#endif

// References:
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html

uint32_t find_memory_type_bits(vren::context const& context, uint32_t required_flags, uint32_t forbidden_flags)
{
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

vren::vk_utils::buffer alloc_buffer(
    vren::context const& context,
    VkBufferUsageFlags buffer_usage,
    size_t size,
    std::vector<VkMemoryPropertyFlags> const& required_flags_attempts,
    VkMemoryPropertyFlags forbidden_flags,
    VmaAllocationCreateFlags allocation_create_flags
)
{
    assert(size > 0);

    VkBufferCreateInfo buffer_create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .size = size,
        .usage = buffer_usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr};

    for (uint32_t i = 0; i < std::size(required_flags_attempts); i++)
    {
        uint32_t memory_type_bits = find_memory_type_bits(context, required_flags_attempts.at(i), forbidden_flags);

        VmaAllocationCreateInfo allocation_create_info{
            .flags = allocation_create_flags,
            .usage = VMA_MEMORY_USAGE_UNKNOWN,
            .requiredFlags = NULL,
            .preferredFlags = NULL,
            .memoryTypeBits = memory_type_bits,
            .pool = VK_NULL_HANDLE,
            .pUserData = nullptr,
            .priority = 0.0f};

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

vren::vk_utils::buffer
vren::vk_utils::alloc_device_only_buffer(vren::context const& context, VkBufferUsageFlags buffer_usage, size_t size)
{
    return alloc_buffer(
        context, buffer_usage, size, {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT}, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, NULL
    );
}

vren::vk_utils::buffer vren::vk_utils::alloc_host_only_buffer(
    vren::context const& context, VkBufferUsageFlags buffer_usage, VkDeviceSize size, bool persistently_mapped
)
{
    return alloc_buffer(
        context,
        buffer_usage,
        size,
        {VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
             VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT},
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
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

void vren::vk_utils::update_device_only_buffer(
    vren::context const& context, vren::vk_utils::buffer& buffer, void const* data, size_t size, size_t dst_offset
)
{
    vren::vk_utils::buffer staging_buffer =
        alloc_host_visible_buffer(context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, false);
    update_host_visible_buffer(context, staging_buffer, data, size, 0);

    copy_buffer(context, staging_buffer.m_buffer.m_handle, buffer.m_buffer.m_handle, size, 0, dst_offset);
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

// --------------------------------------------------------------------------------------------------------------------------------
// Resizable buffer
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::resizable_buffer::resizable_buffer(vren::context const& context, VkBufferUsageFlags buffer_usage) :
    m_context(&context),
    m_buffer_usage(buffer_usage)
{
}

vren::vk_utils::resizable_buffer::resizable_buffer(vren::vk_utils::resizable_buffer const&& other) :
    m_context(other.m_context),
    m_buffer_usage(other.m_buffer_usage),
    m_buffer(std::move(other.m_buffer)),
    m_size(other.m_size),
    m_offset(other.m_offset)
{
}

vren::vk_utils::resizable_buffer::~resizable_buffer() {}

void vren::vk_utils::resizable_buffer::clear()
{
    m_offset = 0;
}

void vren::vk_utils::resizable_buffer::append_data(void const* data, size_t length)
{
    if (length == 0)
    {
        return;
    }

    size_t required_size = m_offset + length;
    if (required_size > m_size)
    {
        size_t alloc_size = (size_t) glm::ceil(required_size / (float) k_block_size) * k_block_size;

        std::shared_ptr<vren::vk_utils::buffer> new_buffer =
            std::make_shared<vren::vk_utils::buffer>(vren::vk_utils::alloc_host_visible_buffer(
                *m_context,
                m_buffer_usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                alloc_size,
                false
            ));

        if (m_buffer)
        {
            vren::vk_utils::copy_buffer(
                *m_context, m_buffer->m_buffer.m_handle, new_buffer->m_buffer.m_handle, m_size, 0, 0
            );
        }

        m_buffer = new_buffer;
        m_size = alloc_size;
    }

    if (data)
    {
        vren::vk_utils::update_host_visible_buffer(*m_context, *m_buffer, data, length, m_offset);
    }

    m_offset += length;
}

void vren::vk_utils::resizable_buffer::set_data(void const* data, size_t length)
{
    clear();
    append_data(data, length);
}
