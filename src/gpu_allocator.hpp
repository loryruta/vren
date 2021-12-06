#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace vren
{
	class renderer;

	//

	struct gpu_buffer
	{
		VkBuffer m_buffer = VK_NULL_HANDLE;
		VmaAllocation m_allocation = VK_NULL_HANDLE;

		gpu_buffer() = default;
		~gpu_buffer();
	};

	//

	class gpu_allocator
	{
	public:
		vren::renderer& m_renderer;

		VmaAllocator m_allocator;

		gpu_allocator(vren::renderer& renderer);
		~gpu_allocator();

		void destroy_buffer_if_any(vren::gpu_buffer& buffer);

		void alloc_host_visible_buffer(
			vren::gpu_buffer& buffer,
			VkBufferUsageFlags buffer_usage,
			size_t size,
			bool persistently_mapped = false
		);

		void alloc_device_only_buffer(
			vren::gpu_buffer& buffer,
			VkBufferUsageFlags buffer_usage,
			size_t size
		);

		void update_host_visible_buffer(
			vren::gpu_buffer& buffer,
			void const* data,
			size_t size,
			size_t dst_offset
		);

		void update_device_only_buffer(
			vren::gpu_buffer& buffer,
			void const* data,
			size_t size,
			size_t dst_offset
		);

		void update_buffer(
			vren::gpu_buffer& buffer,
			void const* data,
			size_t size,
			size_t dst_offset
		);
	};
}
