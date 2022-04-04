#include "shader.hpp"

#include <fstream>

#include <spirv_reflect.h>

#include "context.hpp"
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
	std::shared_ptr<vren::context> const& ctx,
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
	vren::vk_utils::check(vkCreateShaderModule(ctx->m_device, &shader_info, nullptr, &shader_mod));
	return vren::vk_shader_module(ctx, shader_mod);
}

vren::vk_shader_module vren::vk_utils::load_shader_module(
	std::shared_ptr<vren::context> const& ctx,
	char const* shad_path
)
{
	std::vector<char> buf;
	load_bin_file(shad_path, buf);
	return create_shader_module(ctx, buf.size(), reinterpret_cast<uint32_t*>(buf.data()));
}

// --------------------------------------------------------------------------------------------------------------------------------
// compute_pipeline
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

vren::vk_descriptor_set_layout create_descriptor_set_layout(
	std::shared_ptr<vren::context> const& ctx,
	VkShaderStageFlags shader_stage,
	SpvReflectDescriptorSet* spv_refl_desc_set
)
{
	using std::cout;
	using std::endl;

	printf("Descriptor set #%d (bindings: %d)\n", spv_refl_desc_set->set, spv_refl_desc_set->binding_count);

	std::vector<VkDescriptorSetLayoutBinding> bindings;
	for (uint32_t i = 0; i < spv_refl_desc_set->binding_count; i++)
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
		fflush(stdout);
	}

	VkDescriptorSetLayoutCreateInfo desc_set_layout_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.bindingCount = (uint32_t) bindings.size(),
		.pBindings = bindings.data()
	};

	VkDescriptorSetLayout desc_set_layout;
	vkCreateDescriptorSetLayout(ctx->m_device, &desc_set_layout_info, nullptr, &desc_set_layout);
	return vren::vk_descriptor_set_layout(ctx, desc_set_layout);
}

vren::vk_utils::self_described_shader
vren::vk_utils::create_and_describe_shader(
	std::shared_ptr<vren::context> const& ctx,
	size_t spv_code_size,
	uint32_t* spv_code
)
{
	SpvReflectShaderModule module;
	spirv_reflect_check(
		spvReflectCreateShaderModule(spv_code_size, spv_code, &module)
	);

	VkShaderStageFlagBits shader_stage = parse_shader_stage(module.shader_stage);

	uint32_t num;

	printf("----------------------------------------------------------------\n");
	printf("Shader: %s (type: %d)\n", module.source_file, module.shader_stage);
	printf("----------------------------------------------------------------\n");

	/* Descriptor set layouts */
	spirv_reflect_check(spvReflectEnumerateDescriptorSets(&module, &num, nullptr));

	std::vector<SpvReflectDescriptorSet*> spv_refl_desc_sets(num);
	spirv_reflect_check(spvReflectEnumerateDescriptorSets(&module, &num, spv_refl_desc_sets.data()));

	std::unordered_map<uint32_t, vren::vk_descriptor_set_layout> desc_set_layouts;

	for (int i = 0; i < spv_refl_desc_sets.size(); i++)
	{
		desc_set_layouts.emplace(
			spv_refl_desc_sets.at(i)->set,
			create_descriptor_set_layout(ctx, shader_stage, spv_refl_desc_sets.at(i))
		);
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
			.stageFlags = static_cast<VkShaderStageFlags>(shader_stage),
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
		.m_descriptor_set_layouts = std::move(desc_set_layouts),
		.m_push_constant_ranges = std::move(push_constant_ranges)
	};
}

vren::vk_utils::self_described_shader
vren::vk_utils::load_and_describe_shader(
	std::shared_ptr<vren::context> const& ctx,
	char const* shad_path
)
{
	std::vector<char> buf;
	load_bin_file(shad_path, buf);
	return create_and_describe_shader(ctx, buf.size(), reinterpret_cast<uint32_t*>(buf.data()));
}

vren::vk_utils::self_described_compute_pipeline
vren::vk_utils::create_compute_pipeline(
	std::shared_ptr<vren::context> const& ctx,
	std::shared_ptr<self_described_shader> const& comp_shad
)
{
	/* Descriptor set layouts */
	auto desc_set_layouts = comp_shad->get_descriptor_set_layouts();

	/* Push constant ranges */
	auto& push_const_ranges = comp_shad->m_push_constant_ranges;

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
		vkCreatePipelineLayout(ctx->m_device, &pipeline_layout_info, nullptr, &pipeline_layout)
	);

	/* Pipeline */
	VkComputePipelineCreateInfo pipeline_info{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.stage = VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = NULL,
			.stage = comp_shad->m_stage,
			.module = comp_shad->m_module.m_handle,
			.pName = comp_shad->m_entry_point.c_str(),
			.pSpecializationInfo = nullptr
		},
		.layout = pipeline_layout,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = 0,
	};
	VkPipeline pipeline;
	vren::vk_utils::check(
		vkCreateComputePipelines(ctx->m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline)
	);

	return vren::vk_utils::self_described_compute_pipeline{
		.m_pipeline = vren::vk_pipeline(ctx, pipeline),
		.m_pipeline_layout = vren::vk_pipeline_layout(ctx, pipeline_layout),
	  	.m_shader = std::move(comp_shad)
	};
}

vren::vk_utils::self_described_graphics_pipeline
vren::vk_utils::create_graphics_pipeline(
	std::shared_ptr<vren::context> const& ctx,
	std::vector<std::shared_ptr<self_described_shader>> const& shaders,
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
	std::vector<VkPipelineShaderStageCreateInfo> shad_stages;
	std::vector<VkDescriptorSetLayout> desc_set_layouts;
	std::vector<VkPushConstantRange> push_const_ranges;

	for (auto& shad : shaders)
	{
		/* Shader stage */
		shad_stages.push_back({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = NULL,
			.stage = shad->m_stage,
		    .module = shad->m_module.m_handle,
			.pName = shad->m_entry_point.c_str(),
			.pSpecializationInfo = nullptr
		});

		/* Descriptor set layouts */
		auto shader_desc_set_layouts = shad->get_descriptor_set_layouts();
		desc_set_layouts.insert(desc_set_layouts.end(), shader_desc_set_layouts.begin(), shader_desc_set_layouts.end());

		/* Push constant ranges */
		push_const_ranges.insert(push_const_ranges.end(), shad->m_push_constant_ranges.begin(), shad->m_push_constant_ranges.end());
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
	vren::vk_utils::check(vkCreatePipelineLayout(ctx->m_device, &pipeline_layout_info, nullptr, &pipeline_layout));

	/* Graphics pipeline */
	VkGraphicsPipelineCreateInfo pipeline_info{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.stageCount = (uint32_t) shad_stages.size(),
		.pStages = shad_stages.data(),
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
	vren::vk_utils::check(vkCreateGraphicsPipelines(ctx->m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));

	/* */

	return {
		.m_pipeline = vren::vk_pipeline(ctx, pipeline),
		.m_pipeline_layout = vren::vk_pipeline_layout(ctx, pipeline_layout),
		.m_shaders = std::move(shaders)
	};
}

