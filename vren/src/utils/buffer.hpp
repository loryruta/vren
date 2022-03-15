#pragma once

#include <memory>
#include <optional>

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
		vren::vk_buffer m_buffer;
		vren::vma_allocation m_allocation;
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

	// --------------------------------------------------------------------------------------------------------------------------------
	// Variable length buffer
	// --------------------------------------------------------------------------------------------------------------------------------

	class var_len_buffer
	{
	private:
		std::shared_ptr<vren::context> m_context;

		std::optional<vren::vk_utils::buffer> m_buffer;
		size_t m_current_buffer_length = 0;

		VkBufferUsageFlagBits m_buffer_usage;

	public:
		var_len_buffer(
			std::shared_ptr<vren::context> const& ctx,
			VkBufferUsageFlagBits buf_usage
		);

		std::optional<vren::vk_utils::buffer> const& get_buffer() const {
			return m_buffer;
		}

		size_t get_buffer_length() const {
			return m_current_buffer_length;
		}

		void set_data(void* data, uint32_t len);

		/** Sets a list of typed elements on a variable length buffer using the following format:
		 * - 4 bytes for the count
		 * - 3 * 4 bytes used as a padding
		 * - the remnant variable bytes are used for the data */
		template<typename _t>
		void set_elements_with_count(_t* elements, uint32_t elements_count);
	};
}
