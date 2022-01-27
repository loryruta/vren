#include "presenter.hpp"

#include <iostream>
#include <optional>
#include <chrono>

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

vren::swapchain_framebuffer::swapchain_framebuffer(
	VkImage image,
	vren::vk_image_view&& image_view,
	vren::vk_framebuffer&& fb
) :
	m_image(image),
	m_image_view(std::move(image_view)),
	m_framebuffer(std::move(fb))
{}

VkSwapchainKHR vren::presenter::create_swapchain(VkExtent2D extent)
{
	auto surface_details = vren::get_surface_details(m_surface, m_renderer->m_context->m_physical_device);

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
		for (VkSurfaceFormatKHR surface_format : surface_details.m_surface_formats)
		{
			if (surface_format.format == vren::renderer::k_color_output_format/*&& surface_format.colorSpace == m_info.m_color_space*/)
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

	m_present_queue_family_idx = m_renderer->m_context->m_queue_families.m_graphics_idx;
	VkBool32 has_support;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_renderer->m_context->m_physical_device, m_present_queue_family_idx, m_surface, &has_support);
	if (!has_support)
	{
		throw std::runtime_error("Couldn't find a queue family with present support");
	}

	swapchain_info.preTransform = surface_details.m_capabilities.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; // todo is supported?
	swapchain_info.clipped = VK_TRUE;

	vren::vk_utils::check(vkCreateSwapchainKHR(m_renderer->m_context->m_device, &swapchain_info, nullptr, &m_swapchain));

	m_current_extent = extent;
	vren::vk_utils::check(vkGetSwapchainImagesKHR(m_renderer->m_context->m_device, m_swapchain, &m_image_count, nullptr));

	create_depth_buffer();

	for (int i = 0; i < m_image_count; i++)
	{
		m_image_available_semaphores.emplace_back(
			std::make_shared<vren::vk_semaphore>(
				vren::vk_utils::create_semaphore(m_renderer->m_context)
			)
		);
	}

	_create_frames();
	m_frames.resize(m_image_count);
}

void vren::presenter::_create_frames()
{
	std::vector<VkImage> swapchain_images(m_image_count);
	vren::vk_utils::check(vkGetSwapchainImagesKHR(m_renderer->m_context->m_device, m_swapchain, &m_image_count, swapchain_images.data()));

	for (int i = 0; i < m_image_count; i++)
	{
		VkImage img = swapchain_images.at(i);

		auto img_view = vren::create_image_view(
			m_renderer->m_context,
			img,
			vren::renderer::k_color_output_format,
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		std::vector<VkImageView> attachments = {
			img_view.m_handle,
			m_depth_buffer->m_image_view->m_handle
		};

		auto fb = vren::create_framebuffer(
			m_renderer->m_context,
			m_renderer->m_render_pass,
			attachments,
			m_current_extent
		);

		m_swapchain_framebuffers.emplace_back(img, std::move(img_view), std::move(fb));
	}
}

void vren::presenter::create_depth_buffer()
{
	m_depth_buffer = std::make_shared<vren::presenter::depth_buffer>();

	m_depth_buffer->m_image = std::make_shared<vren::image>();
	vren::create_image(
		m_renderer->m_context,
		m_current_extent.width,
		m_current_extent.height,
		nullptr,
		VK_FORMAT_D32_SFLOAT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		*m_depth_buffer->m_image
	);

	m_depth_buffer->m_image_view = std::make_shared<vren::vk_image_view>(
		vren::create_image_view(
			m_renderer->m_context,
			m_depth_buffer->m_image->m_image->m_handle,
			VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_ASPECT_DEPTH_BIT
		)
	);
}

void vren::presenter::destroy_depth_buffer()
{
	m_depth_buffer.reset();
}

void vren::presenter::destroy_swapchain()
{
	m_frames.clear();

	destroy_depth_buffer();

	if (m_swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_renderer->m_context->m_device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	}
}

void vren::presenter::recreate_swapchain(VkExtent2D extent)
{
	destroy_swapchain();
	create_swapchain(extent);
}

vren::presenter::presenter(
	std::shared_ptr<vren::renderer> const& renderer,
	presenter_info const& info,
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
		vkWaitForFences(m_renderer->m_context->m_device, frame->m_out_fences.size(), frame->m_out_fences.data(), VK_TRUE, UINT64_MAX);
	}

	m_frames.clear();

	destroy_swapchain();

	vkDestroySurfaceKHR(m_renderer->m_context->m_instance, m_surface, nullptr); // TODO should initialize the surface itself
}

void vren::presenter::present(std::function<void(std::unique_ptr<vren::frame>&)> const& frame_func)
{
	VkResult result;

	if (m_frames.at(m_current_frame_idx))
	{
		vren::vk_utils::check(vkWaitForFences(m_renderer->m_context->m_device, m_frames[m_current_frame_idx]->m_out_fences.size(), m_frames[m_current_frame_idx]->m_out_fences.data(), VK_TRUE, UINT64_MAX));
	}

	// Creates a new frame that overlaps the old one at the same index, this will lead to *destroying*
	// all the unused resources in-use for the old frame instance.

	auto& swapchain_fb = m_swapchain_framebuffers.at(m_current_frame_idx);
	auto& frame = m_frames[m_current_frame_idx] = std::make_unique<vren::frame>(
		m_renderer->m_context,
		swapchain_fb.m_image,
		swapchain_fb.m_image_view.m_handle,
		swapchain_fb.m_framebuffer.m_handle
	);

	// Acquires the next image that has to be processed by the current frame in-flight.
	uint32_t image_idx;
	result = vkAcquireNextImageKHR(m_renderer->m_context->m_device, m_swapchain, UINT64_MAX, m_image_available_semaphores[m_current_frame_idx]->m_handle, VK_NULL_HANDLE, &image_idx);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swapchain(m_current_extent); // The swapchain is invalid (due to surface change for example), it needs to be re-created.
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Acquirement of the next swapchain image failed.");
	}

	frame->add_in_semaphore(m_image_available_semaphores[m_current_frame_idx]);

	// Render
	/*
	//vren::vk_utils::check(vkResetFences(m_renderer->m_context->m_device, frame->m_wait_fences.size(), frame->m_wait_fences.data()));
	vren::renderer_target target{};
	target.m_framebuffer = frame->m_framebuffer;
	target.m_render_area.offset = {0, 0};
	target.m_render_area.extent = m_current_extent;
	target.m_viewport = { // todo leave freedom to set viewport outside
		.x = 0,
		.y = (float) m_current_extent.height,
		.width = (float) m_current_extent.width,
		.height = (float) -m_current_extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	m_renderer->render(
		*frame,
		target,
		render_list,
		light_array,
		camera,
		{frame->m_image_available_semaphore},
		frame->m_render_finished_fence
	);
	 */

	frame_func(frame);

	// Present
	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;
	present_info.waitSemaphoreCount = frame->m_out_semaphores.size(); // Waits for the rendering to be finished before presenting
	present_info.pWaitSemaphores = frame->m_out_semaphores.data();
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &m_swapchain;
	present_info.pImageIndices = &image_idx;
	present_info.pResults = nullptr;

	result = vkQueuePresentKHR(m_renderer->m_context->m_queues.at(m_present_queue_family_idx), &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swapchain(m_current_extent);
		return;
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Presentation to swapchain image failed.");
	}

	m_current_frame_idx = (m_current_frame_idx + 1) % m_frames.size();
}
