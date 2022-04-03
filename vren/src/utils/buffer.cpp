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
	VkBufferUsageFlags buffer_usage,
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
	VkBufferUsageFlags buffer_usage,
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

    copy_buffer(tb, staging_buf.m_buffer.m_handle, buf.m_buffer.m_handle, size, 0, dst_offset);
}

void vren::vk_utils::copy_buffer(
	vren::vk_utils::toolbox const& toolbox,
	VkBuffer src_buf,
	VkBuffer dst_buf,
	size_t size,
	size_t src_off,
	size_t dst_off
)
{
    vren::vk_utils::immediate_transfer_queue_submit(toolbox, [&](VkCommandBuffer cmd_buf, vren::resource_container& res_container)
    {
        VkBufferCopy copy_region{
			.srcOffset = src_off,
			.dstOffset = dst_off,
			.size = size
		};
        vkCmdCopyBuffer(cmd_buf, src_buf, dst_buf, 1, &copy_region);
    });
}

vren::vk_utils::buffer vren::vk_utils::create_device_only_buffer(
	vren::vk_utils::toolbox const& tb,
	VkBufferUsageFlags buffer_usage,
	void const* data,
	size_t size
)
{
	auto buf = vren::vk_utils::alloc_device_only_buffer(tb.m_context, buffer_usage, size);
	if (data != nullptr) {
		vren::vk_utils::update_device_only_buffer(tb, buf, data, size, 0);
	}
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
	return vren::vk_utils::create_device_only_buffer(tb, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, instances, sizeof(vren::instance_data) * instances_count);
}

// --------------------------------------------------------------------------------------------------------------------------------
// resizable_buffer
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::resizable_buffer::resizable_buffer(
	vren::vk_utils::toolbox const& toolbox,
	VkBufferUsageFlags buffer_usage
) :
	m_toolbox(&toolbox),
	m_buffer_usage(buffer_usage)
{}

vren::vk_utils::resizable_buffer::~resizable_buffer()
{}

void vren::vk_utils::resizable_buffer::clear()
{
	m_offset = 0;
}

void vren::vk_utils::resizable_buffer::push_value(void const* data, size_t length)
{
	if (length == 0) {
		return;
	}

	size_t req_size = m_offset + length;
	if (req_size > m_size)
	{
		size_t alloc_size = (size_t) glm::ceil(req_size / (float) k_block_size) * k_block_size;

		auto new_buf = std::make_shared<vren::vk_utils::buffer>(
			vren::vk_utils::alloc_host_visible_buffer(
				m_toolbox->m_context,
				m_buffer_usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				alloc_size,
				false
			)
		);

		if (m_buffer) {
			vren::vk_utils::copy_buffer(
				*m_toolbox,
				m_buffer->m_buffer.m_handle,
				new_buf->m_buffer.m_handle,
				m_size,
				0,
				0
			);
		}

		m_buffer = new_buf;
		m_size = alloc_size;
	}

	vren::vk_utils::update_host_visible_buffer(*m_toolbox->m_context, *m_buffer, data, length, m_offset);
	m_offset += length;
}
