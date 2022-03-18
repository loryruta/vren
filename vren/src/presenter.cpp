#include "presenter.hpp"

#include <iostream>

#include "utils/image_layout_transitions.hpp"

vren::swapchain_frame::swapchain_frame(
	std::shared_ptr<vren::context> const& ctx,
	color_buffer&& color_buf,
	vren::vk_framebuffer&& fb
) :
	m_color_buffer(std::move(color_buf)),
	m_framebuffer(std::move(fb)),

	m_image_available_semaphore(vren::vk_utils::create_semaphore(ctx)),
	m_transited_to_color_attachment_image_layout_semaphore(vren::vk_utils::create_semaphore(ctx)),
	m_render_finished_semaphore(vren::vk_utils::create_semaphore(ctx)),
	m_transited_to_present_image_layout_semaphore(vren::vk_utils::create_semaphore(ctx)),
	m_frame_fence(vren::vk_utils::create_fence(ctx, true))
{}

// --------------------------------------------------------------------------------------------------------------------------------
// swapchain
// --------------------------------------------------------------------------------------------------------------------------------

vren::swapchain::swapchain(
	std::shared_ptr<vren::context> const& ctx,
	VkSwapchainKHR handle,
	uint32_t img_width,
	uint32_t img_height,
	uint32_t img_count,
    VkSurfaceFormatKHR surface_format,
    VkPresentModeKHR present_mode,
	std::shared_ptr<vren::vk_render_pass> const& render_pass
) :
	m_context(ctx),
	m_handle(handle),
	m_depth_buffer(_create_depth_buffer(img_width, img_height)),
	m_image_width(img_width),
	m_image_height(img_height),
    m_image_count(img_count),
    m_surface_format(surface_format),
    m_present_mode(present_mode),
	m_render_pass(render_pass)
{
	std::vector<VkImage> swapchain_images(img_count);
	vren::vk_utils::check(vkGetSwapchainImagesKHR(m_context->m_device, m_handle, &img_count, swapchain_images.data()));

	for (VkImage swapchain_img : swapchain_images)
	{
		auto color_buf = _create_color_buffer_for_frame(swapchain_img);
		auto fb = _create_framebuffer_for_frame(color_buf, img_width, img_height, render_pass->m_handle);

		m_frames.emplace_back(m_context, std::move(color_buf), std::move(fb));
	}
}

vren::swapchain::swapchain(swapchain&& other) :
	m_context(other.m_context),
	m_handle(other.m_handle),
	m_depth_buffer(std::move(other.m_depth_buffer)),
	m_frames(std::move(other.m_frames))
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

vren::swapchain& vren::swapchain::operator=(swapchain&& other)
{
	swapchain(std::move(other))._swap(*this);
	return *this;
}

void vren::swapchain::_swap(swapchain& other)
{
	std::swap(m_context, other.m_context);
	std::swap(m_handle, other.m_handle);
	std::swap(m_depth_buffer, other.m_depth_buffer);
	std::swap(m_frames, m_frames);
}

vren::swapchain::depth_buffer
vren::swapchain::_create_depth_buffer(uint32_t width, uint32_t height)
{
	auto img = vren::vk_utils::create_image(
		m_context,
		width, height,
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
        vren::vk_utils::create_image_view(m_context, img.m_image.m_handle, VREN_DEPTH_BUFFER_OUTPUT_FORMAT, VK_IMAGE_ASPECT_DEPTH_BIT)
	);
}

vren::swapchain_frame::color_buffer
vren::swapchain::_create_color_buffer_for_frame(VkImage swapchain_img)
{
	return vren::swapchain_frame::color_buffer(
        swapchain_img,
        vren::vk_utils::create_image_view(m_context, swapchain_img, m_surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT)
    );
}

vren::vk_framebuffer vren::swapchain::_create_framebuffer_for_frame(
	vren::swapchain_frame::color_buffer const& color_buf,
	uint32_t width,
	uint32_t height,
	VkRenderPass render_pass
)
{
	VkImageView attachments[]{
		color_buf.m_image_view.m_handle,
		m_depth_buffer.m_image_view.m_handle
	};

	VkFramebufferCreateInfo fb_info{};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.renderPass = render_pass;
	fb_info.attachmentCount = std::size(attachments);
	fb_info.pAttachments = attachments;
	fb_info.width = width;
	fb_info.height = height;
	fb_info.layers = 1;

	VkFramebuffer fb;
	vren::vk_utils::check(vkCreateFramebuffer(m_context->m_device, &fb_info, nullptr, &fb));
	return vren::vk_framebuffer(m_context, fb);
}

// --------------------------------------------------------------------------------------------------------------------------------
// presenter
// --------------------------------------------------------------------------------------------------------------------------------

vren::presenter::presenter(
	std::shared_ptr<vren::context> const& ctx,
	VkSurfaceKHR surface
) :
	m_context(ctx),
	m_surface(surface)
{
	m_present_queue_family_idx = m_context->m_queue_families.m_graphics_idx;
	VkBool32 has_support;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_context->m_physical_device, m_present_queue_family_idx, m_surface, &has_support);
	if (!has_support) {
		throw std::runtime_error("Unsupported state: the graphics queue family doesn't have the present support");
	}
}

vren::presenter::~presenter()
{
	vkDestroySurfaceKHR(m_context->m_instance, m_surface, nullptr);
}

uint32_t vren::presenter::_pick_min_image_count(vren::vk_utils::surface_details const& surf_details)
{
	uint32_t img_count = surf_details.m_capabilities.minImageCount + 1;
	return glm::min(img_count, surf_details.m_capabilities.maxImageCount);
}

VkSurfaceFormatKHR vren::presenter::_pick_surface_format(vren::vk_utils::surface_details const& surf_details)
{
	for (VkSurfaceFormatKHR surf_format : surf_details.m_surface_formats) {
		if (surf_format.format == VK_FORMAT_B8G8R8A8_UNORM/* && surf_format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR*/) {
			return surf_format;
		}
	}

	throw std::runtime_error("Unsupported format");
}

VkPresentModeKHR vren::presenter::_pick_present_mode(vren::vk_utils::surface_details const& surf_details)
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

VkResult vren::presenter::_acquire_swapchain_image(vren::swapchain_frame const& frame, uint32_t* image_idx)
{
	return vkAcquireNextImageKHR(m_context->m_device, m_swapchain->m_handle, UINT64_MAX, frame.m_image_available_semaphore.m_handle, VK_NULL_HANDLE, image_idx);
}

void
vren::presenter::_transition_to_color_attachment_image_layout(vren::swapchain_frame& frame)
{
	auto cmd_buf = std::make_shared<vren::pooled_vk_command_buffer>(
		m_context->m_graphics_command_pool->acquire()
	);
	frame.m_resource_container.add_resource(cmd_buf);

    /* Command buffer begin */
    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vren::vk_utils::check(vkBeginCommandBuffer(cmd_buf->m_handle, &begin_info));

    /* Recording */
	vren::vk_utils::transition_image_layout_undefined_to_color_attachment(cmd_buf->m_handle, frame.m_color_buffer.m_image);

	/* Command buffer end */
	vkEndCommandBuffer(cmd_buf->m_handle);

    /* Submission */
	VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &frame.m_image_available_semaphore.m_handle;
	submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf->m_handle;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &frame.m_transited_to_color_attachment_image_layout_semaphore.m_handle;

	vren::vk_utils::check(vkQueueSubmit(m_context->m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
}

void
vren::presenter::_transition_to_present_image_layout(vren::swapchain_frame& frame)
{
	auto cmd_buf = std::make_shared<vren::pooled_vk_command_buffer>(
		m_context->m_graphics_command_pool->acquire()
	);
	frame.m_resource_container.add_resource(cmd_buf);

    /* Command buffer begin */
    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vren::vk_utils::check(vkBeginCommandBuffer(cmd_buf->m_handle, &begin_info));

    /* Recording */
	vren::vk_utils::transition_image_layout_color_attachment_to_present(cmd_buf->m_handle, frame.m_color_buffer.m_image);

	/* Command buffer end */
	vkEndCommandBuffer(cmd_buf->m_handle);

    /* Submission */
	VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &frame.m_render_finished_semaphore.m_handle;
	submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf->m_handle;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &frame.m_transited_to_present_image_layout_semaphore.m_handle;

	vren::vk_utils::check(vkQueueSubmit(m_context->m_graphics_queue, 1, &submit_info, frame.m_frame_fence.m_handle));
}

VkResult vren::presenter::_present(vren::swapchain_frame const& frame, uint32_t image_idx)
{
	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &frame.m_transited_to_present_image_layout_semaphore.m_handle;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &m_swapchain->m_handle;
	present_info.pImageIndices = &image_idx;
	present_info.pResults = nullptr;

	return vkQueuePresentKHR(m_context->m_queues.at(m_present_queue_family_idx), &present_info);
}

void vren::presenter::recreate_swapchain(
	uint32_t width,
	uint32_t height,
	std::shared_ptr<vren::vk_render_pass> const& render_pass
)
{
	auto surf_details = vren::vk_utils::get_surface_details(m_context->m_physical_device, m_surface);

	uint32_t img_count = _pick_min_image_count(surf_details);
	VkSurfaceFormatKHR surf_format = _pick_surface_format(surf_details);
    VkPresentModeKHR present_mode = _pick_present_mode(surf_details);

	VkSwapchainCreateInfoKHR swapchain_info{};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.pNext = nullptr;
	swapchain_info.flags = NULL;
	swapchain_info.surface = m_surface;
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

	m_swapchain = std::make_unique<vren::swapchain>(m_context, swapchain, width, height, img_count, surf_format, present_mode, render_pass);
}

void vren::presenter::present(render_func const& render_fn)
{
	if (!m_swapchain) {
		return;
	}

	VkResult result;
	auto& frame = m_swapchain->m_frames.at(m_current_frame_idx);

	vren::vk_utils::check(vkWaitForFences(m_context->m_device, 1, &frame.m_frame_fence.m_handle, VK_TRUE, UINT64_MAX));
	frame.m_resource_container.clear();

	vkResetFences(m_context->m_device, 1, &frame.m_frame_fence.m_handle);

	/* Image acquirement */
	uint32_t image_idx;
	result = _acquire_swapchain_image(frame, &image_idx);
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

	/* Image layout transition undefined to color attachment */
	_transition_to_color_attachment_image_layout(frame);

	/* Render pass */
	vren::render_target render_target{
		.m_framebuffer = frame.m_framebuffer.m_handle,
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

    render_fn(
        m_current_frame_idx,
		frame.m_resource_container,
		render_target,
		frame.m_transited_to_color_attachment_image_layout_semaphore.m_handle,
		frame.m_render_finished_semaphore.m_handle
	);

	/* Image layout transition color attachment to present */
	_transition_to_present_image_layout(frame);

	/* Present */
	result = _present(frame, image_idx);
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
		throw std::runtime_error("Presentation of the image to swapchain failed");
	}

	m_current_frame_idx = (m_current_frame_idx + 1) % m_swapchain->m_frames.size();
}
