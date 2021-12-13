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

VkSwapchainKHR vren::presenter::_create_swapchain(VkExtent2D extent)
{
	auto surface_details = vren::get_surface_details(m_surface, m_renderer->m_physical_device);

	uint32_t min_image_count = surface_details.m_capabilities.minImageCount + 1;
	if (surface_details.m_capabilities.maxImageCount > 0 && min_image_count > surface_details.m_capabilities.maxImageCount)
	{
		min_image_count = surface_details.m_capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchain_info{};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = m_surface;
	swapchain_info.minImageCount = min_image_count;

	// Image format & color space
	{
		std::optional<VkSurfaceFormatKHR> found;
		for (auto surface_format : surface_details.m_surface_formats)
		{
			if (surface_format.format == vren::renderer::m_color_output_format && surface_format.colorSpace == m_info.m_color_space)
			{
				found = surface_format;
				break;
			}
		}

		if (!found.has_value())
		{
			throw std::runtime_error("Surface doesn't support given format and color space");
		}

		swapchain_info.imageFormat = found->format;
		swapchain_info.imageColorSpace = found->colorSpace;
	}

	// Extent
	swapchain_info.imageExtent = extent;

	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	m_present_queue_family_idx = m_renderer->m_queue_families.m_graphics_idx;
	VkBool32 has_support;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_renderer->m_physical_device, m_present_queue_family_idx, m_surface, &has_support);
	if (!has_support)
	{
		throw std::runtime_error("Couldn't find a queue family with present support");
	}

	swapchain_info.preTransform = surface_details.m_capabilities.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; // todo is supported?
	swapchain_info.clipped = VK_TRUE;

	vren::vk_utils::check(vkCreateSwapchainKHR(m_renderer->m_device, &swapchain_info, nullptr, &m_swapchain));

	m_current_extent = extent;
	vren::vk_utils::check(vkGetSwapchainImagesKHR(m_renderer->m_device, m_swapchain, &m_image_count, nullptr));

	_create_depth_buffer();

	_create_frames();
}

void vren::presenter::_create_frames()
{
	std::vector<VkImage> swapchain_images(m_image_count);
	vren::vk_utils::check(vkGetSwapchainImagesKHR(m_renderer->m_device, m_swapchain, &m_image_count, swapchain_images.data()));

	for (int i = 0; i < m_image_count; i++)
	{
		auto& frame = m_frames.emplace_back(*m_renderer);

		// Swapchain image
		frame.m_swapchain_image = swapchain_images.at(i);

		// Swapchain image view
		VkImageViewCreateInfo image_view_info{};
		image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_info.image = swapchain_images.at(i);
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = vren::renderer::m_color_output_format;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.baseMipLevel = 0;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.baseArrayLayer = 0;
		image_view_info.subresourceRange.layerCount = 1;

		vren::vk_utils::check(vkCreateImageView(m_renderer->m_device, &image_view_info, nullptr, &frame.m_swapchain_image_view));

		// Swapchain framebuffer
		std::initializer_list<VkImageView> attachments = {
			frame.m_swapchain_image_view,
			m_depth_buffer.m_image_view.m_handle
		};

		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = m_renderer->m_render_pass;
		framebuffer_info.attachmentCount = (uint32_t) attachments.size();
		framebuffer_info.pAttachments = attachments.begin();
		framebuffer_info.width = m_current_extent.width;
		framebuffer_info.height = m_current_extent.height;
		framebuffer_info.layers = 1;

		vren::vk_utils::check(vkCreateFramebuffer(m_renderer->m_device, &framebuffer_info, nullptr, &frame.m_swapchain_framebuffer));
	}
}

void vren::presenter::_create_depth_buffer()
{
	vren::create_image(
		*m_renderer,
		m_current_extent.width,
		m_current_extent.height,
		nullptr,
		VK_FORMAT_D32_SFLOAT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_depth_buffer.m_image
	);

	vren::create_image_view(
		*m_renderer,
		m_depth_buffer.m_image,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		m_depth_buffer.m_image_view
	);
}

void vren::presenter::_destroy_depth_buffer()
{
	vren::destroy_image(
		*m_renderer,
		m_depth_buffer.m_image
	);

	vren::destroy_image_view(
		*m_renderer,
		m_depth_buffer.m_image_view
	);
}

void vren::presenter::_destroy_swapchain()
{
	m_frames.clear();

	_destroy_depth_buffer();

	if (m_swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_renderer->m_device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	}
}

void vren::presenter::recreate_swapchain(VkExtent2D extent)
{
	_destroy_swapchain();
	_create_swapchain(extent);
}

vren::presenter::presenter(
	std::shared_ptr<vren::renderer>& renderer,
	presenter_info& info,
	VkSurfaceKHR surface,
	VkExtent2D initial_extent
) :
	m_renderer(renderer),
	m_info(info),
	m_surface(surface)
{
	recreate_swapchain(initial_extent);
}

vren::presenter::~presenter()
{
	for (auto& frame : m_frames)
	{
		vkWaitForFences(m_renderer->m_device, 1, &frame.m_render_finished_fence, VK_TRUE, UINT64_MAX);
	}

	_destroy_swapchain();
}

void vren::presenter::present(
	vren::render_list const& render_list,
	vren::lights_array const& light_array,
	vren::camera const& camera
)
{
	VkResult result;

	auto& frame = m_frames.at(m_current_frame_idx);

	vren::vk_utils::check(vkWaitForFences(m_renderer->m_device, 1, &frame.m_render_finished_fence, VK_TRUE, UINT64_MAX));
	frame._on_render();

	// Acquires the next image that has to be processed by the current frame in-flight.
	uint32_t image_idx;
	result = vkAcquireNextImageKHR(m_renderer->m_device, m_swapchain, UINT64_MAX, frame.m_image_available_semaphore, VK_NULL_HANDLE, &image_idx);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swapchain(m_current_extent); // The swapchain is invalid (due to surface change for example), it needs to be re-created.
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Acquirement of the next swapchain image failed.");
	}

	// Render
	vren::renderer::target target{};
	target.m_framebuffer = frame.m_swapchain_framebuffer;
	target.m_render_area.offset = {0, 0};
	target.m_render_area.extent = m_current_extent;
	target.m_viewport = { // todo leave freedom to set viewport outside
		.x = 0,
		.y = (float) m_current_extent.height,
		.width = (float) m_current_extent.width,
		.height = -float(m_current_extent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	vren::vk_utils::check(vkResetFences(m_renderer->m_device, 1, &frame.m_render_finished_fence));

	m_renderer->render(
		frame,
		target,
		render_list,
		light_array,
		camera,
		{frame.m_image_available_semaphore},
		frame.m_render_finished_fence
	);

	// Present
	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &m_swapchain;
	present_info.waitSemaphoreCount = 1; // Waits for the rendering to be finished before presenting
	present_info.pWaitSemaphores = &frame.m_render_finished_semaphore;
	present_info.pImageIndices = &image_idx;

	result = vkQueuePresentKHR(m_renderer->m_queues.at(m_present_queue_family_idx), &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swapchain(m_current_extent);
		return;
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Presentation to swapchain image failed.");
	}

	m_current_frame_idx = (m_current_frame_idx + 1) % m_frames.size();
}
