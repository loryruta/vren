#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "context.hpp"

namespace vren
{
	class frame
	{
	public:
		std::vector<VkDescriptorSet> m_acquired_descriptor_sets;

		std::shared_ptr<vren::context> m_context;

		VkImage m_swapchain_image = VK_NULL_HANDLE;
		VkImageView m_swapchain_image_view = VK_NULL_HANDLE;
		VkFramebuffer m_swapchain_framebuffer = VK_NULL_HANDLE;

		VkCommandBuffer m_command_buffer = VK_NULL_HANDLE;

		VkSemaphore m_image_available_semaphore = VK_NULL_HANDLE;
		VkSemaphore m_render_finished_semaphore = VK_NULL_HANDLE;
		VkFence m_render_finished_fence = VK_NULL_HANDLE;

		explicit frame(std::shared_ptr<vren::context> const& ctx);
		frame(vren::frame&& other) noexcept;
		~frame();

		void release_descriptor_sets();

		VkDescriptorSet acquire_material_descriptor_set();
		VkDescriptorSet acquire_lights_array_descriptor_set();
	};
}
