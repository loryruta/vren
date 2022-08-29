#include "mesh_shader_renderer.hpp"

#include <array>

#include "context.hpp"
#include "vk_helpers/misc.hpp"
#include "vk_helpers/debug_utils.hpp"

vren::mesh_shader_renderer::mesh_shader_renderer(
	vren::context const& context,
	bool occlusion_culling
) :
	m_context(&context),
	m_mesh_shader_draw_pass(context, occlusion_culling),
	m_occlusion_culling(occlusion_culling)
{}

vren::render_graph_t vren::mesh_shader_renderer::render(
	vren::render_graph_allocator& render_graph_allocator,
	glm::uvec2 const& screen,
	vren::camera_data const& camera_data,
	vren::light_array const& light_array,
	vren::clusterized_model_draw_buffer const& draw_buffer,
	vren::depth_buffer_pyramid const& depth_buffer_pyramid,
	vren::gbuffer const& gbuffer,
	vren::vk_utils::depth_buffer_t const& depth_buffer
)
{
	vren::render_graph_node* node = render_graph_allocator.allocate();

	node->set_name("mesh_shader_renderer | render");

	node->set_src_stage(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

	gbuffer.add_render_graph_node_resources(*node, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	node->add_image({ .m_image = depth_buffer.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT, }, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

	node->set_callback([&](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkRect2D render_area = {
			.offset = {0, 0},
			.extent = {screen.x, screen.y}
		};

		// GBuffer
		VkRenderingAttachmentInfoKHR color_attachments[]{
			{ // Normal buffer
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
				.pNext = nullptr,
				.imageView = gbuffer.m_normal_buffer.get_image_view(),
				.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.resolveMode = VK_RESOLVE_MODE_NONE,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			},
			{ // Texcoord buffer
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
				.pNext = nullptr,
				.imageView = gbuffer.m_texcoord_buffer.get_image_view(),
				.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.resolveMode = VK_RESOLVE_MODE_NONE,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			},
			{ // Material index buffer
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
				.pNext = nullptr,
				.imageView = gbuffer.m_material_index_buffer.get_image_view(),
				.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.resolveMode = VK_RESOLVE_MODE_NONE,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			},
		};

		// Depth buffer
		VkRenderingAttachmentInfoKHR depth_buffer_attachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
			.pNext = nullptr,
			.imageView = depth_buffer.get_image_view(),
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE
		};

		// Begin rendering
		VkRenderingInfoKHR rendering_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
			.pNext = nullptr,
			.flags = NULL,
			.renderArea = render_area,
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = std::size(color_attachments),
			.pColorAttachments = color_attachments,
			.pDepthAttachment = &depth_buffer_attachment,
			.pStencilAttachment = nullptr
		};
		vkCmdBeginRendering(command_buffer, &rendering_info);

		VkViewport viewport{
			.x = 0,
			.y = (float) screen.y,
			.width = (float)screen.x,
			.height = -((float) screen.y),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		vkCmdSetScissor(command_buffer, 0, 1, &render_area);

		m_mesh_shader_draw_pass.render(frame_idx, command_buffer, resource_container, camera_data, draw_buffer, light_array, depth_buffer_pyramid);

		vkCmdEndRendering(command_buffer);
	});
	return vren::render_graph_gather(node);
}
