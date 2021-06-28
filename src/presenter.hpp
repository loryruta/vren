
#pragma once

#include "renderer.hpp"

#include <vector>

#include <vulkan/vulkan.h>

namespace vren
{
	struct surface_details
	{
		VkSurfaceCapabilitiesKHR m_capabilities;
		std::vector<VkPresentModeKHR> m_present_modes;
		std::vector<VkSurfaceFormatKHR> m_surface_formats;
	};

	surface_details get_surface_details(VkSurfaceKHR surface, VkPhysicalDevice physical_device);

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

		void create_swapchain_images();
		void create_swapchain_image_views();
		void create_swapchain_framebuffers();

	public:
		vren::renderer& m_renderer;

		presenter_info m_info;
		VkSurfaceKHR m_surface;
		uint32_t m_present_queue_family_idx = -1;

		VkExtent2D m_current_extent{};
		uint32_t m_image_count;

		VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
		std::vector<VkImage> m_swapchain_images;
		std::vector<VkImageView> m_swapchain_image_views;
		std::vector<VkFramebuffer> m_swapchain_framebuffers;

		std::vector<VkSemaphore> m_image_available_semaphores;

		uint32_t m_current_frame_idx = 0;

		void create_sync_objects();
		void destroy_sync_objects();


		void destroy_swapchain();

		void recreate_swapchain(VkExtent2D extent);

		presenter(vren::renderer& renderer, vren::presenter_info& info, VkSurfaceKHR surface, VkExtent2D initial_extent);
		~presenter();

		void present(vren::render_list const& render_list, vren::camera const& camera);
	};
}

