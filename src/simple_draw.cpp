#include "simple_draw.hpp"

#include "renderer.hpp"

#include <fstream>
#include <iostream>

#include <glm/gtc/type_ptr.hpp>

#define UBO_CAMERA_BINDING 0
#define UBO_CAMERA_SIZE (sizeof(float) * (16 + 16))

#define VREN_DRAW_MATERIAL_TEXTURES_BINDING 1


// ------------------------------------------------------------------------------------------------ simple_draw_pass

vren::simple_draw_pass::simple_draw_pass(vren::renderer& renderer) :
	m_renderer(renderer)
{
	create_graphics_pipeline();
}

vren::simple_draw_pass::~simple_draw_pass()
{
	vkDestroyPipelineLayout(m_renderer.m_device, m_pipeline_layout, nullptr);
	vkDestroyPipeline(m_renderer.m_device, m_graphics_pipeline, nullptr);
}

VkShaderModule create_shader_module(VkDevice device, char const* path)
{
	// Read file
	std::ifstream f(path, std::ios::ate | std::ios::binary);
	if (!f.is_open()) {
		throw std::runtime_error("Failed to open file");
	}

	auto file_size = f.tellg();
	std::vector<char> buffer(file_size);

	f.seekg(0);
	f.read(buffer.data(), file_size);

	f.close();

	// Create shader
	VkShaderModuleCreateInfo shader_info{};
	shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_info.codeSize = buffer.size();
	shader_info.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

	VkShaderModule shader_module;
	vren::vk_utils::check(vkCreateShaderModule(device, &shader_info, nullptr, &shader_module));

	return shader_module;
}

void vren::simple_draw_pass::create_graphics_pipeline()
{
	// Shader stages
	std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

	VkPipelineShaderStageCreateInfo shader_stage_info{};
	shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stage_info.module = create_shader_module(m_renderer.m_device, "./.vren/resources/simple_draw.vert.bin");
	shader_stage_info.pName = "main";
	shader_stage_infos.push_back(shader_stage_info);

	shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stage_info.module =  create_shader_module(m_renderer.m_device, "./.vren/resources/simple_draw.frag.bin");
	shader_stage_info.pName = "main";
	shader_stage_infos.push_back(shader_stage_info);

	//
	VkVertexInputBindingDescription vtx_binding{};
	vtx_binding.binding = 0;
	vtx_binding.stride = sizeof(vren::vertex);
	vtx_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputBindingDescription instance_data_binding{};
	instance_data_binding.binding = 1;
	instance_data_binding.stride = sizeof(vren::instance_data);
	instance_data_binding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

	auto binding_descriptions = render_object::get_all_binding_desc();
	auto attribute_descriptions = render_object::get_all_attrib_desc();

	VkPipelineVertexInputStateCreateInfo vtx_input_info{};
	vtx_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vtx_input_info.vertexBindingDescriptionCount = binding_descriptions.size();
	vtx_input_info.pVertexBindingDescriptions = binding_descriptions.data();
	vtx_input_info.vertexAttributeDescriptionCount = attribute_descriptions.size();
	vtx_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = VK_FALSE;

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterization_state_info{};
	rasterization_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state_info.cullMode = VK_CULL_MODE_NONE;
	rasterization_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_state_info.lineWidth = 1.0f;

	// Multisampling state
	VkPipelineMultisampleStateCreateInfo multisampling_info{};
	multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling_info.sampleShadingEnable = VK_FALSE;
	multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
	depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_info.depthTestEnable = VK_TRUE;
	depth_stencil_info.depthWriteEnable = VK_TRUE;
	depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_info.minDepthBounds = 0.0f;
	depth_stencil_info.maxDepthBounds = 1.0f;
	depth_stencil_info.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo color_blend_info{};
	color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_info.logicOpEnable = VK_FALSE;
	color_blend_info.attachmentCount = 1;
	color_blend_info.pAttachments = &color_blend_attachment;

	std::vector<VkDescriptorSetLayout> descriptor_set_layouts = {
		m_renderer.m_descriptor_set_pool->m_material_layout,
		m_renderer.m_descriptor_set_pool->m_lights_array_layout
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = descriptor_set_layouts.size();
	pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();

	std::vector<VkPushConstantRange> push_constants;

	// Camera
	VkPushConstantRange push_constant{};
	push_constant.offset = 0;
	push_constant.size = sizeof(float) * (16 + 16);
	push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	push_constants.push_back(push_constant);

	pipeline_layout_info.pPushConstantRanges = push_constants.data();
	pipeline_layout_info.pushConstantRangeCount = (uint32_t) push_constants.size();

	vren::vk_utils::check(vkCreatePipelineLayout(m_renderer.m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout));

	// Viewport state
	VkPipelineViewportStateCreateInfo viewport_state_info{};
	viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_info.viewportCount = 1;
	viewport_state_info.scissorCount = 1;

	// Dynamic states
	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamic_state_info{};
	dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.dynamicStateCount = (uint32_t) dynamic_states.size();
	dynamic_state_info.pDynamicStates = dynamic_states.data();

	// Graphics pipeline
	VkGraphicsPipelineCreateInfo graphics_pipeline_info{};
	graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_info.stageCount = shader_stage_infos.size();
	graphics_pipeline_info.pStages = shader_stage_infos.data();
	graphics_pipeline_info.pVertexInputState = &vtx_input_info;
	graphics_pipeline_info.pInputAssemblyState = &input_assembly_info;
	graphics_pipeline_info.pViewportState = &viewport_state_info;
	graphics_pipeline_info.pDynamicState = &dynamic_state_info;
	graphics_pipeline_info.pRasterizationState = &rasterization_state_info;
	graphics_pipeline_info.pMultisampleState = &multisampling_info;
	graphics_pipeline_info.pDepthStencilState = &depth_stencil_info;
	graphics_pipeline_info.pColorBlendState = &color_blend_info;
	graphics_pipeline_info.layout = m_pipeline_layout;
	graphics_pipeline_info.renderPass = m_renderer.m_render_pass;
	graphics_pipeline_info.subpass = 0;
	graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

	vren::vk_utils::check(vkCreateGraphicsPipelines(m_renderer.m_device, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &m_graphics_pipeline));

	//vkDestroyShaderModule(m_renderer.m_device, vert_shader_mod, nullptr);
	//vkDestroyShaderModule(m_renderer.m_device, frag_shader_mod, nullptr);
}

void vren::simple_draw_pass::record_commands(
	vren::frame& frame,
	vren::render_list const& render_list,
	vren::lights_array const& lights_array,
	vren::camera const& camera
)
{
	vkCmdBindPipeline(frame.m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);

	VkDescriptorSet descriptor_set;

	// Camera
	vkCmdPushConstants(
		frame.m_command_buffer,
		m_pipeline_layout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(vren::camera),
		&camera
	);

	for (size_t i = 0; i < render_list.m_render_objects.size(); i++)
	{
		auto& render_obj = render_list.m_render_objects.at(i);

		if (!render_obj.is_valid())
		{
			printf("WARNING: Render object %d is invalid.\n", render_obj.m_idx);
			continue;
		}

		VkDeviceSize offsets[] = { 0 };

		// Vertex buffer
		vkCmdBindVertexBuffers(
			frame.m_command_buffer,
			0,
			1,
			&render_obj.m_vertex_buffer.m_buffer,
			offsets
		);

		// Indices buffer
		vkCmdBindIndexBuffer(
			frame.m_command_buffer,
			render_obj.m_indices_buffer.m_buffer,
			0,
			vren::render_object::s_index_type
		);

		// Instances buffer
		vkCmdBindVertexBuffers(
			frame.m_command_buffer,
			1,
			1,
			&render_obj.m_instances_buffer.m_buffer,
			offsets
		);

		// Material
		descriptor_set = frame.acquire_material_descriptor_set();
		vkCmdBindDescriptorSets(
			frame.m_command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipeline_layout,
			VREN_MATERIAL_DESCRIPTOR_SET,
			1,
			&descriptor_set,
			0,
			nullptr
		);
		m_renderer.m_material_manager->update_material_descriptor_set(
			render_obj.m_material,
			descriptor_set
		);

		// Lights array
		descriptor_set = frame.acquire_lights_array_descriptor_set();
		vkCmdBindDescriptorSets(
			frame.m_command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipeline_layout,
			VREN_LIGHTS_ARRAY_DESCRIPTOR_SET,
			1,
			&descriptor_set,
			0,
			nullptr
		);
		lights_array.update_descriptor_set(descriptor_set);

		vkCmdDrawIndexed(
			frame.m_command_buffer,
			render_obj.m_indices_count,
			render_obj.m_instances_count,
			0,
			0,
			0
		);
	}
}
