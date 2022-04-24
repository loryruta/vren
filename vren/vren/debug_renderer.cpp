#include "debug_renderer.hpp"

#include <execution>

#include <volk.h>
#include <glm/gtc/matrix_transform.hpp>

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

	m_render_pass(create_render_pass()),
	m_pipeline(create_graphics_pipeline())
{}

vren::vk_render_pass vren::debug_renderer::create_render_pass()
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
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
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
	VREN_CHECK(vkCreateRenderPass(m_context->m_device, &render_pass_info, nullptr, &render_pass), m_context);
	return vren::vk_render_pass(*m_context, render_pass);
}

vren::vk_utils::pipeline vren::debug_renderer::create_graphics_pipeline()
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
		.pNext = nullptr,
		.flags = NULL,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {},
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
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
		m_render_pass.m_handle,
		0
	);
}

void vren::debug_renderer::render(
	uint32_t frame_idx,
	VkCommandBuffer command_buffer,
	vren::resource_container& resource_container,
	vren::render_target const& render_target,
	vren::camera const& camera,
	vren::debug_renderer_draw_buffer const& draw_buffer
)
{
	if (draw_buffer.m_vertex_count == 0) {
		return;
	}

	// Render pass begin
	VkRenderPassBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = m_render_pass.m_handle,
		.framebuffer = render_target.m_framebuffer,
		.renderArea = render_target.m_render_area,
		.clearValueCount = 0,
		.pClearValues = nullptr
	};
	vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(command_buffer, 0, 1, &render_target.m_viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &render_target.m_render_area);

	m_pipeline.bind(command_buffer);

	// Camera
	m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_VERTEX_BIT, &camera, sizeof(vren::camera));

	VkDeviceSize offsets[]{0};

	// Draw
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &draw_buffer.m_vertex_buffer.m_buffer->m_buffer.m_handle, offsets);
	resource_container.add_resource(draw_buffer.m_vertex_buffer.m_buffer);

	vkCmdSetLineWidth(command_buffer, 1.0f);
	vkCmdSetPrimitiveTopologyEXT(command_buffer, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

	vkCmdDraw(command_buffer, draw_buffer.m_vertex_count, 1, 0, 0);

	// Render pass end
	vkCmdEndRenderPass(command_buffer);
}
