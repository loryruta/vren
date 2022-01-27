#include "renderer.hpp"

#include "simple_draw.hpp"
#include "imgui_renderer.hpp"
#include "utils/image.hpp"
#include "utils/misc.hpp"

vren::renderer::renderer(std::shared_ptr<vren::context> const& ctx) :
	m_context(ctx)
{}

vren::renderer::~renderer()
{
	vkDestroyRenderPass(m_context->m_device, m_render_pass, nullptr);
}

void vren::renderer::_init()
{
	_init_render_pass();

	m_simple_draw_pass = std::make_unique<vren::simple_draw_pass>(shared_from_this());
}

void vren::renderer::_init_render_pass()
{
	// ---------------------------------------------------------------- Attachments

	VkAttachmentDescription color_attachment{};
	color_attachment.format = vren::renderer::k_color_output_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // todo we're not always intending to present a frame (e.g. when we render to a texture (?))

	VkAttachmentDescription depth_attachment{};
	depth_attachment.format = VK_FORMAT_D32_SFLOAT;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::initializer_list<VkAttachmentDescription> attachments = {
		color_attachment,
		depth_attachment
	};

	// ---------------------------------------------------------------- Subpasses

	std::vector<VkSubpassDescription> subpasses;
	{
		// simple_draw
		VkAttachmentReference color_attachment_ref{};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref{};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;
		subpasses.push_back(subpass);
	}

	// ---------------------------------------------------------------- Dependencies

	std::vector<VkSubpassDependency> dependencies;
	{
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies.push_back(dependency);
	}

	VkRenderPassCreateInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = attachments.size();
	render_pass_info.pAttachments = attachments.begin();
	render_pass_info.subpassCount = subpasses.size();
	render_pass_info.pSubpasses = subpasses.data();
	render_pass_info.dependencyCount = dependencies.size();
	render_pass_info.pDependencies = dependencies.data();

	vren::vk_utils::check(vkCreateRenderPass(m_context->m_device, &render_pass_info, nullptr, &m_render_pass));
}

void vren::renderer::record_commands(
	vren::frame& frame,
	vren::vk_command_buffer const& cmd_buf,
	vren::renderer_target const& target,
	vren::render_list const& render_list,
	vren::lights_array const& lights_array,
	vren::camera const& camera
)
{
	VkRenderPassBeginInfo render_pass_begin_info{};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = m_render_pass;
	render_pass_begin_info.framebuffer = target.m_framebuffer;
	render_pass_begin_info.renderArea = target.m_render_area;

	std::initializer_list<VkClearValue> clear_values{
		{ .color = m_clear_color },
		{ .depthStencil = { 1.0f, 0 } }
	};
	render_pass_begin_info.clearValueCount = (uint32_t) clear_values.size();
	render_pass_begin_info.pClearValues = clear_values.begin();

	vkCmdBeginRenderPass(cmd_buf.m_handle, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(cmd_buf.m_handle, 0, 1, &target.m_viewport);
	vkCmdSetScissor(cmd_buf.m_handle, 0, 1, &target.m_render_area);

	m_simple_draw_pass->record_commands(frame, cmd_buf, render_list, lights_array, camera);

	vkCmdEndRenderPass(cmd_buf.m_handle);
}

std::shared_ptr<vren::renderer> vren::renderer::create(std::shared_ptr<context> const& ctx)
{
	auto renderer = std::shared_ptr<vren::renderer>(new vren::renderer(ctx));
	renderer->_init();
	return renderer;
}
