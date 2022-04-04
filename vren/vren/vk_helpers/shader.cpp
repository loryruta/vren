#include "shader.hpp"

#include <fstream>

#include <spirv_reflect.h>

#include "context.hpp"
#include "toolbox.hpp"
#include "misc.hpp"

void load_bin_file(char const* file_path, std::vector<char>& buf)
{
	std::ifstream f(file_path, std::ios::ate | std::ios::binary);
	if (!f.is_open()) {
		throw std::runtime_error("Failed to open file");
	}

	auto file_size = f.tellg();
	buf.resize((size_t) glm::ceil(file_size / (float) sizeof(uint32_t)) * sizeof(uint32_t)); // Rounds to a multiple of 4 because codeSize requires it

	f.seekg(0);
	f.read(buf.data(), file_size);

	f.close();
}

vren::vk_shader_module vren::vk_utils::create_shader_module(
	vren::context const& ctx,
	size_t code_size,
	uint32_t* code
)
{
	VkShaderModuleCreateInfo shader_info{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.codeSize = code_size,
		.pCode = code
	};

	VkShaderModule shader_mod;
	vren::vk_utils::check(vkCreateShaderModule(ctx.m_device, &shader_info, nullptr, &shader_mod));
	return vren::vk_shader_module(ctx, shader_mod);
}

vren::vk_shader_module vren::vk_utils::load_shader_module(
	vren::context const& ctx,
	char const* shad_path
)
{
	std::vector<char> buf;
	load_bin_file(shad_path, buf);
	return create_shader_module(ctx, buf.size(), reinterpret_cast<uint32_t*>(buf.data()));
}

// --------------------------------------------------------------------------------------------------------------------------------
// Shader
// --------------------------------------------------------------------------------------------------------------------------------

void spirv_reflect_check(SpvReflectResult res)
{
	assert(res == SPV_REFLECT_RESULT_SUCCESS);
}

VkShaderStageFlagBits parse_shader_stage(SpvReflectShaderStageFlagBits spv_refl_shader_stage)
{
	return static_cast<VkShaderStageFlagBits>(spv_refl_shader_stage);
}

VkDescriptorType parse_descriptor_type(SpvReflectDescriptorType spv_refl_desc_type)
{
	return static_cast<VkDescriptorType>(spv_refl_desc_type);
}

std::unordered_map<uint32_t, vren::vk_descriptor_set_layout>
create_descriptor_set_layouts(
	vren::context const& ctx,
	std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> bindings_by_set_idx
)
{
	std::unordered_map<uint32_t, vren::vk_descriptor_set_layout> desc_set_layout_by_idx;

	for (auto& [desc_set_idx, bindings] : bindings_by_set_idx)
	{
		VkDescriptorSetLayoutCreateInfo desc_set_layout_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = NULL,
			.bindingCount = (uint32_t) bindings.size(),
			.pBindings = bindings.data(),
		};
		VkDescriptorSetLayout desc_set_layout;
		vkCreateDescriptorSetLayout(ctx.m_device, &desc_set_layout_info, nullptr, &desc_set_layout);
	}

	return desc_set_layout_by_idx;
}

vren::vk_utils::shader
vren::vk_utils::load_shader(
	vren::context const& ctx,
	size_t spv_code_size,
	uint32_t* spv_code
)
{
	SpvReflectShaderModule module;
	spirv_reflect_check(
		spvReflectCreateShaderModule(spv_code_size, spv_code, &module)
	);

	VkShaderStageFlags shader_stage = parse_shader_stage(module.shader_stage);

	uint32_t num;

	printf("----------------------------------------------------------------\n");
	printf("Shader: %s (type: %d)\n", module.source_file, module.shader_stage);
	printf("----------------------------------------------------------------\n");

	/* Descriptor sets */
	spirv_reflect_check(spvReflectEnumerateDescriptorSets(&module, &num, nullptr));

	std::vector<SpvReflectDescriptorSet*> spv_refl_desc_sets(num);
	spirv_reflect_check(spvReflectEnumerateDescriptorSets(&module, &num, spv_refl_desc_sets.data()));

	std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> descriptors_by_set_idx;

	for (int i = 0; i < spv_refl_desc_sets.size(); i++)
	{
		auto spv_refl_desc_set = spv_refl_desc_sets.at(i);

		printf("Descriptor set #%d (bindings: %d)\n", spv_refl_desc_set->set, spv_refl_desc_set->binding_count);

		std::vector<VkDescriptorSetLayoutBinding> bindings{};
		for (uint32_t j = 0; j < spv_refl_desc_set->binding_count; j++)
		{
			auto binding = spv_refl_desc_set->bindings[i];
			bindings.push_back({
				.binding = binding->binding,
				.descriptorType = parse_descriptor_type(binding->descriptor_type),
				.descriptorCount = binding->count,
				.stageFlags = shader_stage,
				.pImmutableSamplers = nullptr
			});

			printf("- Binding #%d (count: %d): %s\n", binding->binding, binding->count, binding->name);
		}

		descriptors_by_set_idx.emplace(spv_refl_desc_sets.at(i)->set, bindings);
	}

	/* Push constants */
	spirv_reflect_check(spvReflectEnumeratePushConstantBlocks(&module, &num, nullptr));

	std::vector<SpvReflectBlockVariable*> spv_refl_block_vars(num);
	spirv_reflect_check(spvReflectEnumeratePushConstantBlocks(&module, &num, spv_refl_block_vars.data()));

	std::vector<VkPushConstantRange> push_constant_ranges;
	for (int i = 0; i < spv_refl_block_vars.size(); i++)
	{
		auto spv_refl_block_var = spv_refl_block_vars.at(i);
		push_constant_ranges.push_back({
			.stageFlags = shader_stage,
			.offset = spv_refl_block_var->absolute_offset,
			.size = spv_refl_block_var->padded_size
		});
	}

	/* */

	spvReflectDestroyShaderModule(&module);

	return {
		.m_module = create_shader_module(ctx, spv_code_size, spv_code),
		.m_stage = shader_stage,
		.m_entry_point = "main",
		.m_bindings_by_set_idx = std::move(descriptors_by_set_idx),
		.m_push_constant_ranges = std::move(push_constant_ranges)
	};
}

vren::vk_utils::shader
vren::vk_utils::load_shader_from_file(
	vren::context const& ctx,
	char const* shad_path
)
{
	std::vector<char> buf;
	load_bin_file(shad_path, buf);
	return load_shader(ctx, buf.size(), reinterpret_cast<uint32_t*>(buf.data()));
}

// --------------------------------------------------------------------------------------------------------------------------------
// Pipeline
// --------------------------------------------------------------------------------------------------------------------------------

VkDescriptorSetLayout vren::vk_utils::pipeline::get_descriptor_set_layout(VkShaderStageFlags shader_stage, uint32_t desc_set_idx)
{
	return m_descriptor_set_layouts_table.at(shader_stage).at(desc_set_idx).m_handle;
}

void vren::vk_utils::pipeline::bind(VkCommandBuffer cmd_buf)
{
	vkCmdBindPipeline(cmd_buf, m_bind_point, m_pipeline.m_handle);
}

void vren::vk_utils::pipeline::bind_descriptor_set(VkCommandBuffer cmd_buf, uint32_t desc_set_idx, VkDescriptorSet desc_set)
{
	vkCmdBindDescriptorSets(cmd_buf, m_bind_point, m_pipeline_layout.m_handle, desc_set_idx, 1, &desc_set, 0, nullptr);
}

void vren::vk_utils::pipeline::push_constants(VkCommandBuffer cmd_buf, VkShaderStageFlags shader_stage, void const* val, uint32_t val_size, uint32_t val_off)
{
	vkCmdPushConstants(cmd_buf, m_pipeline_layout.m_handle, shader_stage, val_off, val_size, val);
}

void vren::vk_utils::pipeline::acquire_and_bind_descriptor_set(
	vren::context const& ctx,
	VkCommandBuffer cmd_buf,
	vren::resource_container& res_container,
	VkShaderStageFlags shader_stage,
	uint32_t desc_set_idx,
	std::function<void(VkDescriptorSet)> const& update_func
)
{
	auto desc_set = std::make_shared<vren::pooled_vk_descriptor_set>(
		ctx.m_toolbox->m_descriptor_pool.acquire(get_descriptor_set_layout(shader_stage, desc_set_idx))
	);
	update_func(desc_set->m_handle.m_descriptor_set);
	bind_descriptor_set(cmd_buf, desc_set_idx, desc_set->m_handle.m_descriptor_set);
	res_container.add_resources(desc_set);
}

vren::vk_utils::pipeline
vren::vk_utils::create_compute_pipeline(vren::context const& ctx, shader const& comp_shader)
{
	/* Descriptor set layouts */
	pipeline::descriptor_set_layout_table_t desc_set_layouts_table = {
		{VK_SHADER_STAGE_COMPUTE_BIT, create_descriptor_set_layouts(ctx, comp_shader.m_bindings_by_set_idx)}
	};
	std::vector<VkDescriptorSetLayout> desc_set_layouts;
	for (auto& [shader_stage, row] : desc_set_layouts_table) {
		for (auto& [desc_set_idx, desc_set_layout] : row) {
			desc_set_layouts.push_back(desc_set_layout.m_handle);
		}
	}

	/* Push constant ranges */
	auto& push_const_ranges = comp_shader.m_push_constant_ranges;

	/* Pipeline layout */
	VkPipelineLayoutCreateInfo pipeline_layout_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.setLayoutCount = (uint32_t) desc_set_layouts.size(),
		.pSetLayouts = desc_set_layouts.data(),
		.pushConstantRangeCount = (uint32_t) push_const_ranges.size(),
		.pPushConstantRanges = push_const_ranges.data(),
	};
	VkPipelineLayout pipeline_layout;
	vren::vk_utils::check(
		vkCreatePipelineLayout(ctx.m_device, &pipeline_layout_info, nullptr, &pipeline_layout)
	);

	/* Pipeline */
	VkComputePipelineCreateInfo pipeline_info{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = NULL,
			.stage = static_cast<VkShaderStageFlagBits>(comp_shader.m_stage),
			.module = comp_shader.m_module.m_handle,
			.pName = comp_shader.m_entry_point.c_str(),
			.pSpecializationInfo = nullptr
		},
		.layout = pipeline_layout,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = 0,
	};
	VkPipeline pipeline;
	vren::vk_utils::check(
		vkCreateComputePipelines(ctx.m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline)
	);

	return vren::vk_utils::pipeline{
		.m_pipeline = vren::vk_pipeline(ctx, pipeline),
		.m_pipeline_layout = vren::vk_pipeline_layout(ctx, pipeline_layout),
		.m_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE,
		.m_descriptor_set_layouts_table = desc_set_layouts_table,
	};
}

vren::vk_utils::pipeline
vren::vk_utils::create_graphics_pipeline(
	vren::context const& ctx,
	std::vector<shader> const& shaders,
	VkPipelineVertexInputStateCreateInfo* vtx_input_state_info,
	VkPipelineInputAssemblyStateCreateInfo* input_assembly_state_info,
	VkPipelineTessellationStateCreateInfo* tessellation_state_info,
	VkPipelineViewportStateCreateInfo* viewport_state_info,
	VkPipelineRasterizationStateCreateInfo* rasterization_state_info,
	VkPipelineMultisampleStateCreateInfo* multisample_state_info,
	VkPipelineDepthStencilStateCreateInfo* depth_stencil_state_info,
	VkPipelineColorBlendStateCreateInfo* color_blend_state_info,
	VkPipelineDynamicStateCreateInfo* dynamic_state_info,
	VkRenderPass render_pass,
	uint32_t subpass
)
{
	/* Descriptor set layouts */
	pipeline::descriptor_set_layout_table_t desc_set_layouts_table{};
	for (auto& shader : shaders) {
		desc_set_layouts_table.emplace(shader.m_stage, create_descriptor_set_layouts(ctx, shader.m_bindings_by_set_idx));
	}
	std::vector<VkDescriptorSetLayout> desc_set_layouts;
	for (auto& [shader_stage, row] : desc_set_layouts_table) {
		for (auto& [desc_set_idx, desc_set_layout] : row) {
			desc_set_layouts.push_back(desc_set_layout.m_handle);
		}
	}

	/* Shader stages */
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};
	for (auto& shader : shaders) {
		shader_stages.push_back({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = NULL,
			.stage = static_cast<VkShaderStageFlagBits>(shader.m_stage),
			.module = shader.m_module.m_handle,
			.pName = shader.m_entry_point.c_str(),
			.pSpecializationInfo = nullptr
		});
	}

	/* Push constant ranges */
	std::vector<VkPushConstantRange> push_const_ranges;
	for (auto& shader : shaders) {
		push_const_ranges.insert(
			push_const_ranges.end(),
			shader.m_push_constant_ranges.begin(),
			shader.m_push_constant_ranges.end()
		);
	}

	/* Pipeline layout */
	VkPipelineLayoutCreateInfo pipeline_layout_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.setLayoutCount = (uint32_t) desc_set_layouts.size(),
		.pSetLayouts = desc_set_layouts.data(),
		.pushConstantRangeCount = (uint32_t) push_const_ranges.size(),
		.pPushConstantRanges = push_const_ranges.data(),
	};
	VkPipelineLayout pipeline_layout;
	vren::vk_utils::check(vkCreatePipelineLayout(ctx.m_device, &pipeline_layout_info, nullptr, &pipeline_layout));

	/* Graphics pipeline */
	VkGraphicsPipelineCreateInfo pipeline_info{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.stageCount = (uint32_t) shader_stages.size(),
		.pStages = shader_stages.data(),
		.pVertexInputState = vtx_input_state_info,
		.pInputAssemblyState = input_assembly_state_info,
		.pTessellationState = tessellation_state_info,
		.pViewportState = viewport_state_info,
		.pRasterizationState = rasterization_state_info,
		.pMultisampleState = multisample_state_info,
		.pDepthStencilState = depth_stencil_state_info,
		.pColorBlendState = color_blend_state_info,
		.pDynamicState = dynamic_state_info,
		.layout = pipeline_layout,
		.renderPass = render_pass, // Shouldn't render_pass lifetime be managed ALSO by the graphics pipeline class?
		.subpass = subpass
	};
	VkPipeline pipeline;
	vren::vk_utils::check(vkCreateGraphicsPipelines(ctx.m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));

	/* */

	return {
		.m_pipeline = vren::vk_pipeline(ctx, pipeline),
		.m_pipeline_layout = vren::vk_pipeline_layout(ctx, pipeline_layout),
		.m_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.m_descriptor_set_layouts_table = desc_set_layouts_table
	};
}
