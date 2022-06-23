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
	};

	// Deprecated
	vren::vk_utils::buffer alloc_host_visible_buffer(
		vren::context const& ctx,
		VkBufferUsageFlags buffer_usage,
		size_t size,
		bool persistently_mapped = false
	);

	// Deprecated
	vren::vk_utils::buffer alloc_device_only_buffer(
		vren::context const& ctx,
		VkBufferUsageFlags buffer_usage,
		size_t size
	);

	vren::vk_utils::buffer alloc_buffer(
		vren::context const& context,
		VkBufferUsageFlags buffer_usage,
		VkDeviceSize size,
		VmaAllocationCreateFlags allocation_flags,
		VkMemoryPropertyFlags memory_property_flags
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
