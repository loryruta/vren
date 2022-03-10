#include "buffer.hpp"

#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "render_object.hpp"
#include "utils/misc.hpp"

#define VREN_VAR_LEN_BUFFER_INCREMENT 256

// References:
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/quick_start.html

vren::vk_utils::buffer vren::vk_utils::alloc_host_visible_buffer(
	std::shared_ptr<vren::context> const& ctx,
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

	VkBuffer buf;
	VmaAllocation alloc;
	vren::vk_utils::check(vmaCreateBuffer(ctx->m_vma_allocator, &buf_info, &alloc_info, &buf, &alloc, nullptr));

	return vren::vk_utils::buffer{
		.m_buffer = vren::vk_buffer(ctx, buf),
		.m_allocation = vren::vma_allocation(ctx, alloc)
	};
}

vren::vk_utils::buffer vren::vk_utils::alloc_device_only_buffer(
	std::shared_ptr<vren::context> const& ctx,
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

	VkBuffer buf;
	VmaAllocation alloc;
	vren::vk_utils::check(vmaCreateBuffer(ctx->m_vma_allocator, &buf_info, &alloc_info, &buf, &alloc, nullptr));

	return vren::vk_utils::buffer{
		.m_buffer = vren::vk_buffer(ctx, buf),
		.m_allocation = vren::vma_allocation(ctx, alloc)
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

void vren::vk_utils::update_device_only_buffer(
	std::shared_ptr<vren::context> const& ctx,
	vren::vk_utils::buffer& buf,
	void const* data,
	size_t size,
	size_t dst_offset
)
{
	vren::vk_utils::buffer staging_buf = alloc_host_visible_buffer(ctx, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, false);
	update_host_visible_buffer(*ctx, staging_buf, data, size, 0);

	auto cmd_buf = ctx->m_transfer_command_pool->acquire_command_buffer();

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd_buf.m_handle, &begin_info);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = dst_offset;
	copy_region.size = size;
	vkCmdCopyBuffer(cmd_buf.m_handle, staging_buf.m_buffer.m_handle, buf.m_buffer.m_handle, 1, &copy_region);

	vkEndCommandBuffer(cmd_buf.m_handle);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf.m_handle;
	vkQueueSubmit(ctx->m_queues.at(ctx->m_queue_families.m_transfer_idx), 1, &submit_info, VK_NULL_HANDLE);

	vkQueueWaitIdle(ctx->m_transfer_queue);
}

void vren::vk_utils::copy_buffer(
	vren::context const& ctx,
	vren::vk_utils::buffer& src_buffer,
	vren::vk_utils::buffer& dst_buffer,
	size_t size,
	size_t src_offset,
	size_t dst_offset
)
{
	auto cmd_buf = ctx.m_transfer_command_pool->acquire_command_buffer();

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd_buf.m_handle, &begin_info);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = src_offset;
	copy_region.dstOffset = dst_offset;
	copy_region.size = size;
	vkCmdCopyBuffer(cmd_buf.m_handle, src_buffer.m_buffer->m_handle, dst_buffer.m_buffer->m_handle, 1, &copy_region);

	vkEndCommandBuffer(cmd_buf.m_handle);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf.m_handle;
	vkQueueSubmit(ctx.m_queues.at(ctx.m_queue_families.m_transfer_idx), 1, &submit_info, VK_NULL_HANDLE);

	vkQueueWaitIdle(ctx.m_transfer_queue);
}

vren::vk_utils::buffer vren::vk_utils::create_device_only_buffer(
	std::shared_ptr<vren::context> const& ctx,
	VkBufferUsageFlagBits buffer_usage,
	void const* data,
	size_t size
)
{
	vren::vk_utils::buffer buf =
		vren::vk_utils::alloc_device_only_buffer(ctx, buffer_usage, size);
	vren::vk_utils::update_device_only_buffer(ctx, buf, data, size, 0);
	return buf;
}

vren::vk_utils::buffer vren::vk_utils::create_vertex_buffer(
	std::shared_ptr<vren::context> const& ctx,
	vren::vertex const* vertices,
	size_t vertices_count
)
{
	return vren::vk_utils::create_device_only_buffer(ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices, sizeof(vren::vertex) * vertices_count);
}

vren::vk_utils::buffer vren::vk_utils::create_indices_buffer(
	std::shared_ptr<vren::context> const& ctx,
	uint32_t const* indices,
	size_t indices_count
)
{
	return vren::vk_utils::create_device_only_buffer(ctx, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices, sizeof(uint32_t) * indices_count);
}

vren::vk_utils::buffer vren::vk_utils::create_instances_buffer(
	std::shared_ptr<vren::context> const& ctx,
	vren::instance_data const* instances,
	size_t instances_count
)
{
	return vren::vk_utils::create_device_only_buffer(ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, instances, sizeof(vren::instance_data) * instances_count);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Variable length buffer
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::var_len_buffer::var_len_buffer(
	std::shared_ptr<vren::context> const& ctx,
	VkBufferUsageFlagBits buf_usage
) :
	m_context(ctx),
	m_buffer_usage(buf_usage)
{
}

void vren::vk_utils::var_len_buffer::set_data(void* data, uint32_t len)
{
	size_t req_buf_len =
		glm::ceil((float) len / (float) VREN_VAR_LEN_BUFFER_INCREMENT) * VREN_VAR_LEN_BUFFER_INCREMENT;

	if (req_buf_len != m_current_buffer_length)
	{
		m_buffer = vren::vk_utils::alloc_host_visible_buffer(m_context, m_buffer_usage, req_buf_len);

		vren::vk_utils::update_host_visible_buffer(*m_context, *m_buffer, data, req_buf_len, sizeof(uint32_t));

		m_current_buffer_length = req_buf_len;
	}
}

template<typename _t>
void vren::vk_utils::var_len_buffer::set_elements_with_count(_t* elements, uint32_t elements_count)
{
	std::vector<uint8_t> buf_data(
		sizeof(uint32_t) +
			sizeof(uint32_t) * 3 +
			elements_count * sizeof(_t)
	);

	std::memcpy(buf_data.data(), &elements_count, sizeof(uint32_t));
	std::memcpy(buf_data.data() + sizeof(uint32_t) * 4, elements, elements_count * sizeof(_t));

	set_data(buf_data.data(), buf_data.size());
}
