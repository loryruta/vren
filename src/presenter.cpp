#include "presenter.hpp"

#include <iostream>
#include <optional>

vren::surface_details vren::get_surface_details(VkSurfaceKHR surface, VkPhysicalDevice physical_device)
{
	vren::surface_details surface_details{};

	// Capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_details.m_capabilities);

	// Formats
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);

	if (format_count != 0) {
		surface_details.m_surface_formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surface_details.m_surface_formats.data());
	}

	// Present modes
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);

	if (present_mode_count != 0) {
		surface_details.m_present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, surface_details.m_present_modes.data());
	}

	return surface_details;
}

void vren::presenter::create_sync_objects()
{
	m_image_available_semaphores.resize(m_image_count);

	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (int i = 0; i < m_image_count; i++) {
		vkCreateSemaphore(m_renderer.m_device, &semaphore_info, nullptr, &m_image_available_semaphores.at(i));
	}
}

void vren::presenter::destroy_sync_objects()
{
	for (int i = 0; i < m_image_count; i++) {
		vkDestroySemaphore(m_renderer.m_device, m_image_available_semaphores.at(i), nullptr);
	}

	m_image_available_semaphores.clear();
}

VkSwapchainKHR vren::presenter::create_swapchain(VkExtent2D extent)
{
	auto surface_details = vren::get_surface_details(m_surface, m_renderer.m_physical_device);

	uint32_t min_image_count = surface_details.m_capabilities.minImageCount + 1;
	if (surface_details.m_capabilities.maxImageCount > 0 && min_image_count > surface_details.m_capabilities.maxImageCount) {
		min_image_count = surface_details.m_capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchain_info{};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = m_surface;
	swapchain_info.minImageCount = min_image_count;

	// Image format & color space
	{
		std::optional<VkSurfaceFormatKHR> found;
		for (auto surface_format : surface_details.m_surface_formats) {
			if (
				surface_format.format == vren::renderer::m_color_output_format &&
				surface_format.colorSpace == m_info.m_color_space
			) {
				found = surface_format;
				break;
			}
		}

		if (!found.has_value()) {
			throw std::runtime_error("This surface doesn't support the given format & color space.");
		}

		swapchain_info.imageFormat = found->format;
		swapchain_info.imageColorSpace = found->colorSpace;
	}

	// Extent
	swapchain_info.imageExtent = extent;

	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	m_present_queue_family_idx = m_renderer.m_queue_families.m_graphics_idx;
	VkBool32 has_support;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_renderer.m_physical_device, m_present_queue_family_idx, m_surface, &has_support);
	if (!has_support) {
		throw std::runtime_error("Couldn't find a queue family with present support.");
	}

	swapchain_info.preTransform = surface_details.m_capabilities.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; // todo is supported?
	swapchain_info.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(m_renderer.m_device, &swapchain_info, nullptr, &m_swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the swapchain.");
	}

	std::cout << "[presenter] Swapchain created" << std::endl;

	m_current_extent = extent;
	vkGetSwapchainImagesKHR(m_renderer.m_device, m_swapchain, &m_image_count, nullptr); // Gets the actual image count used by the swapchain.

	std::cout << "[presenter] Swapchain image count: " << m_image_count << std::endl;
}

void vren::presenter::create_swapchain_images()
{
	m_swapchain_images.resize(m_image_count);
	vkGetSwapchainImagesKHR(m_renderer.m_device, m_swapchain, &m_image_count, m_swapchain_images.data());
}

void vren::presenter::create_swapchain_image_views()
{
	m_swapchain_image_views.resize(m_image_count);

	for (uint32_t i = 0; i < m_image_count; i++)
	{
		VkImageViewCreateInfo image_view_info{};
		image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_info.image = m_swapchain_images.at(i);
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = vren::renderer::m_color_output_format;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.baseMipLevel = 0;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.baseArrayLayer = 0;
		image_view_info.subresourceRange.layerCount = 1;

		VkImageView image_view;
		if (vkCreateImageView(m_renderer.m_device, &image_view_info, nullptr, &image_view) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create the image view.");
		}
		m_swapchain_image_views[i] = image_view;
	}
}

void vren::presenter::create_swapchain_framebuffers()
{
	m_swapchain_framebuffers.resize(m_image_count);

	for (size_t i = 0; i < m_image_count; i++)
	{
		std::initializer_list<VkImageView> attachments = {
			m_swapchain_image_views.at(i)
		};

		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = m_renderer.m_render_pass;
		framebuffer_info.attachmentCount = (uint32_t)attachments.size();
		framebuffer_info.pAttachments = attachments.begin();
		framebuffer_info.width = m_current_extent.width;
		framebuffer_info.height = m_current_extent.height;
		framebuffer_info.layers = 1;

		if (vkCreateFramebuffer(m_renderer.m_device, &framebuffer_info, nullptr, &m_swapchain_framebuffers.at(i)) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create the framebuffer.");
		}
	}
}

void vren::presenter::destroy_swapchain()
{
	for (auto framebuffer : m_swapchain_framebuffers) {
		vkDestroyFramebuffer(m_renderer.m_device, framebuffer, nullptr);
	}
	//m_swapchain_framebuffers.clear();

	for (auto image_view : m_swapchain_image_views) {
		vkDestroyImageView(m_renderer.m_device, image_view, nullptr);
	}
	//m_swapchain_image_views.clear();

	for (auto image : m_swapchain_images) {
		vkDestroyImage(m_renderer.m_device, image, nullptr);
	}
	//m_swapchain_images.clear();

	if (m_swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(m_renderer.m_device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	}
}

void vren::presenter::recreate_swapchain(VkExtent2D extent)
{
	destroy_swapchain();

	create_swapchain(extent);

 	create_swapchain_images();
	create_swapchain_image_views();
	create_swapchain_framebuffers();
}

vren::presenter::presenter(vren::renderer& renderer, presenter_info& info,VkSurfaceKHR surface, VkExtent2D initial_extent) :
	m_renderer(renderer),
	m_info(info),
	m_surface(surface)
{
	recreate_swapchain(initial_extent);

	create_sync_objects();
}

vren::presenter::~presenter()
{
	destroy_swapchain();
}

void vren::presenter::present(vren::render_list const& render_list, vren::camera const& camera)
{
	VkResult result;

	// Acquires the next image that has to be processed by the current frame in-flight.
	uint32_t image_idx;
	result = vkAcquireNextImageKHR(m_renderer.m_device, m_swapchain, UINT64_MAX, m_image_available_semaphores.at(m_current_frame_idx), VK_NULL_HANDLE, &image_idx);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swapchain(m_current_extent); // The swapchain is invalid (due to surface change for example), it needs to be re-created.
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Acquirement of the next swapchain image failed.");
	}

	// Calls the renderer with the image as a target.
	vren::renderer::target target{};
	target.m_framebuffer = m_swapchain_framebuffers.at(image_idx);
	target.m_render_area.offset = {0, 0};
	target.m_render_area.extent = m_current_extent;

	VkSemaphore render_finished_semaphore = m_renderer.render(image_idx, target, render_list, camera);

	/* Present */
	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_finished_semaphore; // Waits for the rendering to be completed before presenting.

	result = vkQueuePresentKHR(m_renderer.m_queues.at(m_present_queue_family_idx), &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swapchain(m_current_extent);
		return;
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Presentation to swapchain image failed.");
	}

	m_current_frame_idx = (m_current_frame_idx + 1) % m_swapchain_framebuffers.size();
}
