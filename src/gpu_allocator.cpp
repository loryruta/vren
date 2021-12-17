
#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "gpu_allocator.hpp"
#include "renderer.hpp"

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html

// ------------------------------------------------------------------------------------------------ gpu_buffer

vren::gpu_buffer::gpu_buffer(vren::gpu_buffer&& other)
{
	*this = std::move(other);
}

vren::gpu_buffer::~gpu_buffer()
{
	// TODO better to destroy here
}

vren::gpu_buffer& vren::gpu_buffer::operator=(vren::gpu_buffer&& other) noexcept
{
	m_buffer = other.m_buffer;
	m_allocation = other.m_allocation;

	other.m_buffer = VK_NULL_HANDLE;
	other.m_allocation = VK_NULL_HANDLE;

	return *this;
}

// ------------------------------------------------------------------------------------------------ gpu_allocator

vren::gpu_allocator::gpu_allocator(vren::renderer& renderer) :
	m_renderer(renderer)
{
	VmaAllocatorCreateInfo create_info{};
	//create_info.vulkanApiVersion = VK_API_VERSION_1_2;
	create_info.instance = renderer.m_instance;
	create_info.physicalDevice = renderer.m_physical_device;
	create_info.device = renderer.m_device;

	vren::vk_utils::check(vmaCreateAllocator(&create_info, &m_allocator));
}

vren::gpu_allocator::~gpu_allocator()
{
	vmaDestroyAllocator(m_allocator);
}

void vren::gpu_allocator::destroy_buffer_if_any(vren::gpu_buffer& buffer)
{
	if (buffer.is_valid())
	{
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
	cmd_buf_alloc_info.commandPool = m_renderer.m_transfer_command_pool;
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
	vkQueueSubmit(m_renderer.m_queues.at(m_renderer.m_queue_families.m_transfer_idx), 1, &submit_info, VK_NULL_HANDLE);

	vkQueueWaitIdle(m_renderer.m_transfer_queue);

	vkResetCommandPool(m_renderer.m_device, m_renderer.m_transfer_command_pool, NULL);

	destroy_buffer_if_any(staging_buf);

	// TODO use copy_buffer(...)
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

void vren::gpu_allocator::copy_buffer(
	vren::gpu_buffer& src_buffer,
	vren::gpu_buffer& dst_buffer,
	size_t size,
	size_t src_offset,
	size_t dst_offset
)
{
	VkCommandBufferAllocateInfo cmd_buf_alloc_info{};
	cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buf_alloc_info.commandPool = m_renderer.m_transfer_command_pool;
	cmd_buf_alloc_info.commandBufferCount = 1;

	VkCommandBuffer cmd_buf{};
	vkAllocateCommandBuffers(m_renderer.m_device, &cmd_buf_alloc_info, &cmd_buf);

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd_buf, &begin_info);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = src_offset;
	copy_region.dstOffset = dst_offset;
	copy_region.size = size;
	vkCmdCopyBuffer(cmd_buf, src_buffer.m_buffer, dst_buffer.m_buffer, 1, &copy_region);

	vkEndCommandBuffer(cmd_buf);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf;
	vkQueueSubmit(m_renderer.m_queues.at(m_renderer.m_queue_families.m_transfer_idx), 1, &submit_info, VK_NULL_HANDLE);

	vkQueueWaitIdle(m_renderer.m_transfer_queue);

	vkResetCommandPool(m_renderer.m_device, m_renderer.m_transfer_command_pool, NULL);
}
