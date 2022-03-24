#include "buffer.hpp"

#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "context.hpp"
#include "vk_toolbox.hpp"
#include "render_object.hpp"
#include "utils/misc.hpp"

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
	vren::vk_utils::toolbox const& tb,
	vren::vk_utils::buffer& buf,
	void const* data,
	size_t size,
	size_t dst_offset
)
{
	auto& ctx = tb.m_context;

	vren::vk_utils::buffer staging_buf = alloc_host_visible_buffer(ctx, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, false);
	update_host_visible_buffer(*ctx, staging_buf, data, size, 0);

    copy_buffer(tb, staging_buf, buf, size, 0, dst_offset);
}

void vren::vk_utils::copy_buffer(
	vren::vk_utils::toolbox const& tb,
	vren::vk_utils::buffer& src_buffer,
	vren::vk_utils::buffer& dst_buffer,
	size_t size,
	size_t src_offset,
	size_t dst_offset
)
{
    vren::vk_utils::immediate_transfer_queue_submit(tb, [&](VkCommandBuffer cmd_buf, vren::resource_container& res_container)
    {
        VkBufferCopy copy_region{};
        copy_region.srcOffset = src_offset;
        copy_region.dstOffset = dst_offset;
        copy_region.size = size;
        vkCmdCopyBuffer(cmd_buf, src_buffer.m_buffer.m_handle, dst_buffer.m_buffer.m_handle, 1, &copy_region);
    });
}

vren::vk_utils::buffer vren::vk_utils::create_device_only_buffer(
	vren::vk_utils::toolbox const& tb,
	VkBufferUsageFlagBits buffer_usage,
	void const* data,
	size_t size
)
{
	auto buf = vren::vk_utils::alloc_device_only_buffer(tb.m_context, buffer_usage, size);
	vren::vk_utils::update_device_only_buffer(tb, buf, data, size, 0);
	return buf;
}

vren::vk_utils::buffer vren::vk_utils::create_vertex_buffer(
	vren::vk_utils::toolbox const& tb,
	vren::vertex const* vertices,
	size_t vertices_count
)
{
	return vren::vk_utils::create_device_only_buffer(tb, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices, sizeof(vren::vertex) * vertices_count);
}

vren::vk_utils::buffer vren::vk_utils::create_indices_buffer(
	vren::vk_utils::toolbox const& tb,
	uint32_t const* indices,
	size_t indices_count
)
{
	return vren::vk_utils::create_device_only_buffer(tb, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices, sizeof(uint32_t) * indices_count);
}

vren::vk_utils::buffer vren::vk_utils::create_instances_buffer(
	vren::vk_utils::toolbox const& tb,
	vren::instance_data const* instances,
	size_t instances_count
)
{
	return vren::vk_utils::create_device_only_buffer(tb, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, instances, sizeof(vren::instance_data) * instances_count);
}
