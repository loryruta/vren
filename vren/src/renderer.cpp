#include "renderer.hpp"

#include "simple_draw.hpp"
#include "imgui_renderer.hpp"
#include "material.hpp"
#include "utils/image.hpp"
#include "utils/misc.hpp"

std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> vren::renderer::_create_point_lights_buffers()
{
	return std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT>{
		vren::vk_utils::alloc_host_visible_buffer(m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VREN_MAX_LIGHTS_COUNT * sizeof(vren::point_light), true)...
	};
}

vren::renderer::renderer(std::shared_ptr<vren::context> const& ctx) :
	m_context(ctx),
	m_render_pass(_create_render_pass(ctx)),

	m_material_descriptor_set_layout(vren::create_material_descriptor_set_layout(ctx)),
	m_light_array_descriptor_set_layout(vren::create_light_array_descriptor_set_layout(ctx)),

	m_material_descriptor_pool(std::make_shared<vren::material_descriptor_pool>(ctx))

{
}

vren::renderer::~renderer()
{}

void vren::renderer::_init()
{
	m_simple_draw_pass = std::make_unique<vren::simple_draw_pass>(shared_from_this());
}

vren::vk_render_pass vren::renderer::_create_render_pass(std::shared_ptr<vren::context> const& ctx)
{
	VkAttachmentDescription color_attachment{};
	color_attachment.format = vren::renderer::k_color_output_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depth_attachment{};
	depth_attachment.format = VK_FORMAT_D32_SFLOAT;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription attachments[]{
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
	render_pass_info.attachmentCount = std::size(attachments);
	render_pass_info.pAttachments = attachments;
	render_pass_info.subpassCount = subpasses.size();
	render_pass_info.pSubpasses = subpasses.data();
	render_pass_info.dependencyCount = dependencies.size();
	render_pass_info.pDependencies = dependencies.data();

	VkRenderPass render_pass;
	vren::vk_utils::check(vkCreateRenderPass(ctx->m_device, &render_pass_info, nullptr, &render_pass));
	return vren::vk_render_pass(ctx, render_pass);
}

void vren::renderer::render(
	uint32_t frame_idx,
	vren::resource_container& resource_container,
	vren::render_target const& target,
	VkSemaphore src_semaphore,
	VkSemaphore dst_semaphore,
	vren::render_list const& render_list,
	vren::light_array const& lights_array,
	vren::camera const& camera
)
{
	auto cmd_buf = m_context->m_graphics_command_pool->acquire_command_buffer();

	/* Command buffer begin */
	VkCommandBufferBeginInfo cmd_buf_begin_info{};
	cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buf_begin_info.pNext = nullptr;
	cmd_buf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmd_buf_begin_info.pInheritanceInfo = nullptr;

	vren::vk_utils::check(vkBeginCommandBuffer(cmd_buf.m_handle, &cmd_buf_begin_info));

	/* Render pass begin */
	VkClearValue clear_values[] = {
		{ .color = m_clear_color },
		{ .depthStencil = { 1.0f, 0 } }
	};
	VkRenderPassBeginInfo render_pass_begin_info{};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = m_render_pass.m_handle;
	render_pass_begin_info.framebuffer = target.m_framebuffer;
	render_pass_begin_info.renderArea = target.m_render_area;
	render_pass_begin_info.clearValueCount = std::size(clear_values);
	render_pass_begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(cmd_buf.m_handle, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(cmd_buf.m_handle, 0, 1, &target.m_viewport);
	vkCmdSetScissor(cmd_buf.m_handle, 0, 1, &target.m_render_area);

	/* Write light buffers */
	vren::vk_utils::update_host_visible_buffer(*m_context, m_light_buffers[frame_idx], lights_array.m_point_lights.data(), lights_array.m_point_lights.size() * sizeof(vren::point_light), 0);
	vren::vk_utils::update_host_visible_buffer(*m_context, m_light_buffers[frame_idx], lights_array.m_directional_lights.data(), lights_array.m_directional_lights.size() * sizeof(vren::directional_light), 0);
	vren::vk_utils::update_host_visible_buffer(*m_context, m_light_buffers[frame_idx], lights_array.m.data(), lights_array.m_point_lights.size() * sizeof(vren::light_array), 0);

	/* Subpass recording */
	m_simple_draw_pass->record_commands(frame, cmd_buf, render_list, lights_array, camera);

	/* Render pass end */
	vkCmdEndRenderPass(cmd_buf.m_handle);
	vren::vk_utils::check(vkEndCommandBuffer(cmd_buf.m_handle));

	/* Submission */
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &src_semaphore;
	submit_info.pWaitDstStageMask = &wait_stage;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf.m_handle;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &dst_semaphore;

	vren::vk_utils::check(vkQueueSubmit(m_context->m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
}

std::shared_ptr<vren::renderer> vren::renderer::create(std::shared_ptr<context> const& ctx)
{
	auto renderer = std::shared_ptr<vren::renderer>(new vren::renderer(ctx));
	renderer->_init();
	return renderer;
}
