#include "mesh_shader_draw_pass.hpp"

#include "context.hpp"
#include "toolbox.hpp"
#include "renderer.hpp"
#include "vk_helpers/shader.hpp"
#include "vk_helpers/debug_utils.hpp"

vren::mesh_shader_draw_pass::mesh_shader_draw_pass(vren::context const& context, VkRenderPass render_pass, uint32_t subpass_idx) :
	m_context(&context),
	m_render_pass(render_pass),
	m_subpass_idx(subpass_idx),
	m_pipeline(create_graphics_pipeline())
{}

vren::vk_utils::pipeline vren::mesh_shader_draw_pass::create_graphics_pipeline()
{
	/* Vertex input state */
	/* Input assembly state */
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
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/draw.task.spv"),
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/draw.mesh.spv"),
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/pbr_draw.frag.spv")
	};
	return vren::vk_utils::create_graphics_pipeline(
		*m_context,
		shaders,
		nullptr,
		nullptr,
		nullptr,
		&viewport_info,
		&rasterization_info,
		&multisample_info,
		&depth_stencil_info,
		&color_blend_info,
		&dynamic_state_info,
		m_render_pass,
		m_subpass_idx
	);
}

void write_meshlet_buffer_descriptor_set(vren::context const& ctx, VkDescriptorSet desc_set, VkBuffer vertex_buffer, VkBuffer meshlet_vertex_buffer, VkBuffer meshlet_triangle_buffer, VkBuffer meshlet_buffer)
{
	VkDescriptorBufferInfo buffer_info[]{
		{ // Vertex buffer
			.buffer = vertex_buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE,
		},
		{ // Meshlet vertex buffer
			.buffer = meshlet_vertex_buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE,
		},
		{ // Meshlet triangle buffer
			.buffer = meshlet_triangle_buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE,
		},
		{ // Meshlet buffer
			.buffer = meshlet_buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE
		}
	};
	VkWriteDescriptorSet write_desc_set{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = desc_set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = std::size(buffer_info),
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = buffer_info
	};
	vkUpdateDescriptorSets(ctx.m_device, 1, &write_desc_set, 0, nullptr);
}

void vren::mesh_shader_draw_pass::render(
	uint32_t frame_idx,
	VkCommandBuffer command_buffer,
	vren::resource_container& resource_container,
	vren::camera const& camera,
	vren::draw_buffer const& draw_buffer,
	vren::light_array const& light_array
)
{
	m_pipeline.bind(command_buffer);

	// Push constants
	m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT, &camera, sizeof(vren::camera));

	// Bind texture manager
	m_pipeline.bind_descriptor_set(command_buffer, 0, m_context->m_toolbox->m_texture_manager.m_descriptor_set->m_handle.m_descriptor_set);
	resource_container.add_resource(m_context->m_toolbox->m_texture_manager.m_descriptor_set);

	// Bind light array
	auto light_array_descriptor_set = light_array.get_descriptor_set(frame_idx);
	m_pipeline.bind_descriptor_set(command_buffer, 1, light_array_descriptor_set);

	// Bind draw buffer
	m_pipeline.acquire_and_bind_descriptor_set(*m_context, command_buffer, resource_container, 2, [&](VkDescriptorSet desc_set) {
		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) desc_set, "meshlet_buffer");
		write_meshlet_buffer_descriptor_set(
			*m_context,
			desc_set,
			draw_buffer.m_vertex_buffer.m_buffer.m_handle,
			draw_buffer.m_meshlet_vertex_buffer.m_buffer.m_handle,
			draw_buffer.m_meshlet_triangle_buffer.m_buffer.m_handle,
			draw_buffer.m_meshlet_buffer.m_buffer.m_handle
		);
	});

	// Bind material manager
	auto material_descriptor_set = m_context->m_toolbox->m_material_manager.get_descriptor_set(frame_idx);
	m_pipeline.bind_descriptor_set(command_buffer, 3, material_descriptor_set);

	// Draw
	vkCmdDrawMeshTasksNV(command_buffer, (uint32_t) draw_buffer.m_meshlet_count, 0);
}