#pragma once

#include <vector>
#include <functional>

#include <volk.h>

#include "renderer.hpp"
#include "vk_helpers/image.hpp"
#include "vk_helpers/misc.hpp"
#include "light.hpp"
#include "base/resource_container.hpp"

namespace vren
{
	// Forward decl
	class context;

	// --------------------------------------------------------------------------------------------------------------------------------
	// Swapchain frame data
	// --------------------------------------------------------------------------------------------------------------------------------

	struct swapchain_frame_data
	{
		vren::vk_semaphore m_image_available_semaphore;
		vren::vk_semaphore m_render_finished_semaphore;

		vren::vk_fence m_frame_fence;

		/**
		 * The resource container holds the resources that are in-use by the frame and ensures they live enough.
		 */
		vren::resource_container m_resource_container;

		swapchain_frame_data(vren::context const& context);
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

			color_buffer(VkImage image, vren::vk_image_view&& image_view) :
				m_image(image),
				m_image_view(std::move(image_view))
			{}
		};

		struct depth_buffer
		{
			vren::vk_utils::image m_image;
			vren::vk_image_view m_image_view;

			depth_buffer(vren::vk_utils::image&& image, vren::vk_image_view&& image_view) :
				m_image(std::move(image)),
				m_image_view(std::move(image_view))
			{}
		};

	private:
		vren::context const* m_context;

		void swap(swapchain& other);

		std::vector<VkImage> get_swapchain_images();
		depth_buffer create_depth_buffer();

	public:
		VkSwapchainKHR m_handle;

		uint32_t m_image_width, m_image_height;
		uint32_t m_image_count;
		VkSurfaceFormatKHR m_surface_format;
		VkPresentModeKHR m_present_mode;

		std::vector<VkImage> m_images;
		std::vector<color_buffer> m_color_buffers;
		depth_buffer m_depth_buffer;

		std::vector<vren::swapchain_frame_data> m_frame_data;

		swapchain(
			vren::context const& context,
			VkSwapchainKHR handle,
			uint32_t image_width,
			uint32_t image_height,
			uint32_t image_count,
            VkSurfaceFormatKHR surface_format,
            VkPresentModeKHR present_mode
		);
		swapchain(swapchain const& other) = delete;
		swapchain(swapchain&& other);
		~swapchain();

		swapchain& operator=(swapchain const& other) = delete;
		swapchain& operator=(swapchain&& other);
	};

	// --------------------------------------------------------------------------------------------------------------------------------
	// Swapchain framebuffer
	// --------------------------------------------------------------------------------------------------------------------------------

	class swapchain_framebuffer
	{
	private:
		vren::context const* m_context;
		VkRenderPass m_render_pass;
		std::vector<vren::vk_framebuffer> m_framebuffers;

	public:
		swapchain_framebuffer(vren::context const& context, VkRenderPass render_pass);

		void on_swapchain_recreate(vren::swapchain const& swapchain);

		VkFramebuffer get_framebuffer(uint32_t swapchain_image_idx) const;
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

	private:
		vren::context const* m_context;

		uint32_t pick_min_image_count(vren::vk_utils::surface_details const& surf_details);
		VkSurfaceFormatKHR pick_surface_format(vren::vk_utils::surface_details const& surf_details);
        VkPresentModeKHR pick_present_mode(vren::vk_utils::surface_details const& surf_details);

	public:
		std::shared_ptr<vren::vk_surface_khr> m_surface;
		std::function<void(vren::swapchain const& swapchain)> m_swapchain_recreate_callback;

        uint32_t m_image_count;

		std::shared_ptr<vren::swapchain> m_swapchain;

		uint32_t m_present_queue_family_idx = -1;
		uint32_t m_current_frame_idx = 0;

		presenter(
			vren::context const& context,
			std::shared_ptr<vren::vk_surface_khr> const& surface,
			std::function<void(vren::swapchain const& swapchain)> const& swapchain_recreate_callback // When the swapchain is re-created the user is interested in re-creating the attached framebuffers
		);

		void recreate_swapchain(uint32_t width, uint32_t height);

		using render_func_t = std::function<void(
			uint32_t frame_idx,
			uint32_t swapchain_image_idx,
			vren::swapchain const& swapchain,
			VkCommandBuffer command_buffer,
			vren::resource_container& resource_container
		)>;
		void present(render_func_t const& render_func);
	};
}

