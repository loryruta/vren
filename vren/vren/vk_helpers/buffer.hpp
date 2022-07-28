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
	// ------------------------------------------------------------------------------------------------
	// Buffer
	// ------------------------------------------------------------------------------------------------

	struct buffer
	{
		vren::vk_buffer m_buffer;
		vren::vma_allocation m_allocation;
		VmaAllocationInfo m_allocation_info;

		template<typename _t = void>
		inline _t* get_mapped_pointer()
		{
			return reinterpret_cast<_t*>(m_allocation_info.pMappedData);
		}
	};

	// TODO Rename to alloc_host_visible_device_local_buffer
	vren::vk_utils::buffer alloc_host_visible_buffer(
		vren::context const& ctx,
		VkBufferUsageFlags buffer_usage,
		size_t size,
		bool persistently_mapped = false
	);

	// TODO Rename to alloc_device_only_buffer
	vren::vk_utils::buffer alloc_device_only_buffer(
		vren::context const& ctx,
		VkBufferUsageFlags buffer_usage,
		size_t size
	);

	/** 
	 * Allocate a host-only buffer: for sure host visible, possibly host coherent and host cached but not device local.
	 */
	vren::vk_utils::buffer alloc_host_only_buffer(
		vren::context const& context,
		VkBufferUsageFlags buffer_usage,
		VkDeviceSize size,
		bool persistently_mapped = false
	);

	void update_host_visible_buffer(vren::context const& context, vren::vk_utils::buffer& buffer, void const* data, size_t size, size_t dst_offset);
	void update_device_only_buffer(vren::context const& context, vren::vk_utils::buffer& buffer, void const* data, size_t size, size_t dst_offset);

	void copy_buffer(vren::context const& context, VkBuffer src_buffer, VkBuffer dst_buffer, size_t size, size_t src_offset, size_t dst_offset);

	vren::vk_utils::buffer create_device_only_buffer(
		vren::context const& ctx,
		VkBufferUsageFlags buffer_usage,
		void const* data,
		size_t size
	);

	// ------------------------------------------------------------------------------------------------
	// Resizable buffer
	// ------------------------------------------------------------------------------------------------

	class resizable_buffer
	{
	public:
		static constexpr size_t k_block_size = 1024 * 1024; // 1MB

	private:
		vren::context const* m_context;
		VkBufferUsageFlags m_buffer_usage;

	public:
		std::shared_ptr<vren::vk_utils::buffer> m_buffer;
		size_t m_size = 0;
		size_t m_offset = 0;

		resizable_buffer(vren::context const& context, VkBufferUsageFlags buffer_usage);
		resizable_buffer(resizable_buffer const& other) = delete;
		resizable_buffer(resizable_buffer const&& other);
		~resizable_buffer();

		void clear();
		void append_data(void const* data, size_t length);
		void set_data(void const* data, size_t length);
	};
}
