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

VkShaderStageFlags parse_shader_stage(SpvReflectShaderStageFlagBits spv_refl_shader_stage)
{
	return static_cast<VkShaderStageFlags>(spv_refl_shader_stage);
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
	std::vector<VkDescriptorSetLayoutBinding> bindings(spv_refl_desc_set->binding_count);

	for (uint32_t i = 0; i < spv_refl_desc_set->binding_count; i++)
	{
		auto binding = spv_refl_desc_set->bindings[i];
		bindings[i] = {
			.binding = binding->binding,
			.descriptorType = parse_descriptor_type(binding->descriptor_type),
			.descriptorCount = binding->count,
			.stageFlags = shader_stage,
			.pImmutableSamplers = nullptr
		};
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

	VkShaderStageFlags shader_stage = parse_shader_stage(module.shader_stage);

	uint32_t num;

	/* Descriptor set layouts */
	spirv_reflect_check(spvReflectEnumerateDescriptorSets(&module, &num, nullptr));

	std::vector<SpvReflectDescriptorSet*> spv_refl_desc_sets(num);
	spirv_reflect_check(spvReflectEnumerateDescriptorSets(&module, &num, spv_refl_desc_sets.data()));

	std::vector<vren::vk_descriptor_set_layout> desc_set_layouts(spv_refl_desc_sets.size());
	for (int i = 0; i < spv_refl_desc_sets.size(); i++)
	{
		desc_set_layouts[spv_refl_desc_sets.at(i)->set] =
			create_descriptor_set_layout(ctx, shader_stage, spv_refl_desc_sets.at(i));
	}

	/* Push constants */
	spirv_reflect_check(spvReflectEnumeratePushConstantBlocks(&module, &num, nullptr));

	std::vector<SpvReflectBlockVariable*> spv_refl_block_vars(num);
	spirv_reflect_check(spvReflectEnumeratePushConstantBlocks(&module, &num, spv_refl_block_vars.data()));

	std::vector<VkPushConstantRange> push_constant_ranges(spv_refl_block_vars.size());
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
		.m_shader_module = create_shader_module(ctx, spv_code_size, spv_code),
		.m_descriptor_set_layouts = desc_set_layouts,
		.m_push_constant_ranges = push_constant_ranges
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
	self_described_shader&& comp_shader
)
{
	/* Descriptor set layouts */
	std::vector<VkDescriptorSetLayout> desc_set_layouts(
		comp_shader.m_descriptor_set_layouts.size()
	);
	for (auto& desc_set_layout : comp_shader.m_descriptor_set_layouts) {
		desc_set_layouts.push_back(desc_set_layout.m_handle);
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
		vkCreatePipelineLayout(ctx->m_device, &pipeline_layout_info, nullptr, &pipeline_layout)
	);

	/* Pipeline */
	VkComputePipelineCreateInfo pipeline_info{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = comp_shader.m_shader_module.m_handle,
			.pName = "main",
			.pSpecializationInfo = nullptr,
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
	  	.m_shader = std::move(comp_shader)
	};
}

