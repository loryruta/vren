#pragma once

#include <memory>
#include <optional>
#include <functional>

#include "vk_raii.hpp"

namespace vren
{
	// Forward decl
	class context;
	struct vertex;
	struct instance_data;
}

namespace vren::vk_utils
{
	// Forward decl
	class toolbox;

	// --------------------------------------------------------------------------------------------------------------------------------

	struct buffer
	{
		vren::vk_buffer m_buffer;
		vren::vma_allocation m_allocation;
	};

	vren::vk_utils::buffer alloc_host_visible_buffer(
		std::shared_ptr<vren::context> const& ctx,
		VkBufferUsageFlags buffer_usage,
		size_t size,
		bool persistently_mapped = false
	);

	vren::vk_utils::buffer alloc_device_only_buffer(
		std::shared_ptr<vren::context> const& ctx,
		VkBufferUsageFlags buffer_usage,
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
		vren::vk_utils::toolbox const& tb,
		vren::vk_utils::buffer& buf,
		void const* data,
		size_t size,
		size_t dst_offset
	);

	void copy_buffer(
		vren::vk_utils::toolbox const& toolbox,
		VkBuffer src_buf,
		VkBuffer dst_buf,
		size_t size,
		size_t src_offset,
		size_t dst_offset
	);

	vren::vk_utils::buffer create_device_only_buffer(
		vren::vk_utils::toolbox const& tb,
		VkBufferUsageFlags buffer_usage,
		void const* data,
		size_t size
	);

	vren::vk_utils::buffer create_vertex_buffer(
		vren::vk_utils::toolbox const& tb,
		vren::vertex const* vertices,
		size_t vertices_count
	);

	vren::vk_utils::buffer create_indices_buffer(
		vren::vk_utils::toolbox const& tb,
		uint32_t const* indices,
		size_t indices_count
	);

	vren::vk_utils::buffer create_instances_buffer(
		vren::vk_utils::toolbox const& tb,
		vren::instance_data const* instances,
		size_t instances_count
	);

	// ------------------------------------------------------------------------------------------------
	// resizable_buffer
	// ------------------------------------------------------------------------------------------------

	class resizable_buffer
	{
	public:
		static constexpr size_t k_block_size = 1024 * 1024; // 1MB

	private:
		VkBufferUsageFlags m_buffer_usage;

	public:
		vren::vk_utils::toolbox const* m_toolbox;

		std::shared_ptr<vren::vk_utils::buffer> m_buffer;
		size_t m_size = 0;
		size_t m_offset = 0;

		resizable_buffer(
			vren::vk_utils::toolbox const& toolbox,
			VkBufferUsageFlags buffer_usage
		);
		~resizable_buffer();

		void clear();
		void push_value(void const* data, size_t length);
	};
}
