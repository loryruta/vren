#include "presenter.hpp"

#include <iostream>

#include "context.hpp"
#include "toolbox.hpp"
#include "utils/image_layout_transitions.hpp"

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

vren::swapchain::swapchain(
	vren::context const& ctx,
	VkSwapchainKHR handle,
	uint32_t img_width,
	uint32_t img_height,
	uint32_t img_count,
    VkSurfaceFormatKHR surface_format,
    VkPresentModeKHR present_mode,
	std::shared_ptr<vren::vk_render_pass> const& render_pass
) :
	m_context(&ctx),
	m_handle(handle),

	m_image_width(img_width),
	m_image_height(img_height),
    m_image_count(img_count),
    m_surface_format(surface_format),
    m_present_mode(present_mode),
	m_render_pass(render_pass),

	m_images(get_swapchain_images()),
	m_depth_buffer(create_depth_buffer())
{
	/* Create color buffers and framebuffers */
	for (auto img : m_images)
	{
		color_buffer col_buf(img, vren::vk_utils::create_image_view(*m_context, img, m_surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT));

		VkImageView attachments[]{
			col_buf.m_image_view.m_handle,
			m_depth_buffer.m_image_view.m_handle
		};

		VkFramebufferCreateInfo fb_info{};
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass = m_render_pass->m_handle;
		fb_info.attachmentCount = std::size(attachments);
		fb_info.pAttachments = attachments;
		fb_info.width = m_image_width;
		fb_info.height = m_image_height;
		fb_info.layers = 1;

		VkFramebuffer fb_handle;
		vren::vk_utils::check(vkCreateFramebuffer(m_context->m_device, &fb_info, nullptr, &fb_handle));

		m_color_buffers.push_back(std::move(col_buf));
		m_framebuffers.push_back(vren::vk_framebuffer(*m_context, fb_handle));

		m_frame_data.emplace_back(*m_context); // Also initialize a frame data struct for the current swapchain image.
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
	m_color_buffers(std::move(other.m_color_buffers)),
	m_framebuffers(std::move(other.m_framebuffers)),
	m_depth_buffer(std::move(other.m_depth_buffer)),

	m_render_pass(std::move(other.m_render_pass)),

	m_frame_data(std::move(other.m_frame_data))
{
	other.m_handle = VK_NULL_HANDLE;
}

vren::swapchain::~swapchain()
{
	if (m_handle != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_context->m_device, m_handle, nullptr);
	}
}

void vren::swapchain::swap(swapchain& other)
{
	std::swap(m_context, other.m_context);
	std::swap(m_handle, other.m_handle);

	std::swap(m_images, other.m_images);
	std::swap(m_color_buffers, other.m_color_buffers);
	std::swap(m_framebuffers, other.m_framebuffers);
	std::swap(m_depth_buffer, other.m_depth_buffer);

	std::swap(m_image_width, other.m_image_width);
	std::swap(m_image_height, other.m_image_height);
	std::swap(m_image_count, other.m_image_count);
	std::swap(m_surface_format, other.m_surface_format);
	std::swap(m_present_mode, other.m_present_mode);

	std::swap(m_render_pass, other.m_render_pass);

	std::swap(m_frame_data, other.m_frame_data);
}

vren::swapchain& vren::swapchain::operator=(swapchain&& other)
{
	swapchain(std::move(other)).swap(*this);
	return *this;
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


vren::swapchain::depth_buffer vren::swapchain::create_depth_buffer()
{
	auto img = vren::vk_utils::create_image(
		*m_context,
		m_image_width, m_image_height,
        VREN_DEPTH_BUFFER_OUTPUT_FORMAT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

    vren::vk_utils::immediate_graphics_queue_submit(*m_context, [&](VkCommandBuffer cmd_buf, vren::resource_container& res_container)
    {
        vren::vk_utils::transition_image_layout_undefined_to_depth_stencil_attachment(cmd_buf, img.m_image.m_handle);
    });

	return vren::swapchain::depth_buffer(
		std::move(img),
        vren::vk_utils::create_image_view(*m_context, img.m_image.m_handle, VREN_DEPTH_BUFFER_OUTPUT_FORMAT, VK_IMAGE_ASPECT_DEPTH_BIT)
	);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Presenter
// --------------------------------------------------------------------------------------------------------------------------------

vren::presenter::presenter(
	vren::context const& ctx,
	std::shared_ptr<vren::vk_surface_khr> const& surface
) :
	m_context(&ctx),
	m_surface(surface)
{
	m_present_queue_family_idx = ctx.m_queue_families.m_graphics_idx;
	VkBool32 has_support;
	vkGetPhysicalDeviceSurfaceSupportKHR(ctx.m_physical_device, m_present_queue_family_idx, m_surface->m_handle, &has_support);
	if (!has_support) {
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

void vren::presenter::recreate_swapchain(
	uint32_t width,
	uint32_t height,
	std::shared_ptr<vren::vk_render_pass> const& render_pass
)
{
	// If a swapchain was already created, waits for all the frames attached to the old swapchain to end before destroying it.
	if (m_swapchain) {
		for (auto& frame : m_swapchain->m_frame_data) {
			vren::vk_utils::check(vkWaitForFences(m_context->m_device, 1, &frame.m_frame_fence.m_handle, VK_TRUE, UINT64_MAX));
		}
	}

	auto surf_details = vren::vk_utils::get_surface_details(m_context->m_physical_device, m_surface->m_handle);

	uint32_t img_count = pick_min_image_count(surf_details);
	VkSurfaceFormatKHR surf_format = pick_surface_format(surf_details);
    VkPresentModeKHR present_mode = pick_present_mode(surf_details);

	VkSwapchainCreateInfoKHR swapchain_info{};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.pNext = nullptr;
	swapchain_info.flags = NULL;
	swapchain_info.surface = m_surface->m_handle;
	swapchain_info.minImageCount = img_count;
	swapchain_info.imageFormat = surf_format.format;
	swapchain_info.imageColorSpace = surf_format.colorSpace;
	swapchain_info.imageExtent = { width, height };
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_info.queueFamilyIndexCount = NULL;
	swapchain_info.pQueueFamilyIndices = nullptr;
	swapchain_info.preTransform = surf_details.m_capabilities.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = present_mode;
	swapchain_info.clipped = VK_TRUE;
	swapchain_info.oldSwapchain = m_swapchain ? m_swapchain->m_handle : VK_NULL_HANDLE;

	VkSwapchainKHR swapchain;
	vren::vk_utils::check(vkCreateSwapchainKHR(m_context->m_device, &swapchain_info, nullptr, &swapchain));

	m_swapchain = std::make_shared<vren::swapchain>(*m_context, swapchain, width, height, img_count, surf_format, present_mode, render_pass);
}

void vren::presenter::present(render_func const& render_func)
{
	if (!m_swapchain) {
		return;
	}

	VkResult result;
	auto& frame_data = m_swapchain->m_frame_data.at(m_current_frame_idx);

	vren::vk_utils::check(vkWaitForFences(m_context->m_device, 1, &frame_data.m_frame_fence.m_handle, VK_TRUE, UINT64_MAX));
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
	vren::vk_utils::check(vkBeginCommandBuffer(cmd_buf->m_handle, &cmd_buf_begin_info));

	/* Image acquirement */
	uint32_t img_idx;
	result = vkAcquireNextImageKHR(m_context->m_device, m_swapchain->m_handle, UINT64_MAX, frame_data.m_image_available_semaphore.m_handle, VK_NULL_HANDLE, &img_idx);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreate_swapchain(
			m_swapchain->m_image_width,
			m_swapchain->m_image_height,
			m_swapchain->m_render_pass
		);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Acquirement of the next swapchain image failed");
	}

	/* Image layout transition from undefined image layout to color attachment */
	vren::vk_utils::transition_image_layout_undefined_to_color_attachment(cmd_buf->m_handle, m_swapchain->m_images.at(img_idx));

	/* Render pass */
	vren::render_target render_target{
		.m_framebuffer = m_swapchain->m_framebuffers.at(img_idx).m_handle,
		.m_render_area = {
			.offset = {0, 0},
			.extent = {m_swapchain->m_image_width, m_swapchain->m_image_height}
		},
		.m_viewport = {
			.x = 0,
			.y = (float) m_swapchain->m_image_height,
			.width = (float) m_swapchain->m_image_width,
			.height = -((float) m_swapchain->m_image_height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		}
	};
	render_func(
        m_current_frame_idx,
		cmd_buf->m_handle,
		frame_data.m_resource_container,
		render_target
	);

	/* Image layout transition from color attachment to present */
	vren::vk_utils::transition_image_layout_color_attachment_to_present(cmd_buf->m_handle, m_swapchain->m_images.at(img_idx));

	/* Submit */
	vren::vk_utils::check(vkEndCommandBuffer(cmd_buf->m_handle));

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
	vren::vk_utils::check(vkQueueSubmit(m_context->m_graphics_queue, 1, &submit_info, frame_data.m_frame_fence.m_handle));

	/* Present */
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
		recreate_swapchain(
			m_swapchain->m_image_width,
			m_swapchain->m_image_height,
			m_swapchain->m_render_pass
		);
		return;
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present the swapchain image");
	}

	m_current_frame_idx = (m_current_frame_idx + 1) % m_swapchain->m_frame_data.size();
}
