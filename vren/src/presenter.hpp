#pragma once

#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

#include "renderer.hpp"
#include "utils/image.hpp"
#include "utils/misc.hpp"
#include "render_list.hpp"
#include "light_array.hpp"
#include "resource_container.hpp"

namespace vren
{
	// --------------------------------------------------------------------------------------------------------------------------------
	// swapchain_frame
	// --------------------------------------------------------------------------------------------------------------------------------

	class swapchain_frame
	{
	public:
		struct color_buffer
		{
			VkImage m_image; // The image lifetime is managed by the swapchain.
			vren::vk_image_view m_image_view;

			color_buffer(
				VkImage img,
				vren::vk_image_view&& img_view
			) :
				m_image(img),
				m_image_view(std::move(img_view))
			{}
		};

		color_buffer m_color_buffer;
		vren::vk_framebuffer m_framebuffer;

		/* Sync objects */

		vren::vk_semaphore m_image_available_semaphore;
		vren::vk_semaphore m_transited_to_color_attachment_image_layout_semaphore;
		vren::vk_semaphore m_render_finished_semaphore;
		vren::vk_semaphore m_transited_to_present_image_layout_semaphore;

		vren::vk_fence m_frame_fence;

		/** The resource container holds the resources that are in-use by the current frame and
		 * ensures they live enough. */
		vren::resource_container m_resource_container;

		swapchain_frame(
			std::shared_ptr<vren::context> const& ctx,
			color_buffer&& color_buf,
			vren::vk_framebuffer&& fb
		);
	};

	// --------------------------------------------------------------------------------------------------------------------------------
	// swapchain
	// --------------------------------------------------------------------------------------------------------------------------------

	class swapchain
	{
	public:
		struct depth_buffer
		{
			vren::vk_utils::image m_image;
			vren::vk_image_view m_image_view;

			depth_buffer(
				vren::vk_utils::image&& img,
				vren::vk_image_view&& img_view
			) :
				m_image(std::move(img)),
				m_image_view(std::move(img_view))
			{}
		};

	private:
		void _swap(swapchain& other); // Copy-swap idiom

		depth_buffer _create_depth_buffer(uint32_t width, uint32_t height);

		vren::swapchain_frame::color_buffer _create_color_buffer_for_frame(VkImage swapchain_img);
		vren::vk_framebuffer _create_framebuffer_for_frame(
			vren::swapchain_frame::color_buffer const& color_buf,
			uint32_t width,
			uint32_t height,
			VkRenderPass render_pass
		);

	public:
		std::shared_ptr<vren::context> m_context;
		VkSwapchainKHR m_handle;

		depth_buffer m_depth_buffer;

		uint32_t m_image_width, m_image_height;
		std::shared_ptr<vren::vk_render_pass> m_render_pass;

		std::vector<vren::swapchain_frame> m_frames;

		swapchain(
			std::shared_ptr<vren::context> const& ctx,
			VkSwapchainKHR handle,
			uint32_t img_width,
			uint32_t img_height,
			uint32_t img_count,
			std::shared_ptr<vren::vk_render_pass> const& render_pass
		);
		swapchain(swapchain const& other) = delete;
		swapchain(swapchain&& other);
		~swapchain();

		swapchain& operator=(swapchain const& other) = delete;
		swapchain& operator=(swapchain&& other);
	};

	// --------------------------------------------------------------------------------------------------------------------------------
	// presenter
	// --------------------------------------------------------------------------------------------------------------------------------

	class presenter
	{
	public:
		static constexpr std::initializer_list<char const*> s_extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		using render_func = std::function<void(
			int frame_idx,
			vren::resource_container& resource_container, // A storage for all the resources occurred in this rendering operation that shouldn't be destroyed until it completes.
			vren::render_target const& renderer_target,   // The target of the rendering operation (framebuffer, area, scissors and viewport).
			VkSemaphore src_semaphore,                    // The semaphore that has to be waited before starting the rendering operation.
			VkSemaphore dst_semaphore                     // The semaphore that has to be signaled after finishing the rendering operation (restricted to 1).
		)>;

	private:
		uint32_t _pick_min_image_count(vren::vk_utils::surface_details const& surf_details);
		VkSurfaceFormatKHR _pick_surface_format(vren::vk_utils::surface_details const& surf_details);

		VkResult _acquire_swapchain_image(vren::swapchain_frame const& frame, uint32_t* image_idx);
		void _transition_to_color_attachment_image_layout(vren::swapchain_frame const& frame);
		void _transition_to_present_image_layout(vren::swapchain_frame const& frame);
		VkResult _present(vren::swapchain_frame const& frame, uint32_t image_idx);

	public:
		std::shared_ptr<vren::context> m_context;
		VkSurfaceKHR m_surface;

		std::unique_ptr<vren::swapchain> m_swapchain;

		uint32_t m_present_queue_family_idx = -1;

		uint32_t m_current_frame_idx = 0;

		presenter(
			std::shared_ptr<vren::context> const& ctx,
			VkSurfaceKHR surface // The surface ownership is transferred to the presenter!
		);
		~presenter();

		void recreate_swapchain(
			uint32_t width,
			uint32_t height,
			std::shared_ptr<vren::vk_render_pass> const& render_pass
		);

		void present(render_func const& render_func);
	};
}

