#include "dbg_renderer.hpp"

#include "utils/buffer.hpp"
#include "utils/misc.hpp"

vren::dbg_renderer::dbg_renderer(
	vren::vk_utils::toolbox const& toolbox
) :
	m_toolbox(&toolbox),

	m_points_buffer(toolbox, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
	m_lines_buffer(toolbox, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),

	m_render_pass(create_render_pass())
{}

vren::vk_render_pass vren::dbg_renderer::create_render_pass()
{
	/* Attachments */
	VkAttachmentDescription attachments[]{
		{ // Color attachment
			.flags = NULL,
			.format = VREN_COLOR_BUFFER_OUTPUT_FORMAT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		},
		{ // Depth attachment
			.flags = NULL,
			.format = VREN_DEPTH_BUFFER_OUTPUT_FORMAT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		}
	};

	/* Subpass */
	VkAttachmentReference color_attachment_ref{ .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depth_attachment_ref{ .attachment = 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	VkSubpassDescription subpasses[]{
		{ // Subpass 0
			.flags = NULL,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount = 0,
			.pInputAttachments = nullptr,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_ref,
			.pResolveAttachments = 0,
			.pDepthStencilAttachment = &depth_attachment_ref,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr
		}
	};

	/* Dependencies */
	VkSubpassDependency dependencies[]{
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask =
				VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = NULL
		}
	};

	/* */

	VkRenderPassCreateInfo render_pass_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.attachmentCount = std::size(attachments),
		.pAttachments = attachments,
		.subpassCount = std::size(subpasses),
		.pSubpasses = subpasses,
		.dependencyCount = std::size(dependencies),
		.pDependencies = dependencies
	};

	VkRenderPass render_pass;
	vren::vk_utils::check(vkCreateRenderPass(m_toolbox->m_context->m_device, &render_pass_info, nullptr, &render_pass));
	return vren::vk_render_pass(m_toolbox->m_context, render_pass);
}

vren::vk_utils::self_described_graphics_pipeline vren::dbg_renderer::create_graphics_pipeline()
{

}

void vren::dbg_renderer::clear()
{
	m_points_buffer.clear();
	m_lines_buffer.clear();
}

void vren::dbg_renderer::draw_point(point point)
{
	m_points_buffer.push_value(&point, sizeof(point));
}

void vren::dbg_renderer::draw_line(line line)
{
	m_lines_buffer.push_value(&line, sizeof(line));
}

void vren::dbg_renderer::render()
{

}
