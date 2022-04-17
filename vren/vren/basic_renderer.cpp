#include "renderer.hpp"

#include <array>

#include "context.hpp"
#include "vk_helpers/misc.hpp"
#include "base/base.hpp"

vren::vk_render_pass vren::basic_renderer::create_render_pass()
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
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
		{ // Depth attachment
			.flags = NULL,
			.format = VREN_DEPTH_BUFFER_OUTPUT_FORMAT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		}
	};

	/* Subpass */
	VkAttachmentReference color_attachment_ref{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkAttachmentReference depth_attachment_ref{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	VkSubpassDescription subpasses[]{
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount = 0,
			.pInputAttachments = nullptr,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_ref,
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = &depth_attachment_ref,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr
		}
	};

	/* Subpass dependencies */
	VkSubpassDependency dependencies[]{
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
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
	VREN_CHECK(vkCreateRenderPass(m_context->m_device, &render_pass_info, nullptr, &render_pass), m_context);
	return vren::vk_render_pass(*m_context, render_pass);
}

vren::basic_renderer::basic_renderer(vren::context const& context) :
	m_context(&context),
	m_render_pass(create_render_pass()),
	m_vertex_pipeline_draw_pass(context, m_render_pass.m_handle, 0),
	m_light_array(context)
{}

void vren::basic_renderer::render(
	uint32_t frame_idx,
	VkCommandBuffer command_buffer,
	vren::resource_container& resource_container,
	vren::render_target const& render_target,
	vren::camera const& camera,
	std::vector<vren::mesh_buffer> const& mesh_buffers
)
{
	m_light_array.sync_buffers(frame_idx);

	/* Render pass begin */
	VkRenderPassBeginInfo render_pass_begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = m_render_pass.m_handle,
		.framebuffer = render_target.m_framebuffer,
		.renderArea = render_target.m_render_area,
		.clearValueCount = 0,
		.pClearValues = nullptr
	};
	vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(command_buffer, 0, 1, &render_target.m_viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &render_target.m_render_area);

	/* Recording */
	for (auto const& mesh_buffer : mesh_buffers)
	{
		m_vertex_pipeline_draw_pass.draw(frame_idx, command_buffer, resource_container, camera, mesh_buffer, m_light_array);
	}

	/* Render pass end */
	vkCmdEndRenderPass(command_buffer);
}
