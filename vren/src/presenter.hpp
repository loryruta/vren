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
	// Forward decl
	class context;

	// --------------------------------------------------------------------------------------------------------------------------------
	// Swapchain frame data
	// --------------------------------------------------------------------------------------------------------------------------------

	struct swapchain_frame_data
	{
		/* Sync objects */
		vren::vk_semaphore m_image_available_semaphore;
		vren::vk_semaphore m_render_finished_semaphore;

		vren::vk_fence m_frame_fence;

		/** The resource container holds the resources that are in-use by the current frame and
		 * ensures they live enough. */
		vren::resource_container m_resource_container;

		swapchain_frame_data(vren::context const& ctx);
	};

	// --------------------------------------------------------------------------------------------------------------------------------
	// Swapchain
	// --------------------------------------------------------------------------------------------------------------------------------

	class swapchain
	{
	public:
		struct color_buffer
		{
			VkImage m_image; // The image lifetime is bound to the swapchain lifetime
			vren::vk_image_view m_image_view;

			color_buffer(
				VkImage img,
				vren::vk_image_view&& img_view
			) :
				m_image(img),
				m_image_view(std::move(img_view))
			{}
		};

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
		void swap(swapchain& other);

		std::vector<VkImage> get_swapchain_images();
		depth_buffer create_depth_buffer();

		vren::context const* m_context;

	public:
		VkSwapchainKHR m_handle;

		uint32_t m_image_width, m_image_height;
		uint32_t m_image_count;
		VkSurfaceFormatKHR m_surface_format;
		VkPresentModeKHR m_present_mode;

		std::vector<VkImage> m_images;
		std::vector<color_buffer> m_color_buffers;
		std::vector<vren::vk_framebuffer> m_framebuffers;
		depth_buffer m_depth_buffer;

		std::shared_ptr<vren::vk_render_pass> m_render_pass;

		std::vector<vren::swapchain_frame_data> m_frame_data;

		swapchain(
			vren::context const& ctx,
			VkSwapchainKHR handle,
			uint32_t img_width,
			uint32_t img_height,
			uint32_t img_count,
            VkSurfaceFormatKHR surface_format,
            VkPresentModeKHR present_mode,
			std::shared_ptr<vren::vk_render_pass> const& render_pass
		);
		swapchain(swapchain const& other) = delete;
		swapchain(swapchain&& other);
		~swapchain();

		swapchain& operator=(swapchain const& other) = delete;
		swapchain& operator=(swapchain&& other);
	};

	// --------------------------------------------------------------------------------------------------------------------------------
	// Presenter
	// --------------------------------------------------------------------------------------------------------------------------------

	class presenter
	{
	public:
		static constexpr std::initializer_list<char const*> s_extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		using render_func = std::function<void(
			int frame_idx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			vren::render_target const& target
		)>;

	private:
		vren::context const* m_context;

		uint32_t pick_min_image_count(vren::vk_utils::surface_details const& surf_details);
		VkSurfaceFormatKHR pick_surface_format(vren::vk_utils::surface_details const& surf_details);
        VkPresentModeKHR pick_present_mode(vren::vk_utils::surface_details const& surf_details);

	public:
		std::shared_ptr<vren::vk_surface_khr> m_surface;

        uint32_t m_image_count;

		std::shared_ptr<vren::swapchain> m_swapchain;

		uint32_t m_present_queue_family_idx = -1;
		uint32_t m_current_frame_idx = 0;

		presenter(
			vren::context const& ctx,
			std::shared_ptr<vren::vk_surface_khr> const& surface
		);

		void recreate_swapchain(
			uint32_t width,
			uint32_t height,
			std::shared_ptr<vren::vk_render_pass> const& render_pass
		);

		void present(render_func const& render_func);
	};
}

