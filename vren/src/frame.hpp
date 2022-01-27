#pragma once

#include <memory>
#include <vector>
#include <any>

#include <vulkan/vulkan.h>

#include "context.hpp"

namespace vren
{
	class frame
	{
	public:
		VkImage m_image; // Lifetime is externally managed
		VkImageView m_image_view;
		VkFramebuffer m_framebuffer;

		VkCommandBuffer m_command_buffer = VK_NULL_HANDLE;

		VkSemaphore m_image_available_semaphore = VK_NULL_HANDLE;
		VkSemaphore m_render_finished_semaphore = VK_NULL_HANDLE;
		VkFence m_render_finished_fence = VK_NULL_HANDLE;

		std::shared_ptr<vren::context> m_context;

		std::vector<VkDescriptorSet> m_acquired_descriptor_sets;

		/** List of resources used by the current frame. */
		std::vector<std::shared_ptr<void>> m_tracked_resources;

	private:
		void _release_descriptor_sets();

	public:
		explicit frame(
			std::shared_ptr<vren::context> const& ctx,
			VkImage image,
			VkImageView image_view,
			VkFramebuffer framebuffer
		);
		frame(vren::frame&& other) noexcept;
		~frame();

		template<typename _t>
		void track_resource(std::shared_ptr<_t> const& res)
		{
			m_tracked_resources.push_back(std::shared_ptr<_t>(res, nullptr));
		}

		VkDescriptorSet acquire_material_descriptor_set();
		VkDescriptorSet acquire_lights_array_descriptor_set();
	};
}
