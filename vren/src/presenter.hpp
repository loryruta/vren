#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "renderer.hpp"
#include "utils/image.hpp"
#include "frame.hpp"
#include "render_list.hpp"
#include "light.hpp"

namespace vren
{
	struct surface_details
	{
		VkSurfaceCapabilitiesKHR m_capabilities;
		std::vector<VkPresentModeKHR> m_present_modes;
		std::vector<VkSurfaceFormatKHR> m_surface_formats;
	};

	surface_details get_surface_details(VkSurfaceKHR surface, VkPhysicalDevice physical_device);

	struct swapchain_framebuffer
	{
		VkImage m_image;
		vren::vk_image_view m_image_view;
		vren::vk_framebuffer m_framebuffer;

		swapchain_framebuffer(
			VkImage image,
			vren::vk_image_view&& image_view,
			vren::vk_framebuffer&& fb
		);
	};

	struct presenter_info
	{
		VkColorSpaceKHR m_color_space;
	};

	class presenter
	{
	public:
		static constexpr std::initializer_list<char const*> s_extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

	private:
		VkSwapchainKHR create_swapchain(VkExtent2D extent);

		void _create_frames();
		void create_depth_buffer();
		void destroy_depth_buffer();
		void destroy_swapchain();

	public:
		std::shared_ptr<vren::renderer> m_renderer;

		presenter_info m_info;
		VkSurfaceKHR m_surface;
		uint32_t m_present_queue_family_idx = -1;

		VkExtent2D m_current_extent{};
		uint32_t m_image_count;

		VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

		struct depth_buffer
		{
			std::shared_ptr<vren::image> m_image;
			std::shared_ptr<vren::vk_image_view> m_image_view;

			inline bool is_valid() const
			{
				return m_image->is_valid() && m_image_view->is_valid();
			}
		};
		std::shared_ptr<vren::presenter::depth_buffer> m_depth_buffer;

		std::vector<swapchain_framebuffer> m_swapchain_framebuffers;

		std::vector<std::unique_ptr<vren::frame>> m_frames;

		uint32_t m_current_frame_idx = 0;

		void recreate_swapchain(VkExtent2D extent);

		presenter(
			std::shared_ptr<vren::renderer> const& renderer,
			vren::presenter_info const& info,
			VkSurfaceKHR surface,
			VkExtent2D initial_extent
		);
		~presenter();

		void present(
			vren::render_list const& render_list,
			vren::lights_array const& light_array,
			vren::camera const& camera
		);
	};
}

