#include "mesh_shader_draw_pass.hpp"

#include "context.hpp"
#include "toolbox.hpp"
#include "mesh_shader_renderer.hpp"
#include "vk_helpers/shader.hpp"
#include "vk_helpers/debug_utils.hpp"

vren::mesh_shader_draw_pass::mesh_shader_draw_pass(
	vren::context const& context,
	bool occlusion_culling
) :
	m_context(&context),
	m_pipeline(create_graphics_pipeline(occlusion_culling))
{}

vren::pipeline vren::mesh_shader_draw_pass::create_graphics_pipeline(bool occlusion_culling)
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
		.depthTestEnable = true,
		.depthWriteEnable = true,
		.depthCompareOp = VK_COMPARE_OP_LESS
	};

	/* Color blend state */
	VkPipelineColorBlendAttachmentState color_blend_attachments[]{
		{.blendEnable = VK_FALSE, },
		{.blendEnable = VK_FALSE, },
		{.blendEnable = VK_FALSE, },
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

	// Pipeline rendering
	VkFormat color_attachment_formats[]{
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R16_UINT,
	};
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
	vren::shader_module task_shader_mod = vren::load_shader_module_from_file(*m_context, ".vren/resources/shaders/draw.task.spv");
	vren::shader_module mesh_shader_mod = vren::load_shader_module_from_file(*m_context, ".vren/resources/shaders/draw.mesh.spv");
	vren::shader_module frag_shader_mod = vren::load_shader_module_from_file(*m_context, ".vren/resources/shaders/deferred.frag.spv");

	vren::specialized_shader task_shader = vren::specialized_shader(task_shader_mod, "main");
	task_shader.set_specialization_data("OCCLUSION_CULLING", &occlusion_culling, sizeof(occlusion_culling));

	vren::specialized_shader mesh_shader = vren::specialized_shader(mesh_shader_mod, "main");
	vren::specialized_shader frag_shader = vren::specialized_shader(frag_shader_mod, "main");
	
	vren::specialized_shader shaders[] = {
		std::move(task_shader),
		std::move(mesh_shader),
		std::move(frag_shader)
	};
	return vren::create_graphics_pipeline(
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
		&pipeline_rendering_info,
		VK_NULL_HANDLE,
		0
	);
}

void write_meshlet_buffer_descriptor_set(
	vren::context const& context,
	VkDescriptorSet descriptor_set,
	VkBuffer vertex_buffer,
	VkBuffer meshlet_vertex_buffer,
	VkBuffer meshlet_triangle_buffer,
	VkBuffer meshlet_buffer,
	VkBuffer instanced_meshlet_buffer,
	VkBuffer instance_buffer
)
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
		},
		{ // Instanced meshlet buffer
			.buffer = instanced_meshlet_buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE
		},
		{ // Instance buffer
			.buffer = instance_buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE
		}
	};

	for (uint32_t i = 0; i < std::size(buffer_info); i++)
	{
		VkWriteDescriptorSet descriptor_set_write{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptor_set,
			.dstBinding = i,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &buffer_info[i]
		};
		vkUpdateDescriptorSets(context.m_device, 1, &descriptor_set_write, 0, nullptr);
	}
}

void write_depth_buffer_pyramid_descriptor_set(
	vren::context const& context,
	VkDescriptorSet descriptor_set,
	vren::depth_buffer_pyramid const& depth_buffer_pyramid
)
{
	VkDescriptorImageInfo image_info[]{
		{
			.sampler = depth_buffer_pyramid.get_sampler(),
			.imageView = depth_buffer_pyramid.get_image_view(),
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		}
	};
	VkWriteDescriptorSet descriptor_set_write{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptor_set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = std::size(image_info),
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = image_info
	};
	vkUpdateDescriptorSets(context.m_device, 1, &descriptor_set_write, 0, nullptr);
}

void vren::mesh_shader_draw_pass::render(
	uint32_t frame_idx,
	VkCommandBuffer command_buffer,
	vren::resource_container& resource_container,
	vren::camera_data const& camera_data,
	vren::clusterized_model_draw_buffer const& draw_buffer,
	vren::light_array const& light_array,
	vren::depth_buffer_pyramid const& depth_buffer_pyramid
)
{
	m_pipeline.bind(command_buffer);

	// Push constants
	m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_TASK_BIT_NV | VK_SHADER_STAGE_MESH_BIT_NV, &camera_data, sizeof(camera_data));

	// Bind draw buffer
	m_pipeline.acquire_and_bind_descriptor_set(*m_context, command_buffer, resource_container, 2, [&](VkDescriptorSet descriptor_set)
	{
		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_set, "meshlet_buffer");
		write_meshlet_buffer_descriptor_set(
			*m_context,
			descriptor_set,
			draw_buffer.m_vertex_buffer.m_buffer.m_handle,
			draw_buffer.m_meshlet_vertex_buffer.m_buffer.m_handle,
			draw_buffer.m_meshlet_triangle_buffer.m_buffer.m_handle,
			draw_buffer.m_meshlet_buffer.m_buffer.m_handle,
			draw_buffer.m_instanced_meshlet_buffer.m_buffer.m_handle,
			draw_buffer.m_instance_buffer.m_buffer.m_handle
		);
	});

	// Bind depth-buffer pyramid
	m_pipeline.acquire_and_bind_descriptor_set(*m_context, command_buffer, resource_container, 4, [&](VkDescriptorSet descriptor_set)
	{
		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_set, "depth_buffer_pyramid");
		write_depth_buffer_pyramid_descriptor_set(
			*m_context,
			descriptor_set,
			depth_buffer_pyramid
		);
	});

	// Draw
	uint32_t workgroups_num = (uint32_t) glm::ceil(draw_buffer.m_instanced_meshlet_count / (float) 32);
	vkCmdDrawMeshTasksNV(command_buffer, workgroups_num, 0);
}
