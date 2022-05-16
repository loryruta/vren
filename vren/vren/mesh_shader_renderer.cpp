#include "mesh_shader_renderer.hpp"

#include <array>

#include "context.hpp"
#include "vk_helpers/misc.hpp"
#include "vk_helpers/debug_utils.hpp"

void vren::set_object_names(vren::context const& context, vren::mesh_shader_renderer_draw_buffer const& draw_buffer)
{
	vren::vk_utils::set_object_name(context, VK_OBJECT_TYPE_BUFFER, (uint64_t) draw_buffer.m_vertex_buffer.m_buffer.m_handle, "vertex_buffer");
	vren::vk_utils::set_object_name(context, VK_OBJECT_TYPE_BUFFER, (uint64_t) draw_buffer.m_meshlet_vertex_buffer.m_buffer.m_handle, "meshlet_vertex_buffer");
	vren::vk_utils::set_object_name(context, VK_OBJECT_TYPE_BUFFER, (uint64_t) draw_buffer.m_meshlet_triangle_buffer.m_buffer.m_handle, "meshlet_triangle_buffer");
	vren::vk_utils::set_object_name(context, VK_OBJECT_TYPE_BUFFER, (uint64_t) draw_buffer.m_meshlet_buffer.m_buffer.m_handle, "meshlet_buffer");
	vren::vk_utils::set_object_name(context, VK_OBJECT_TYPE_BUFFER, (uint64_t) draw_buffer.m_instanced_meshlet_buffer.m_buffer.m_handle, "instanced_meshlet_buffer");
	vren::vk_utils::set_object_name(context, VK_OBJECT_TYPE_BUFFER, (uint64_t) draw_buffer.m_instance_buffer.m_buffer.m_handle, "instance_buffer");
}

vren::mesh_shader_renderer::mesh_shader_renderer(vren::context const& context) :
	m_context(&context),
	m_mesh_shader_draw_pass(context)
{}

vren::render_graph::graph_t vren::mesh_shader_renderer::render(
	vren::render_graph::allocator& render_graph_allocator,
	vren::render_target const& render_target,
	vren::camera const& camera,
	vren::light_array const& light_array,
	vren::mesh_shader_renderer_draw_buffer const& draw_buffer,
	vren::depth_buffer_pyramid const& depth_buffer_pyramid
)
{
	auto node = render_graph_allocator.allocate();
	node->set_name("mesh_shader_renderer | render");
	node->set_src_stage(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	node->add_image({
		.m_name = "color_buffer",
		.m_image = render_target.m_color_buffer->get_image(),
		.m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
		.m_image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.m_access_flags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	});
	node->add_image({
		.m_name = "depth_buffer",
		.m_image = render_target.m_depth_buffer->get_image(),
		.m_image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
		.m_image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.m_access_flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	});
	node->set_callback([&](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		// Color attachment
		VkRenderingAttachmentInfoKHR color_attachment_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
			.pNext = nullptr,
			.imageView = render_target.m_color_buffer->get_image_view(),
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE
		};

		// Depth attachment
		VkRenderingAttachmentInfoKHR depth_attachment_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
			.pNext = nullptr,
			.imageView = render_target.m_depth_buffer->get_image_view(),
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
			.renderArea = render_target.m_render_area,
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_info,
			.pDepthAttachment = &depth_attachment_info,
			.pStencilAttachment = nullptr
		};
		vkCmdBeginRenderingKHR(command_buffer, &rendering_info);

		vkCmdSetViewport(command_buffer, 0, 1, &render_target.m_viewport);
		vkCmdSetScissor(command_buffer, 0, 1, &render_target.m_render_area);

		// Recording
		m_mesh_shader_draw_pass.render(frame_idx, command_buffer, resource_container, camera, draw_buffer, light_array, depth_buffer_pyramid);

		// End rendering
		vkCmdEndRenderingKHR(command_buffer);
	});
	return vren::render_graph::gather(node);
}
