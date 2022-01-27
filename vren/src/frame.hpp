#pragma once

#include <memory>
#include <vector>
#include <any>

#include <vulkan/vulkan.h>

#include "context.hpp"
#include "utils/misc.hpp"

namespace vren
{
	class frame
	{
	public:
		std::shared_ptr<vren::context> m_context;

		VkImage m_image; // Lifetime is externally managed
		VkImageView m_image_view;
		VkFramebuffer m_framebuffer;

		std::vector<VkSemaphore> m_in_semaphores;
		std::vector<VkSemaphore> m_out_semaphores;
		std::vector<VkFence> m_out_fences;

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

		void acquire_command_buffer();

		/** Adds a semaphore that has to be waited when entering the frame. */
		void add_in_semaphore(std::shared_ptr<vren::vk_semaphore> const& semaphore);

		/** Adds a semaphore that has to be waited when exiting the frame. */
		void add_out_semaphore(std::shared_ptr<vren::vk_semaphore> const& semaphore);

		/** Adds a fence that has to be waited when exiting the frame. */
		void add_out_fence(std::shared_ptr<vren::vk_fence> const& fence);

		VkDescriptorSet acquire_material_descriptor_set();
		VkDescriptorSet acquire_lights_array_descriptor_set();
	};
}
