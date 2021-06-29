
#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "gpu_allocator.hpp"
#include "renderer.hpp"

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html

// ------------------------------------------------------------------------------------------------ gpu_buffer

vren::gpu_buffer::~gpu_buffer()
{
}

// ------------------------------------------------------------------------------------------------ gpu_allocator

vren::gpu_allocator::gpu_allocator(vren::renderer& renderer) :
	m_renderer(renderer)
{
	m_allocator = m_renderer.m_allocator;

	uint32_t transfer_queue_family_idx = m_renderer.m_queue_families.m_transfer_idx;
	m_transfer_queue = m_renderer.m_queues.at(transfer_queue_family_idx);

	VkCommandPoolCreateInfo cmd_pool_create_info{};
	cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_create_info.queueFamilyIndex = transfer_queue_family_idx;
	cmd_pool_create_info.flags = NULL;
	if (vkCreateCommandPool(m_renderer.m_device, &cmd_pool_create_info, nullptr, &m_transfer_cmd_pool) != VK_SUCCESS) {
		throw std::runtime_error("Couldn't create transfer command pool.");
	}
}

vren::gpu_allocator::~gpu_allocator()
{
	vkDestroyCommandPool(m_renderer.m_device, m_transfer_cmd_pool, nullptr);

	m_transfer_queue = VK_NULL_HANDLE;
}

void vren::gpu_allocator::destroy_buffer_if_any(vren::gpu_buffer& buffer)
{
	if (buffer.m_buffer != VK_NULL_HANDLE || buffer.m_allocation != VK_NULL_HANDLE) {
		vmaDestroyBuffer(m_allocator, buffer.m_buffer, buffer.m_allocation);
	}

	buffer.m_buffer = VK_NULL_HANDLE;
	buffer.m_allocation = VK_NULL_HANDLE;
}

void vren::gpu_allocator::alloc_host_visible_buffer(
	vren::gpu_buffer& handle,
	VkBufferUsageFlags buffer_usage,
	size_t size,
	bool persistently_mapped
)
{
	destroy_buffer_if_any(handle);

	VkBufferCreateInfo buf_info{};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.usage = buffer_usage;
	buf_info.size = size;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	alloc_info.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	alloc_info.flags = persistently_mapped ? VMA_ALLOCATION_CREATE_MAPPED_BIT : NULL;

	vmaCreateBuffer(m_allocator, &buf_info, &alloc_info, &handle.m_buffer, &handle.m_allocation, nullptr);
}

void vren::gpu_allocator::update_host_visible_buffer(
	vren::gpu_buffer& handle,
	void const* data,
	size_t size,
	size_t dst_offset
)
{
	void* mapped_data;

	if (!handle.m_allocation->IsPersistentMap()) {
		vmaMapMemory(m_allocator, handle.m_allocation, &mapped_data);
	} else {
		mapped_data = handle.m_allocation->GetMappedData();
	}

	std::memcpy(static_cast<uint8_t*>(mapped_data) + dst_offset, data, size);

	if (!handle.m_allocation->IsPersistentMap()) {
		vmaUnmapMemory(m_allocator, handle.m_allocation);
	}

	VkMemoryPropertyFlags mem_flags;
	vmaGetMemoryTypeProperties(m_allocator, handle.m_allocation->GetMemoryTypeIndex(), &mem_flags);
	if ((mem_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
		vmaFlushAllocation(m_allocator, handle.m_allocation, dst_offset, size);
	}
}

void vren::gpu_allocator::alloc_device_only_buffer(
	vren::gpu_buffer& handle,
	VkBufferUsageFlags buffer_usage,
	size_t size
)
{
	destroy_buffer_if_any(handle);

	VkBufferCreateInfo buf_info{};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.usage = buffer_usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buf_info.size = size;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	if (vmaCreateBuffer(m_allocator, &buf_info, &alloc_info, &handle.m_buffer, &handle.m_allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create buffer.");
	}
}

void vren::gpu_allocator::update_device_only_buffer(
	vren::gpu_buffer& gpu_buffer,
	void const* data,
	size_t size,
	size_t dst_offset
)
{
	vren::gpu_buffer staging_buf{};
	alloc_host_visible_buffer(staging_buf, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, false);
	update_host_visible_buffer(staging_buf, data, size, 0);

	VkCommandBufferAllocateInfo cmd_buf_alloc_info{};
	cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buf_alloc_info.commandPool = m_transfer_cmd_pool;
	cmd_buf_alloc_info.commandBufferCount = 1;

	VkCommandBuffer cmd_buf{};
	vkAllocateCommandBuffers(m_renderer.m_device, &cmd_buf_alloc_info, &cmd_buf);

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd_buf, &begin_info);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = dst_offset;
	copy_region.size = size;
	vkCmdCopyBuffer(cmd_buf, staging_buf.m_buffer, gpu_buffer.m_buffer, 1, &copy_region);

	vkEndCommandBuffer(cmd_buf);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf;
	vkQueueSubmit(m_transfer_queue, 1, &submit_info, VK_NULL_HANDLE);

	vkQueueWaitIdle(m_transfer_queue);

	vkResetCommandPool(m_renderer.m_device, m_transfer_cmd_pool, NULL);

	destroy_buffer_if_any(staging_buf);
}

void vren::gpu_allocator::update_buffer(
	vren::gpu_buffer& gpu_buffer,
	void const* data,
	size_t size,
	size_t dst_offset
)
{
	VkMemoryPropertyFlags memory_flags;
	vmaGetMemoryTypeProperties(m_allocator, gpu_buffer.m_allocation->GetMemoryTypeIndex(), &memory_flags);
	if (memory_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		update_host_visible_buffer(gpu_buffer, data, size, dst_offset);
	} else {
		update_device_only_buffer(gpu_buffer, data, size, dst_offset);
	}
}
