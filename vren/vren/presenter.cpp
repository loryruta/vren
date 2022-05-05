#include "presenter.hpp"

#include <iostream>

#include "context.hpp"
#include "toolbox.hpp"
#include "vk_helpers/image_layout_transitions.hpp"
#include "vk_helpers/debug_utils.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Swapchain frame
// --------------------------------------------------------------------------------------------------------------------------------

vren::swapchain_frame_data::swapchain_frame_data(vren::context const& ctx) :
	m_image_available_semaphore(vren::vk_utils::create_semaphore(ctx)),
	m_render_finished_semaphore(vren::vk_utils::create_semaphore(ctx)),
	m_frame_fence(vren::vk_utils::create_fence(ctx, true))
{}

// --------------------------------------------------------------------------------------------------------------------------------
// Swapchain
// --------------------------------------------------------------------------------------------------------------------------------

void vren::swapchain::swap(swapchain& other)
{
	std::swap(m_context, other.m_context);
	std::swap(m_handle, other.m_handle);

	std::swap(m_images, other.m_images);
	std::swap(m_image_views, other.m_image_views);

	std::swap(m_image_width, other.m_image_width);
	std::swap(m_image_height, other.m_image_height);
	std::swap(m_image_count, other.m_image_count);
	std::swap(m_surface_format, other.m_surface_format);
	std::swap(m_present_mode, other.m_present_mode);

	std::swap(m_frame_data, other.m_frame_data);
}

std::vector<VkImage> vren::swapchain::get_swapchain_images()
{
	std::vector<VkImage> images(m_image_count);

	uint32_t img_count;
	vkGetSwapchainImagesKHR(m_context->m_device, m_handle, &img_count, nullptr);
	vkGetSwapchainImagesKHR(m_context->m_device, m_handle, &img_count, images.data());

	if (m_image_count != img_count) {
		throw std::runtime_error("Image count returned by vkGetSwapchainImagesKHR is smaller than the requested image count");
	}

	return images;
}

vren::swapchain::swapchain(
	vren::context const& context,
	VkSwapchainKHR handle,
	uint32_t image_width,
	uint32_t image_height,
	uint32_t image_count,
    VkSurfaceFormatKHR surface_format,
    VkPresentModeKHR present_mode
) :
	m_context(&context),
	m_handle(handle),

	m_image_width(image_width),
	m_image_height(image_height),
    m_image_count(image_count),
    m_surface_format(surface_format),
    m_present_mode(present_mode),

	m_images(get_swapchain_images())
{
	for (VkImage image : m_images)
	{
		auto image_view = vren::vk_utils::create_image_view(*m_context, image, m_surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT);

		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_IMAGE, (uint64_t) image, "swapchain_image");
		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) image_view.m_handle, "swapchain_image_view");

		m_image_views.push_back(std::move(image_view));

		vren::swapchain_frame_data frame_data(*m_context);
		m_frame_data.emplace_back(std::move(frame_data));
	}
}

vren::swapchain::swapchain(swapchain&& other) :
	m_context(other.m_context),
	m_handle(other.m_handle),

	m_image_width(other.m_image_width),
	m_image_height(other.m_image_height),
	m_image_count(other.m_image_count),
	m_surface_format(other.m_surface_format),
	m_present_mode(other.m_present_mode),

	m_images(std::move(other.m_images)),
	m_image_views(std::move(other.m_image_views)),

	m_frame_data(std::move(other.m_frame_data))
{
	other.m_handle = VK_NULL_HANDLE;
}

vren::swapchain::~swapchain()
{
	m_image_views.clear(); // Delete image views before deleting swapchain images!

	if (m_handle != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(m_context->m_device, m_handle, nullptr);
	}
}

vren::swapchain& vren::swapchain::operator=(swapchain&& other)
{
	swapchain(std::move(other)).swap(*this);
	return *this;
}

// --------------------------------------------------------------------------------------------------------------------------------
// Presenter
// --------------------------------------------------------------------------------------------------------------------------------

vren::presenter::presenter(vren::context const& context, vren::vk_surface_khr const& surface, std::function<void(vren::swapchain const& swapchain)> const& swapchain_recreate_callback) :
	m_context(&context),
	m_surface(&surface),
	m_swapchain_recreate_callback(swapchain_recreate_callback)
{
	m_present_queue_family_idx = context.m_queue_families.m_graphics_idx;

	VkBool32 has_support;
	vkGetPhysicalDeviceSurfaceSupportKHR(context.m_physical_device, m_present_queue_family_idx, m_surface->m_handle, &has_support);
	if (!has_support)
	{
		throw std::runtime_error("Unsupported state: the graphics queue family doesn't have the present support");
	}
}

uint32_t vren::presenter::pick_min_image_count(vren::vk_utils::surface_details const& surf_details)
{
	uint32_t img_count = surf_details.m_capabilities.minImageCount + 1;
	return glm::min(img_count, surf_details.m_capabilities.maxImageCount);
}

VkSurfaceFormatKHR vren::presenter::pick_surface_format(vren::vk_utils::surface_details const& surf_details)
{
	for (VkSurfaceFormatKHR surf_format : surf_details.m_surface_formats) {
		if (surf_format.format == VK_FORMAT_B8G8R8A8_UNORM/* && surf_format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR*/) {
			return surf_format;
		}
	}

	throw std::runtime_error("Unsupported format");
}

VkPresentModeKHR vren::presenter::pick_present_mode(vren::vk_utils::surface_details const& surf_details)
{
    auto& present_modes = surf_details.m_present_modes;

    if (std::find(present_modes.begin(), present_modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != present_modes.end()) {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    if (std::find(present_modes.begin(), present_modes.end(), VK_PRESENT_MODE_FIFO_KHR) != present_modes.end()) {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    throw std::runtime_error("Unsupported present modes");
}

void vren::presenter::recreate_swapchain(uint32_t width, uint32_t height)
{
	// If a swapchain was already created, waits for all the frames attached to the old swapchain to end before destroying it.
	if (m_swapchain) {
		for (auto& frame : m_swapchain->m_frame_data) {
			VREN_CHECK(vkWaitForFences(m_context->m_device, 1, &frame.m_frame_fence.m_handle, VK_TRUE, UINT64_MAX), m_context);
		}
	}

	auto surface_details = vren::vk_utils::get_surface_details(m_context->m_physical_device, m_surface->m_handle);

	uint32_t image_count = pick_min_image_count(surface_details);
	VkSurfaceFormatKHR surface_format = pick_surface_format(surface_details);
    VkPresentModeKHR present_mode = pick_present_mode(surface_details);

	VkSwapchainCreateInfoKHR swapchain_info{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = NULL,
		.surface = m_surface->m_handle,
		.minImageCount = image_count,
		.imageFormat = surface_format.format,
		.imageColorSpace = surface_format.colorSpace,
		.imageExtent = { width, height },
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = NULL,
		.pQueueFamilyIndices = nullptr,
		.preTransform = surface_details.m_capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = m_swapchain ? m_swapchain->m_handle : VK_NULL_HANDLE
	};
	VkSwapchainKHR swapchain;
	VREN_CHECK(vkCreateSwapchainKHR(m_context->m_device, &swapchain_info, nullptr, &swapchain), m_context);
	m_swapchain = std::make_unique<vren::swapchain>(*m_context, swapchain, width, height, image_count, surface_format, present_mode);

	m_swapchain_recreate_callback(*m_swapchain);
}

void vren::presenter::present(render_func_t const& render_func)
{
	if (!m_swapchain) {
		return;
	}

	VkResult result;
	auto& frame_data = m_swapchain->m_frame_data.at(m_current_frame_idx);

	VREN_CHECK(vkWaitForFences(m_context->m_device, 1, &frame_data.m_frame_fence.m_handle, VK_TRUE, UINT64_MAX), m_context);
	frame_data.m_resource_container.clear();
	vkResetFences(m_context->m_device, 1, &frame_data.m_frame_fence.m_handle);

	/* Command buffer init */
	auto cmd_buf = std::make_shared<vren::pooled_vk_command_buffer>(
		m_context->m_toolbox->m_graphics_command_pool.acquire()
	);
	frame_data.m_resource_container.add_resource(cmd_buf);

	VkCommandBufferBeginInfo cmd_buf_begin_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr,
	};
	VREN_CHECK(vkBeginCommandBuffer(cmd_buf->m_handle, &cmd_buf_begin_info), m_context);

	/* Image acquirement */
	uint32_t img_idx;
	result = vkAcquireNextImageKHR(m_context->m_device, m_swapchain->m_handle, UINT64_MAX, frame_data.m_image_available_semaphore.m_handle, VK_NULL_HANDLE, &img_idx);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreate_swapchain(m_swapchain->m_image_width, m_swapchain->m_image_height);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Acquirement of the next swapchain image failed");
	}

	// Frame callback
	render_func(m_current_frame_idx, img_idx, *m_swapchain, cmd_buf->m_handle, frame_data.m_resource_container);

	// Submit
	VREN_CHECK(vkEndCommandBuffer(cmd_buf->m_handle), m_context);

	VkPipelineStageFlags wait_dst_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkSubmitInfo submit_info{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame_data.m_image_available_semaphore.m_handle,
		.pWaitDstStageMask = &wait_dst_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd_buf->m_handle,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &frame_data.m_render_finished_semaphore.m_handle,
	};
	VREN_CHECK(vkQueueSubmit(m_context->m_graphics_queue, 1, &submit_info, frame_data.m_frame_fence.m_handle), m_context);

	// Present
	VkPresentInfoKHR present_info{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame_data.m_render_finished_semaphore.m_handle,
		.swapchainCount = 1,
		.pSwapchains = &m_swapchain->m_handle,
		.pImageIndices = &img_idx,
		.pResults = nullptr
	};
	result = vkQueuePresentKHR(m_context->m_queues.at(m_present_queue_family_idx), &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreate_swapchain(m_swapchain->m_image_width, m_swapchain->m_image_height);
		return;
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present the swapchain image");
	}

	m_current_frame_idx = (m_current_frame_idx + 1) % m_swapchain->m_frame_data.size();
}

// --------------------------------------------------------------------------------------------------------------------------------

vren::render_graph::node* vren::blit_color_buffer_to_swapchain_image(
	vren::render_graph::allocator& allocator,
	vren::vk_utils::color_buffer_t const& color_buffer,
	uint32_t width,
	uint32_t height,
	VkImage swapchain_image,
	uint32_t swapchain_image_width,
	uint32_t swapchain_image_height
)
{
	auto node = allocator.allocate();
	node->set_name("blit_color_buffer_to_swapchain_image");
	node->set_src_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->add_image({
		.m_name = "color_buffer",
		.m_image = color_buffer.get_image(),
		.m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
		.m_image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.m_access_flags = VK_ACCESS_TRANSFER_READ_BIT
	});
	node->add_image({
		.m_name = "swapchain_image",
		.m_image = swapchain_image,
		.m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
		.m_image_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.m_access_flags = VK_ACCESS_TRANSFER_WRITE_BIT
	});
	node->set_callback([=, &color_buffer](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkImageBlit image_blit{
			.srcSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
			.srcOffsets = {
				{ 0, 0, 0 },
				{ (int32_t) width, (int32_t) height, 1 },
			},
			.dstSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1, },
			.dstOffsets = {
				{ 0, 0, 0 },
				{ (int32_t) swapchain_image_width, (int32_t) swapchain_image_height, 1 }
			},
		};
		vkCmdBlitImage(command_buffer, color_buffer.get_image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit, VK_FILTER_LINEAR);
	});
	return node;
}

vren::render_graph::node* vren::transit_swapchain_image_to_present_layout(
	vren::render_graph::allocator& allocator,
	VkImage swapchain_image
)
{
	auto node = allocator.allocate();
	node->set_name("transit_swapchain_image_to_present_layout");
	node->set_src_stage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	node->add_image({
		.m_name = "swapchain_image",
		.m_image = swapchain_image,
		.m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
		.m_image_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.m_access_flags = VK_ACCESS_NONE_KHR
	});
	node->set_callback([](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container) {});
	return node;
}
