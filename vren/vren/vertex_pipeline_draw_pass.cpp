#include "vertex_pipeline_draw_pass.hpp"

#include "mesh.hpp"
#include "context.hpp"
#include "toolbox.hpp"

vren::vk_utils::pipeline vren::vertex_pipeline_draw_pass::create_graphics_pipeline(VkRenderPass render_pass, uint32_t subpass_idx)
{
	/* Vertex input state */
	VkVertexInputBindingDescription vertex_input_binding_descriptions[]{
		{ .binding = 0, .stride = sizeof(vren::vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX },          // Vertex buffer
		{ .binding = 1, .stride = 0, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX },                             // Material index buffer
		{ .binding = 2, .stride = sizeof(vren::mesh_instance), .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE }, // Instance buffer
	};

	VkVertexInputAttributeDescription vertex_input_attribute_descriptions[]{
		// Vertex buffer
		{ .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = (uint32_t) offsetof(vren::vertex, m_position) },  // Position
		{ .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = (uint32_t) offsetof(vren::vertex, m_normal) },    // Normal
		{ .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT,    .offset = (uint32_t) offsetof(vren::vertex, m_texcoords) }, // Texcoords

		// Material buffer
		{ .location = 8, .binding = 1, .format = VK_FORMAT_R32_UINT, .offset = (uint32_t) 0 },

		// Instance buffer
		{ .location = 16, .binding = 2, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = (uint32_t) 0 },                     // Transform 0
		{ .location = 17, .binding = 2, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = (uint32_t) sizeof(glm::vec4) },     // Transform 1
		{ .location = 18, .binding = 2, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = (uint32_t) sizeof(glm::vec4) * 2 }, // Transform 2
		{ .location = 19, .binding = 2, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = (uint32_t) sizeof(glm::vec4) * 3 }, // Transform 3
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.vertexBindingDescriptionCount = std::size(vertex_input_binding_descriptions),
		.pVertexBindingDescriptions = vertex_input_binding_descriptions,
		.vertexAttributeDescriptionCount = std::size(vertex_input_attribute_descriptions),
		.pVertexAttributeDescriptions = vertex_input_attribute_descriptions
	};

	/* Input assembly state */
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
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
	VkPipelineRasterizationStateCreateInfo rasterization_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f
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
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamic_state_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.dynamicStateCount = std::size(dynamic_states),
		.pDynamicStates = dynamic_states
	};

	/* */

	vren::vk_utils::shader shaders[]{
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/basic_draw.vert.spv"),
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/pbr_draw.frag.spv")
	};
	return vren::vk_utils::create_graphics_pipeline(
		*m_context,
		shaders,
		&vertex_input_state_info,
		&input_assembly_state_info,
		nullptr,
		&viewport_info,
		&rasterization_info,
		&multisample_info,
		&depth_stencil_info,
		&color_blend_info,
		&dynamic_state_info,
		render_pass,
		subpass_idx
	);
}

vren::vertex_pipeline_draw_pass::vertex_pipeline_draw_pass(vren::context const& context, VkRenderPass render_pass, uint32_t subpass_idx) :
	m_context(&context),
	m_pipeline(create_graphics_pipeline(render_pass, subpass_idx))
{}

void write_material_buffer_descriptor_set(vren::context const& context, VkDescriptorSet descriptor_set, VkBuffer material_buffer)
{
	VkDescriptorBufferInfo buffer_info[]{
		{
			.buffer = material_buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE,
		}
	};
	VkWriteDescriptorSet write_desc_set{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptor_set,
		.dstBinding = 4,
		.dstArrayElement = 0,
		.descriptorCount = std::size(buffer_info),
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = buffer_info
	};
	vkUpdateDescriptorSets(context.m_device, 1, &write_desc_set, 0, nullptr);
}

void vren::vertex_pipeline_draw_pass::draw(
	uint32_t frame_idx,
	VkCommandBuffer command_buffer,
	vren::resource_container& resource_container,
	vren::camera const& camera,
	vren::mesh_buffer const& mesh_buffer,
	vren::light_array const& light_array
)
{
	m_pipeline.bind(command_buffer);

	// Camera
	m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &camera, sizeof(camera));

	m_pipeline.bind_vertex_buffer(command_buffer, 0, mesh_buffer.m_vertex_buffer.m_buffer.m_handle); // Vertex buffer
	m_pipeline.bind_vertex_buffer(command_buffer, 1, mesh_buffer.m_material_index_buffer.m_buffer.m_handle); // Material index buffer
	m_pipeline.bind_vertex_buffer(command_buffer, 2, mesh_buffer.m_instance_buffer.m_buffer.m_handle); // Instance buffer
	m_pipeline.bind_index_buffer(command_buffer, mesh_buffer.m_index_buffer.m_buffer.m_handle, VK_INDEX_TYPE_UINT32); // Index buffer

	// Texture manager
	m_pipeline.bind_descriptor_set(command_buffer, 0, m_context->m_toolbox->m_texture_manager.m_descriptor_set->m_handle.m_descriptor_set);
	resource_container.add_resource(m_context->m_toolbox->m_texture_manager.m_descriptor_set);

	// Light array
	auto light_array_descriptor_set = light_array.get_descriptor_set(frame_idx);
	m_pipeline.bind_descriptor_set(command_buffer, 1, light_array_descriptor_set);

	// Material manager
	auto material_descriptor_set = m_context->m_toolbox->m_material_manager.get_descriptor_set(frame_idx);
	m_pipeline.bind_descriptor_set(command_buffer, 3, material_descriptor_set);

	vkCmdDrawIndexed(command_buffer, mesh_buffer.m_index_count, mesh_buffer.m_instance_count, 0, 0, 0);
}
