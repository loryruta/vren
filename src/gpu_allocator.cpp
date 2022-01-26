#include "gpu_allocator.hpp"

#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "renderer.hpp"

// References:
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html

vren::vk_utils::buffer::buffer(
	std::shared_ptr<vren::vk_buffer> const& buf,
	std::shared_ptr<vren::vma_allocation> const& alloc
) :
	m_buffer(buf),
	m_allocation(alloc)
{}

vren::vk_utils::buffer vren::vk_utils::alloc_host_visible_buffer(
	std::shared_ptr<vren::renderer> const& renderer,
	VkBufferUsageFlagBits buffer_usage,
	size_t size,
	bool persistently_mapped
)
{
	VkBufferCreateInfo buf_info{};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.usage = buffer_usage;
	buf_info.size = size;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	alloc_info.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	alloc_info.flags = persistently_mapped ? VMA_ALLOCATION_CREATE_MAPPED_BIT : NULL;

	VkBuffer buffer;
	VmaAllocation alloc;
	vren::vk_utils::check(vmaCreateBuffer(renderer->m_vma_allocator, &buf_info, &alloc_info, &buffer, &alloc, nullptr));

	return vren::vk_utils::buffer(
		std::make_shared<vren::vk_buffer>(renderer, buffer),
		std::make_shared<vren::vma_allocation>(renderer, alloc)
	);
}

vren::vk_utils::buffer vren::vk_utils::alloc_device_only_buffer(
	std::shared_ptr<vren::renderer> const& renderer,
	VkBufferUsageFlagBits buffer_usage,
	size_t size
)
{
	VkBufferCreateInfo buf_info{};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.usage = buffer_usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buf_info.size = size;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkBuffer buffer;
	VmaAllocation alloc;
	vren::vk_utils::check(vmaCreateBuffer(renderer->m_vma_allocator, &buf_info, &alloc_info, &buffer, &alloc, nullptr));

	return vren::vk_utils::buffer(
		std::make_shared<vren::vk_buffer>(renderer, buffer),
		std::make_shared<vren::vma_allocation>(renderer, alloc)
	);
}

void vren::vk_utils::update_host_visible_buffer(
	vren::renderer const& renderer,
	vren::vk_utils::buffer& buf,
	void const* data,
	size_t size,
	size_t dst_offset
)
{
	void* mapped_data;

	VmaAllocation alloc = buf.m_allocation->m_handle;

	if (!alloc->IsPersistentMap()) {
		vmaMapMemory(renderer.m_vma_allocator, alloc, &mapped_data);
	} else {
		mapped_data = alloc->GetMappedData();
	}

	std::memcpy(static_cast<uint8_t*>(mapped_data) + dst_offset, data, size);

	if (!alloc->IsPersistentMap()) {
		vmaUnmapMemory(renderer.m_vma_allocator, alloc);
	}

	VkMemoryPropertyFlags mem_flags;
	vmaGetMemoryTypeProperties(renderer.m_vma_allocator, alloc->GetMemoryTypeIndex(), &mem_flags);
	if ((mem_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
		vmaFlushAllocation(renderer.m_vma_allocator, alloc, dst_offset, size);
	}
}

void vren::vk_utils::update_device_only_buffer(
	std::shared_ptr<vren::renderer> const& renderer,
	vren::vk_utils::buffer& buf,
	void const* data,
	size_t size,
	size_t dst_offset
)
{
	vren::vk_utils::buffer staging_buf = alloc_host_visible_buffer(renderer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, false);
	update_host_visible_buffer(*renderer, staging_buf, data, size, 0);

	VkCommandBufferAllocateInfo cmd_buf_alloc_info{};
	cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buf_alloc_info.commandPool = renderer->m_transfer_command_pool;
	cmd_buf_alloc_info.commandBufferCount = 1;

	VkCommandBuffer cmd_buf{};
	vkAllocateCommandBuffers(renderer->m_device, &cmd_buf_alloc_info, &cmd_buf);

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd_buf, &begin_info);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = dst_offset;
	copy_region.size = size;
	vkCmdCopyBuffer(cmd_buf, staging_buf.m_buffer->m_handle, buf.m_buffer->m_handle, 1, &copy_region);

	vkEndCommandBuffer(cmd_buf);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf;
	vkQueueSubmit(renderer->m_queues.at(renderer->m_queue_families.m_transfer_idx), 1, &submit_info, VK_NULL_HANDLE);

	vkQueueWaitIdle(renderer->m_transfer_queue);

	vkResetCommandPool(renderer->m_device, renderer->m_transfer_command_pool, NULL);

	// TODO use copy_buffer(...)
}

void vren::vk_utils::copy_buffer(
	vren::renderer const& renderer,
	vren::vk_utils::buffer& src_buffer,
	vren::vk_utils::buffer& dst_buffer,
	size_t size,
	size_t src_offset,
	size_t dst_offset
)
{
	VkCommandBufferAllocateInfo cmd_buf_alloc_info{};
	cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buf_alloc_info.commandPool = renderer.m_transfer_command_pool;
	cmd_buf_alloc_info.commandBufferCount = 1;

	VkCommandBuffer cmd_buf{};
	vkAllocateCommandBuffers(renderer.m_device, &cmd_buf_alloc_info, &cmd_buf);

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd_buf, &begin_info);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = src_offset;
	copy_region.dstOffset = dst_offset;
	copy_region.size = size;
	vkCmdCopyBuffer(cmd_buf, src_buffer.m_buffer->m_handle, dst_buffer.m_buffer->m_handle, 1, &copy_region);

	vkEndCommandBuffer(cmd_buf);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf;
	vkQueueSubmit(renderer.m_queues.at(renderer.m_queue_families.m_transfer_idx), 1, &submit_info, VK_NULL_HANDLE);

	vkQueueWaitIdle(renderer.m_transfer_queue);

	vkResetCommandPool(renderer.m_device, renderer.m_transfer_command_pool, NULL);
}

vren::vk_utils::buffer vren::vk_utils::create_device_only_buffer(
	std::shared_ptr<vren::renderer> const& renderer,
	VkBufferUsageFlagBits buffer_usage,
	void const* data,
	size_t size
)
{
	vren::vk_utils::buffer buf =
		vren::vk_utils::alloc_device_only_buffer(renderer, buffer_usage, size);
	vren::vk_utils::update_device_only_buffer(renderer, buf, data, size, 0);
	return buf;
}

vren::vk_utils::buffer vren::vk_utils::create_vertex_buffer(
	std::shared_ptr<vren::renderer> const& renderer,
	vren::vertex const* vertices,
	size_t vertices_count
)
{
	return vren::vk_utils::create_device_only_buffer(renderer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices, sizeof(vren::vertex) * vertices_count);
}

vren::vk_utils::buffer vren::vk_utils::create_indices_buffer(
	std::shared_ptr<vren::renderer> const& renderer,
	uint32_t const* indices,
	size_t indices_count
)
{
	return vren::vk_utils::create_device_only_buffer(renderer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices, sizeof(uint32_t) * indices_count);
}

vren::vk_utils::buffer vren::vk_utils::create_instances_buffer(
	std::shared_ptr<vren::renderer> const& renderer,
	vren::instance_data const* instances,
	size_t instances_count
)
{
	return vren::vk_utils::create_device_only_buffer(renderer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, instances, sizeof(vren::instance_data) * instances_count);
}
