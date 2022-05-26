#include "debug_renderer.hpp"

#include <execution>

#include "volk.h"
#include "glm/gtc/matrix_transform.hpp"

#include "vk_helpers/buffer.hpp"
#include "vk_helpers/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Debug renderer draw buffer
// --------------------------------------------------------------------------------------------------------------------------------

vren::debug_renderer_draw_buffer::debug_renderer_draw_buffer(vren::context const& context) :
	m_vertex_buffer(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
{}

void vren::debug_renderer_draw_buffer::clear()
{
	m_vertex_buffer.clear();
	m_vertex_count = 0;
}

void vren::debug_renderer_draw_buffer::add_lines(vren::debug_renderer_line const* lines, size_t line_count)
{
	std::vector<vren::debug_renderer_vertex> vertices(line_count * 2);

	std::for_each(std::execution::par, lines, lines + line_count, [&](vren::debug_renderer_line const& line) {
		uint32_t i = &line - lines;
		vertices[i * 2 + 0] = { .m_position = line.m_from, .m_color = line.m_color };
		vertices[i * 2 + 1] = { .m_position = line.m_to, .m_color = line.m_color };
	});

	m_vertex_buffer.append_data(vertices.data(), vertices.size() * sizeof(vren::debug_renderer_vertex));
	m_vertex_count += vertices.size();
}

void vren::debug_renderer_draw_buffer::add_points(vren::debug_renderer_point const* points, size_t point_count)
{
	std::vector<vren::debug_renderer_line> lines(point_count * 3);

	std::for_each(std::execution::par, points, points + point_count, [&](vren::debug_renderer_point const& point) {
		uint32_t i = &point - points;

		lines[i * 3 + 0] = { .m_from = point.m_position - glm::vec3(k_debug_renderer_point_size, 0, 0), .m_to = point.m_position + glm::vec3(k_debug_renderer_point_size, 0, 0), .m_color = point.m_color };
		lines[i * 3 + 1] = { .m_from = point.m_position - glm::vec3(0, k_debug_renderer_point_size, 0), .m_to = point.m_position + glm::vec3(0, k_debug_renderer_point_size, 0), .m_color = point.m_color };
		lines[i * 3 + 2] = { .m_from = point.m_position - glm::vec3(0, 0, k_debug_renderer_point_size), .m_to = point.m_position + glm::vec3(0, 0, k_debug_renderer_point_size), .m_color = point.m_color };
	});

	add_lines(lines.data(), lines.size());
}

void vren::debug_renderer_draw_buffer::add_spheres(vren::debug_renderer_sphere const* spheres, size_t sphere_count)
{
	std::vector<vren::debug_renderer_line> lines(sphere_count * 3 * vren::k_debug_sphere_segment_count);

	std::for_each(std::execution::par, spheres, spheres + sphere_count, [&](vren::debug_renderer_sphere const& sphere) {
		uint32_t i = &sphere - spheres;

		for (uint32_t j = 0; j < vren::k_debug_sphere_segment_count; j++)
		{
			float a0 = (j / (float) vren::k_debug_sphere_segment_count) * 2 * glm::pi<float>();
			float a1 = ((j + 1) / (float) vren::k_debug_sphere_segment_count) * 2 * glm::pi<float>();

			glm::vec3 p0, p1;

			// XZ circle
			p0 = glm::vec3(sphere.m_center.x + sphere.m_radius * glm::cos(a0), sphere.m_center.y, sphere.m_center.z + sphere.m_radius * glm::sin(a0));
			p1 = glm::vec3(sphere.m_center.x + sphere.m_radius * glm::cos(a1), sphere.m_center.y, sphere.m_center.z + sphere.m_radius * glm::sin(a1));

			lines[i * 3 * vren::k_debug_sphere_segment_count + j * 3 + 0] = { .m_from = p0, .m_to = p1, .m_color = sphere.m_color };

			// YZ circle
			p0 = glm::vec3(sphere.m_center.x, sphere.m_center.y + sphere.m_radius * glm::cos(a0), sphere.m_center.z + sphere.m_radius * glm::sin(a0));
			p1 = glm::vec3(sphere.m_center.x, sphere.m_center.y + sphere.m_radius * glm::cos(a1), sphere.m_center.z + sphere.m_radius * glm::sin(a1));

			lines[i * 3 * vren::k_debug_sphere_segment_count + j * 3 + 1] = { .m_from = p0, .m_to = p1, .m_color = sphere.m_color };

			// XY circle
			p0 = glm::vec3(sphere.m_center.x + sphere.m_radius * glm::cos(a0), sphere.m_center.y + sphere.m_radius * glm::sin(a0), sphere.m_center.z);
			p1 = glm::vec3(sphere.m_center.x + sphere.m_radius * glm::cos(a1), sphere.m_center.y + sphere.m_radius * glm::sin(a1), sphere.m_center.z);

			lines[i * 3 * vren::k_debug_sphere_segment_count + j * 3 + 2] = { .m_from = p0, .m_to = p1, .m_color = sphere.m_color };
		}
	});

	add_lines(lines.data(), lines.size());
}

// --------------------------------------------------------------------------------------------------------------------------------
// Debug renderer
// --------------------------------------------------------------------------------------------------------------------------------

vren::debug_renderer::debug_renderer(vren::context const& ctx) :
	m_context(&ctx),
	m_pipeline(create_graphics_pipeline(/* depth_test */ true)),
	m_no_depth_test_pipeline(create_graphics_pipeline(false))
{}

vren::vk_utils::pipeline vren::debug_renderer::create_graphics_pipeline(bool depth_test)
{
	VkVertexInputBindingDescription vtx_bindings[]{
		{ .binding = 0, .stride = sizeof(vren::debug_renderer_vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, }, // Vertex buffer
	};

	VkVertexInputAttributeDescription vtx_attribs[]{
		{ .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = (uint32_t) offsetof(vren::debug_renderer_vertex, m_position) }, // Position
		{ .location = 1, .binding = 0, .format = VK_FORMAT_R32_UINT, .offset = (uint32_t) offsetof(vren::debug_renderer_vertex, m_color) },            // Color
	};

	/* Vertex input state */
	VkPipelineVertexInputStateCreateInfo vtx_input_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.vertexBindingDescriptionCount = (uint32_t) std::size(vtx_bindings),
		.pVertexBindingDescriptions = vtx_bindings,
		.vertexAttributeDescriptionCount = (uint32_t) std::size(vtx_attribs),
		.pVertexAttributeDescriptions = vtx_attribs,
	};

	/* Input assembly state */
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST, // Dynamically set
		.primitiveRestartEnable = false
	};

	/* Tessellation state */

	/* Viewport state */
	VkPipelineViewportStateCreateInfo viewport_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.viewportCount = 1,
		.pViewports = nullptr,
		.scissorCount = 1,
		.pScissors = nullptr
	};

	/* Rasterization state */
	VkPipelineRasterizationStateCreateInfo raster_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.depthClampEnable = false,
		.rasterizerDiscardEnable = false,
		.polygonMode = VK_POLYGON_MODE_LINE,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = false,
		.depthBiasConstantFactor = 0,
		.depthBiasClamp = 0,
		.depthBiasSlopeFactor = 0,
		.lineWidth = 0.0f // Dynamically set
	};

	/* Multisample state */
	VkPipelineMultisampleStateCreateInfo multisample_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 0.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

	/* Depth-stencil state */
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = depth_test,
		.depthWriteEnable = depth_test,
		.depthCompareOp = VK_COMPARE_OP_LESS
	};

	/* Color blend state */
	VkPipelineColorBlendAttachmentState color_blend_attachments[]{
		{
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = {},
			.dstColorBlendFactor = {},
			.colorBlendOp = {},
			.srcAlphaBlendFactor = {},
			.dstAlphaBlendFactor = {},
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		}
	};
	VkPipelineColorBlendStateCreateInfo color_blend_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.logicOpEnable = VK_FALSE,
		.logicOp = {},
		.attachmentCount = std::size(color_blend_attachments),
		.pAttachments = color_blend_attachments,
		.blendConstants = {}
	};

	/* Dynamic state */
	VkDynamicState dynamic_states[]{
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH,
		VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT
	};
	VkPipelineDynamicStateCreateInfo dynamic_state_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.dynamicStateCount = std::size(dynamic_states),
		.pDynamicStates = dynamic_states
	};

	// Pipeline rendering
	VkFormat color_attachment_formats[] { VREN_COLOR_BUFFER_OUTPUT_FORMAT };
	VkPipelineRenderingCreateInfoKHR pipeline_rendering_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		.pNext = nullptr,
		.viewMask = 0,
		.colorAttachmentCount = std::size(color_attachment_formats),
		.pColorAttachmentFormats = color_attachment_formats,
		.depthAttachmentFormat = VREN_DEPTH_BUFFER_OUTPUT_FORMAT,
		.stencilAttachmentFormat = VK_FORMAT_UNDEFINED
	};

	//

	vren::vk_utils::shader shaders[]{
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/debug_draw.vert.spv"),
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/debug_draw.frag.spv")
	};
	return vren::vk_utils::create_graphics_pipeline(
		*m_context,
		shaders,
		&vtx_input_info,
		&input_assembly_info,
		nullptr,
		&viewport_info,
		&raster_info,
		&multisample_info,
		&depth_stencil_info,
		&color_blend_info,
		&dynamic_state_info,
		&pipeline_rendering_info
	);
}

vren::render_graph::graph_t vren::debug_renderer::render(
	vren::render_graph::allocator& render_graph_allocator,
	vren::render_target const& render_target,
	vren::camera const& camera,
	vren::debug_renderer_draw_buffer const& draw_buffer,
	bool world_space
)
{
	auto node = render_graph_allocator.allocate();
	node->set_name("debug_renderer | render");
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
		.m_access_flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	});
	node->set_callback([=, &draw_buffer](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		if (draw_buffer.m_vertex_count == 0) {
		   return;
		}

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

		vren::vk_utils::pipeline& pipeline = world_space ? m_pipeline : m_no_depth_test_pipeline;

		// Bind pipeline
		pipeline.bind(command_buffer);

		// Camera
		if (world_space)
		{
			pipeline.push_constants(command_buffer, VK_SHADER_STAGE_VERTEX_BIT, &camera, sizeof(vren::camera));
		}
		else
		{
			vren::camera no_camera{
				.m_view = glm::identity<glm::mat4>(),
				.m_projection = glm::identity<glm::mat4>()
			};
			pipeline.push_constants(command_buffer, VK_SHADER_STAGE_VERTEX_BIT, &no_camera, sizeof(vren::camera));
		}

		VkDeviceSize offsets[]{0};

		// Draw
		vkCmdBindVertexBuffers(command_buffer, 0, 1, &draw_buffer.m_vertex_buffer.m_buffer->m_buffer.m_handle, offsets);
		resource_container.add_resource(draw_buffer.m_vertex_buffer.m_buffer);

		vkCmdSetLineWidth(command_buffer, 1.0f);
		vkCmdSetPrimitiveTopologyEXT(command_buffer, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

		vkCmdDraw(command_buffer, draw_buffer.m_vertex_count, 1, 0, 0);

		// End rendering
		vkCmdEndRenderingKHR(command_buffer);
	});
	return vren::render_graph::gather(node);
}
