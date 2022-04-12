#include "simple_draw.hpp"

#include <iostream>

#include "context.hpp"
#include "renderer.hpp"
#include "vk_helpers/shader.hpp"
#include "vk_helpers/debug_utils.hpp"

vren::simple_draw_pass::simple_draw_pass(
	vren::context const& ctx,
	vren::renderer& renderer
) :
	m_context(&ctx),
	m_renderer(&renderer),

	m_pipeline(create_graphics_pipeline())
{}

vren::vk_utils::pipeline vren::simple_draw_pass::create_graphics_pipeline()
{
	/* Vertex input state */
	auto binding_descriptions = vren::render_object::get_all_binding_desc();
	auto attribute_descriptions = vren::render_object::get_all_attrib_desc();

	VkPipelineVertexInputStateCreateInfo vtx_input_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.vertexBindingDescriptionCount = binding_descriptions.size(),
		.pVertexBindingDescriptions = binding_descriptions.data(),
		.vertexAttributeDescriptionCount = attribute_descriptions.size(),
		.pVertexAttributeDescriptions = attribute_descriptions.data()
	};

	/* Input assembly state */
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
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
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/draw.task.spv"),
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/draw.mesh.spv"),
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/pbr_draw.frag.spv")
	};
	return vren::vk_utils::create_graphics_pipeline(
		*m_context,
		shaders,
		&vtx_input_info,
		&input_assembly_info,
		nullptr,
		&viewport_info,
		&rasterization_info,
		&multisample_info,
		&depth_stencil_info,
		&color_blend_info,
		&dynamic_state_info,
		m_renderer->m_render_pass->m_handle,
		0
	);
}


void write_meshlet_buffer_descriptor_set(vren::context const& ctx, VkDescriptorSet desc_set, VkBuffer vertex_buffer, VkBuffer index_buffer, VkBuffer meshlet_buffer)
{
	VkDescriptorBufferInfo buffer_info[]{
		{
			.buffer = vertex_buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE,
		},
		{
			.buffer = index_buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE,
		},
		{
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
		.descriptorCount = 3,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = buffer_info
	};
	vkUpdateDescriptorSets(ctx.m_device, 1, &write_desc_set, 0, nullptr);
}

void vren::simple_draw_pass::record_commands(
    int frame_idx,
    VkCommandBuffer cmd_buf,
	vren::resource_container& res_container,
	VkBuffer vertex_buffer,
	VkBuffer index_buffer,
	VkBuffer meshlet_buffer,
	size_t meshlet_count,
	vren::light_array const& light_arr,
	vren::camera const& camera
)
{
	m_pipeline.bind(cmd_buf);

	/* Camera */
	m_pipeline.push_constants(cmd_buf, VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT, &camera, sizeof(vren::camera));

	/* Vertex, index & meshlet buffer */
	m_pipeline.acquire_and_bind_descriptor_set(
		*m_context,
		cmd_buf,
		res_container,
		k_position_buffer_descriptor_set_idx,
		[&](VkDescriptorSet desc_set) {
			vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) desc_set, "meshlet_buffer");
			write_meshlet_buffer_descriptor_set(*m_context, desc_set, vertex_buffer, index_buffer, meshlet_buffer);
		}
	);

	/* Light array */
	m_pipeline.acquire_and_bind_descriptor_set(
		*m_context,
		cmd_buf,
		res_container,
		k_light_array_descriptor_set_idx,
		[&](VkDescriptorSet desc_set){
			vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) desc_set, "light_array");
			m_renderer->write_light_array_descriptor_set(frame_idx, desc_set);
		});

	/* */
	vkCmdDrawMeshTasksNV(cmd_buf, (uint32_t) meshlet_count, 0);
}
