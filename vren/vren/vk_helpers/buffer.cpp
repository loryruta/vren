#include "buffer.hpp"

#include <volk.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "context.hpp"
#include "vk_helpers/misc.hpp"

// References:
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html

vren::vk_utils::buffer vren::vk_utils::alloc_host_visible_buffer(
	vren::context const& ctx,
	VkBufferUsageFlags buffer_usage,
	size_t size,
	bool persistently_mapped
)
{
	assert(size > 0);

	VkBufferCreateInfo buf_info{};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.usage = buffer_usage;
	buf_info.size = size;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	alloc_create_info.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	alloc_create_info.flags = persistently_mapped ? VMA_ALLOCATION_CREATE_MAPPED_BIT : NULL;

	VkBuffer buf;
	VmaAllocation alloc;
	VmaAllocationInfo alloc_info;
	VREN_CHECK(vmaCreateBuffer(ctx.m_vma_allocator, &buf_info, &alloc_create_info, &buf, &alloc, &alloc_info), &ctx);

	return vren::vk_utils::buffer{
		.m_buffer = vren::vk_buffer(ctx, buf),
		.m_allocation = vren::vma_allocation(ctx, alloc),
		.m_allocation_info = alloc_info
	};
}

vren::vk_utils::buffer vren::vk_utils::alloc_device_only_buffer(
	vren::context const& ctx,
	VkBufferUsageFlags buffer_usage,
	size_t size
)
{
	assert(size > 0);

	VkBufferCreateInfo buf_info{};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.usage = buffer_usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buf_info.size = size;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkBuffer buf;
	VmaAllocation alloc;
	VmaAllocationInfo alloc_info;
	VREN_CHECK(vmaCreateBuffer(ctx.m_vma_allocator, &buf_info, &alloc_create_info, &buf, &alloc, &alloc_info), &ctx);

	return vren::vk_utils::buffer{
		.m_buffer = vren::vk_buffer(ctx, buf),
		.m_allocation = vren::vma_allocation(ctx, alloc),
		.m_allocation_info = alloc_info
	};
}

vren::vk_utils::buffer vren::vk_utils::alloc_buffer(
	vren::context const& context,
	VkBufferUsageFlags buffer_usage,
	VkDeviceSize size,
	VmaAllocationCreateFlags allocation_flags,
	VkMemoryPropertyFlags memory_property_flags
)
{
	VkBufferCreateInfo buffer_create_info{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.size = size,
		.usage = buffer_usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};

	VmaAllocationCreateInfo allocation_create_info{
		.flags = allocation_flags,
		.usage = VMA_MEMORY_USAGE_UNKNOWN,
		.requiredFlags = memory_property_flags,
		.preferredFlags = NULL,
		.memoryTypeBits = UINT32_MAX,
		.pool = VK_NULL_HANDLE,
		.pUserData = nullptr,
		.priority = 0.0f
	};


	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;
	VREN_CHECK(vmaCreateBuffer(context.m_vma_allocator, &buffer_create_info, &allocation_create_info, &buffer, &allocation, &allocation_info), &context);

	return vren::vk_utils::buffer{
		.m_buffer = vren::vk_buffer(context, buffer),
		.m_allocation = vren::vma_allocation(context, allocation),
		.m_allocation_info = allocation_info
	};
}

void vren::vk_utils::update_host_visible_buffer(
	vren::context const& ctx,
	vren::vk_utils::buffer& buf,
	void const* data,
	size_t size,
	size_t dst_offset
)
{
	void* mapped_data;

	VmaAllocation alloc = buf.m_allocation.m_handle;

	if (!alloc->IsPersistentMap()) {
		vmaMapMemory(ctx.m_vma_allocator, alloc, &mapped_data);
	} else {
		mapped_data = alloc->GetMappedData();
	}

	std::memcpy(static_cast<uint8_t*>(mapped_data) + dst_offset, data, size);

	if (!alloc->IsPersistentMap()) {
		vmaUnmapMemory(ctx.m_vma_allocator, alloc);
	}

	VkMemoryPropertyFlags mem_flags;
	vmaGetMemoryTypeProperties(ctx.m_vma_allocator, alloc->GetMemoryTypeIndex(), &mem_flags);
	if ((mem_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
		vmaFlushAllocation(ctx.m_vma_allocator, alloc, dst_offset, size);
	}
}

void vren::vk_utils::update_device_only_buffer(vren::context const& context, vren::vk_utils::buffer& buffer, void const* data, size_t size, size_t dst_offset)
{
	vren::vk_utils::buffer staging_buffer = alloc_host_visible_buffer(context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, false);
	update_host_visible_buffer(context, staging_buffer, data, size, 0);

    copy_buffer(context, staging_buffer.m_buffer.m_handle, buffer.m_buffer.m_handle, size, 0, dst_offset);
}

void vren::vk_utils::copy_buffer(vren::context const& context, VkBuffer src_buffer, VkBuffer dst_buffer, size_t size, size_t src_offset, size_t dst_offset)
{
    vren::vk_utils::immediate_transfer_queue_submit(context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
    {
		VkBufferCopy copy_region{ .srcOffset = src_offset, .dstOffset = dst_offset, .size = size };
		vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
		vkCmdSetCheckpointNV(command_buffer, "Buffer copied");
    });
}

vren::vk_utils::buffer vren::vk_utils::create_device_only_buffer(
	vren::context const& ctx,
	VkBufferUsageFlags buffer_usage,
	void const* data,
	size_t size
)
{
	auto buf = vren::vk_utils::alloc_device_only_buffer(ctx, buffer_usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size);
	if (data != nullptr) {
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
{}

vren::vk_utils::resizable_buffer::resizable_buffer(vren::vk_utils::resizable_buffer const&& other) :
	m_context(other.m_context),
	m_buffer_usage(other.m_buffer_usage),
	m_buffer(std::move(other.m_buffer)),
	m_size(other.m_size),
	m_offset(other.m_offset)
{
}

vren::vk_utils::resizable_buffer::~resizable_buffer()
{}

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

		std::shared_ptr<vren::vk_utils::buffer> new_buffer = std::make_shared<vren::vk_utils::buffer>(
			vren::vk_utils::alloc_host_visible_buffer(*m_context, m_buffer_usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, alloc_size, false)
		);

		if (m_buffer) {
			vren::vk_utils::copy_buffer(*m_context, m_buffer->m_buffer.m_handle, new_buffer->m_buffer.m_handle, m_size, 0, 0);
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
