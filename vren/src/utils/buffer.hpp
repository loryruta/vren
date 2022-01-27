#pragma once

#include <memory>

#include "context.hpp"
#include "vk_raii.hpp"

namespace vren
{
	// Forward decl
	struct vertex;
	struct instance_data;
}

namespace vren::vk_utils
{
	struct buffer
	{
		std::shared_ptr<vren::vk_buffer> m_buffer;
		std::shared_ptr<vren::vma_allocation> m_allocation;

		buffer(
			std::shared_ptr<vren::vk_buffer> const& buf,
			std::shared_ptr<vren::vma_allocation> const& alloc
		);
	};

	vren::vk_utils::buffer alloc_host_visible_buffer(
		std::shared_ptr<vren::context> const& ctx,
		VkBufferUsageFlagBits buffer_usage,
		size_t size,
		bool persistently_mapped = false
	);

	vren::vk_utils::buffer alloc_device_only_buffer(
		std::shared_ptr<vren::context> const& ctx,
		VkBufferUsageFlagBits buffer_usage,
		size_t size
	);

	void update_host_visible_buffer(
		vren::context const& ctx,
		vren::vk_utils::buffer& buf,
		void const* data,
		size_t size,
		size_t dst_offset
	);

	void update_device_only_buffer(
		std::shared_ptr<vren::context> const& ctx,
		vren::vk_utils::buffer& buf,
		void const* data,
		size_t size,
		size_t dst_offset
	);

	void copy_buffer(
		vren::context const& ctx,
		vren::vk_utils::buffer& src_buffer,
		vren::vk_utils::buffer& dst_buffer,
		size_t size,
		size_t src_offset,
		size_t dst_offset
	);

	vren::vk_utils::buffer create_device_only_buffer(
		std::shared_ptr<vren::context> const& ctx,
		VkBufferUsageFlagBits buffer_usage,
		void const* data,
		size_t size
	);

	vren::vk_utils::buffer create_vertex_buffer(
		std::shared_ptr<vren::context> const& ctx,
		vren::vertex const* vertices,
		size_t vertices_count
	);

	vren::vk_utils::buffer create_indices_buffer(
		std::shared_ptr<vren::context> const& ctx,
		uint32_t const* indices,
		size_t indices_count
	);

	vren::vk_utils::buffer create_instances_buffer(
		std::shared_ptr<vren::context> const& ctx,
		vren::instance_data const* instances,
		size_t instances_count
	);
}
