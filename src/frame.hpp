#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace vren
{
	// Forward decl
	class renderer;

	class frame
	{
	public:
		std::vector<VkDescriptorSet> m_acquired_material_descriptor_sets;

		vren::renderer& m_renderer;

		VkImage m_swapchain_image             = VK_NULL_HANDLE;
		VkImageView m_swapchain_image_view    = VK_NULL_HANDLE;
		VkFramebuffer m_swapchain_framebuffer = VK_NULL_HANDLE;

		VkCommandBuffer m_command_buffer = VK_NULL_HANDLE;

		VkSemaphore m_image_available_semaphore = VK_NULL_HANDLE;
		VkSemaphore m_render_finished_semaphore = VK_NULL_HANDLE;
		VkFence m_render_finished_fence         = VK_NULL_HANDLE;

		void _release_material_descriptor_sets();
		VkDescriptorSet acquire_material_descriptor_set();

		explicit frame(vren::renderer& renderer);
		frame(vren::frame&& other) noexcept;
		~frame();

		void _on_render();
	};
}
