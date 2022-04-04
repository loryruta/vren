#include "dbg_renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "vk_helpers/buffer.hpp"
#include "vk_helpers/misc.hpp"

vren::dbg_renderer::dbg_renderer(vren::context const& ctx) :
	m_context(&ctx),

	m_identity_instance_buffer(create_identity_instance_buffer()),

	m_points_vertex_buffer(ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
	m_lines_vertex_buffer(ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),

	m_render_pass(create_render_pass()),
	m_pipeline(create_graphics_pipeline())
{}

vren::vk_utils::buffer vren::dbg_renderer::create_identity_instance_buffer()
{
	auto buf = vren::vk_utils::alloc_device_only_buffer(*m_context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(dbg_renderer::instance_data));

	dbg_renderer::instance_data id{
		.m_transform = glm::identity<glm::mat4>(),
		.m_color = glm::vec4(1)
	};
	vren::vk_utils::update_device_only_buffer(*m_context, buf, &id, sizeof(id), 0);

	return buf;
}

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
	vren::vk_utils::check(vkCreateRenderPass(m_context->m_device, &render_pass_info, nullptr, &render_pass));
	return vren::vk_render_pass(*m_context, render_pass);
}

vren::vk_utils::pipeline vren::dbg_renderer::create_graphics_pipeline()
{
	VkVertexInputBindingDescription vtx_bindings[]{
		{ .binding = 0, .stride = sizeof(dbg_renderer::vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, },        // Vertex buffer
		{ .binding = 1, .stride = sizeof(dbg_renderer::instance_data), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE } // Instance buffer
	};

	VkVertexInputAttributeDescription vtx_attribs[]{
		// Vertex position
		{ .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = (uint32_t) offsetof(dbg_renderer::vertex, m_position) },

		// Vertex color
		{ .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = (uint32_t) offsetof(dbg_renderer::vertex, m_color) },

		// Instance transform
		{ .location = 0, .binding = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 },
		{ .location = 0, .binding = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = sizeof(glm::vec4) },
		{ .location = 0, .binding = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = sizeof(glm::vec4) * 2 },
		{ .location = 0, .binding = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = sizeof(glm::vec4) * 3 },

		// Instance color
		{ .location = 1, .binding = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = (uint32_t) offsetof(dbg_renderer::instance_data, m_transform) },
	};

	/* Vertex input state */
	VkPipelineVertexInputStateCreateInfo vtx_input_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.vertexBindingDescriptionCount = (uint32_t) std::size(vtx_bindings),
		.pVertexBindingDescriptions = vtx_bindings,
		.vertexAttributeDescriptionCount = (uint32_t) std::size(vtx_bindings),
		.pVertexAttributeDescriptions = vtx_attribs,
	};

	/* Input assembly state */
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST, // Dynamically set
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
		.rasterizerDiscardEnable = true,
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
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/dbg_draw_line.vert"),
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/dbg_draw.frag")
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

void vren::dbg_renderer::clear()
{
}

void vren::dbg_renderer::draw_point(dbg_renderer::point point)
{
	dbg_renderer::vertex v{
		.m_position = point.m_position, .m_color = point.m_color
	};
	m_points_vertex_buffer.push_value(&v, 1);
	m_points_vertex_count++;
}

void vren::dbg_renderer::draw_line(dbg_renderer::line line)
{
	dbg_renderer::vertex v[]{
		{ .m_position = line.m_from, .m_color = line.m_color },
		{ .m_position = line.m_to, .m_color = line.m_color }
	};
	m_lines_vertex_buffer.push_value(v, std::size(v));
	m_lines_vertex_count += 2;
}

void vren::dbg_renderer::render(
	uint32_t frame_idx,
	VkCommandBuffer cmd_buf,
	vren::resource_container& res_container
)
{
	// NOTE: The resource container isn't used here (no add_resource(...) call happens), because the dbg_renderer
	// **is supposed to last the program lifetime** thus its resources are always ensured to exist while being used by GPU.

	m_pipeline.bind(cmd_buf);

	VkDeviceSize offsets[]{0};

	/* Draw points */
	vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_points_vertex_buffer.m_buffer->m_buffer.m_handle, offsets);
	vkCmdBindVertexBuffers(cmd_buf, 1, 1, &m_identity_instance_buffer.m_buffer.m_handle, offsets);

	vkCmdSetPrimitiveTopologyEXT(cmd_buf, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

	vkCmdDraw(cmd_buf, m_points_vertex_count, 1, 0, 0);

	/* Draw lines */
	vkCmdBindVertexBuffers(cmd_buf, 0, 1, &m_lines_vertex_buffer.m_buffer->m_buffer.m_handle, offsets);
	vkCmdBindVertexBuffers(cmd_buf, 1, 1, &m_identity_instance_buffer.m_buffer.m_handle, offsets);

	vkCmdSetPrimitiveTopologyEXT(cmd_buf, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

	vkCmdDraw(cmd_buf, m_lines_vertex_count, 1, 0, 0);
}
