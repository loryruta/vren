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

char const* descriptor_type_name(VkDescriptorType desc_type)
{
	switch (desc_type)
	{
	case VK_DESCRIPTOR_TYPE_SAMPLER: return "VK_DESCRIPTOR_TYPE_SAMPLER";
	case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
	case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
	case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
	case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
	case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
	case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
	default: return "?";
	}
}

char const* shader_stage_name(VkShaderStageFlags shader_stage)
{
	switch (shader_stage)
	{
	case VK_SHADER_STAGE_VERTEX_BIT: return "VK_SHADER_STAGE_VERTEX_BIT";
	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT";
	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT";
	case VK_SHADER_STAGE_GEOMETRY_BIT: return "VK_SHADER_STAGE_GEOMETRY_BIT";
	case VK_SHADER_STAGE_FRAGMENT_BIT: return "VK_SHADER_STAGE_FRAGMENT_BIT";
	case VK_SHADER_STAGE_COMPUTE_BIT: return "VK_SHADER_STAGE_COMPUTE_BIT";
	case VK_SHADER_STAGE_ALL_GRAPHICS: return "VK_SHADER_STAGE_ALL_GRAPHICS";
	case VK_SHADER_STAGE_ALL: return "VK_SHADER_STAGE_ALL";
	case VK_SHADER_STAGE_RAYGEN_BIT_KHR: return "VK_SHADER_STAGE_RAYGEN_BIT_KHR";
	case VK_SHADER_STAGE_ANY_HIT_BIT_KHR: return "VK_SHADER_STAGE_ANY_HIT_BIT_KHR";
	case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return "VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR";
	case VK_SHADER_STAGE_MISS_BIT_KHR: return "VK_SHADER_STAGE_MISS_BIT_KHR";
	case VK_SHADER_STAGE_INTERSECTION_BIT_KHR: return "VK_SHADER_STAGE_INTERSECTION_BIT_KHR";
	case VK_SHADER_STAGE_CALLABLE_BIT_KHR: return "VK_SHADER_STAGE_CALLABLE_BIT_KHR";
	case VK_SHADER_STAGE_TASK_BIT_NV: return "VK_SHADER_STAGE_TASK_BIT_NV";
	case VK_SHADER_STAGE_MESH_BIT_NV: return "VK_SHADER_STAGE_MESH_BIT_NV";
	//case VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI: return "VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI";
	default: return "?";
	}
}

std::string composed_shader_stage_name(VkShaderStageFlags shader_stages)
{
	std::string result;
	uint8_t i = 0;
	while (shader_stages > 0)
	{
		if (shader_stages & 1)
		{
			result += shader_stage_name(1 << i);
			result += ", ";
		}
		shader_stages >>= 1;
		i++;
	}
	if (result.size() > 0) {
		result.resize(result.size() - 2);
	}
	return "[" + result + "]";
}

void print_descriptor_slots(std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> const& bindings_by_set_idx)
{
	for (auto const& [set_idx, bindings] : bindings_by_set_idx)
	{
		for (auto const& binding : bindings)
		{
			auto shader_stages_name = composed_shader_stage_name(binding.stageFlags);

			printf("%d.%d) stages: %s, type: %s[%d]\n",
				   set_idx, binding.binding,
				   shader_stages_name.c_str(),
				   descriptor_type_name(binding.descriptorType),
				   binding.descriptorCount
			);
		}
	}
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

	/* Descriptor sets */
	spirv_reflect_check(spvReflectEnumerateDescriptorSets(&module, &num, nullptr));

	std::vector<SpvReflectDescriptorSet*> spv_refl_desc_sets(num);
	spirv_reflect_check(spvReflectEnumerateDescriptorSets(&module, &num, spv_refl_desc_sets.data()));

	std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> desc_slots;
	int32_t max_desc_set_idx = -1;

	for (int i = 0; i < spv_refl_desc_sets.size(); i++)
	{
		auto spv_refl_desc_set = spv_refl_desc_sets.at(i);

		std::vector<VkDescriptorSetLayoutBinding> bindings{};
		for (uint32_t j = 0; j < spv_refl_desc_set->binding_count; j++)
		{
			auto binding = spv_refl_desc_set->bindings[j];

			VkDescriptorType desc_type = parse_descriptor_type(binding->descriptor_type);

			bindings.push_back({
				.binding = binding->binding,
				.descriptorType = desc_type,
				.descriptorCount = binding->count,
				.stageFlags = shader_stage,
				.pImmutableSamplers = nullptr
			});
		}

		desc_slots.emplace(spv_refl_desc_sets.at(i)->set, bindings);
		max_desc_set_idx = std::max(max_desc_set_idx, (int32_t) spv_refl_desc_sets.at(i)->set);
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
		.m_descriptor_slots = std::move(desc_slots),
		.m_max_descriptor_set_idx = max_desc_set_idx,
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

vren::vk_utils::pipeline::~pipeline()
{
	for (VkDescriptorSetLayout desc_set_layout : m_descriptor_set_layouts) {
		vkDestroyDescriptorSetLayout(m_context->m_device, desc_set_layout, nullptr);
	}
}

VkDescriptorSetLayout vren::vk_utils::pipeline::get_descriptor_set_layout(uint32_t desc_set_idx) const
{
	return m_descriptor_set_layouts.at(desc_set_idx);
}

void vren::vk_utils::pipeline::bind(VkCommandBuffer cmd_buf) const
{
	vkCmdBindPipeline(cmd_buf, m_bind_point, m_pipeline.m_handle);
}

void vren::vk_utils::pipeline::bind_descriptor_set(VkCommandBuffer cmd_buf, uint32_t desc_set_idx, VkDescriptorSet desc_set) const
{
	vkCmdBindDescriptorSets(cmd_buf, m_bind_point, m_pipeline_layout.m_handle, desc_set_idx, 1, &desc_set, 0, nullptr);
}

void vren::vk_utils::pipeline::push_constants(VkCommandBuffer cmd_buf, VkShaderStageFlags shader_stage, void const* val, uint32_t val_size, uint32_t val_off) const
{
	vkCmdPushConstants(cmd_buf, m_pipeline_layout.m_handle, shader_stage, val_off, val_size, val);
}

void vren::vk_utils::pipeline::acquire_and_bind_descriptor_set(
	vren::context const& ctx,
	VkCommandBuffer cmd_buf,
	vren::resource_container& res_container,
	uint32_t desc_set_idx,
	std::function<void(VkDescriptorSet)> const& update_func
)
{
	auto desc_set = std::make_shared<vren::pooled_vk_descriptor_set>(
		ctx.m_toolbox->m_descriptor_pool.acquire(get_descriptor_set_layout(desc_set_idx))
	);
	update_func(desc_set->m_handle.m_descriptor_set);
	bind_descriptor_set(cmd_buf, desc_set_idx, desc_set->m_handle.m_descriptor_set);
	res_container.add_resources(desc_set);
}

void vren::vk_utils::merge_shader_descriptor_slots(std::span<shader> const& shaders, descriptor_slots_t& merged_bindings)
{
	for (auto& shader : shaders)
	{
		for (auto const& [desc_set_idx, bindings] : shader.m_descriptor_slots)
		{
			for (auto const& binding : bindings)
			{
				bool added = false;
				if (merged_bindings.contains(desc_set_idx))
				{
					for (auto& added_binding : merged_bindings.at(desc_set_idx))
					{
						if (added_binding.binding == binding.binding)
						{
							bool desc_type_match = added_binding.descriptorType == binding.descriptorType;
							bool desc_count_match = added_binding.descriptorCount == binding.descriptorCount;

							if (!desc_type_match || !desc_count_match)
							{
								auto desc_type_validation = std::string(" != ") + descriptor_type_name(added_binding.descriptorType);
								auto desc_count_validation = std::string(" != ") + std::to_string(added_binding.descriptorCount);

								fprintf(stderr, "Invalid binding %d.%d: descriptorType %s%s, descriptorCount %d%s\n",
										desc_set_idx, added_binding.binding,
										descriptor_type_name(binding.descriptorType),
										desc_type_match ? "" : desc_type_validation.c_str(),
										binding.descriptorCount,
										desc_count_match ? "" : desc_count_validation.c_str()
										);
								throw std::invalid_argument("Failed to merge shader bindings");
							}

							added_binding.stageFlags = VK_SHADER_STAGE_ALL;

							added = true;
							break;
						}
					}
				}

				if (!added)
				{
					merged_bindings.emplace(desc_set_idx, std::vector<VkDescriptorSetLayoutBinding>{});
					merged_bindings.at(desc_set_idx).push_back(binding);
				}
			}
		}
	}
}

void vren::vk_utils::create_descriptor_set_layouts(vren::context const& ctx, uint32_t desc_set_count, descriptor_slots_t const& desc_slots, std::vector<VkDescriptorSetLayout>& result)
{
	for (uint32_t desc_set_idx = 0; desc_set_idx <= desc_set_count; desc_set_idx++)
	{
		auto bindings = desc_slots.contains(desc_set_idx) ? std::make_optional(desc_slots.at(desc_set_idx)) : std::nullopt;

		VkDescriptorSetLayoutCreateInfo desc_set_layout_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = bindings.has_value() ? (uint32_t) bindings->size() : 0,
			.pBindings = bindings.has_value() ? bindings->data() : nullptr,
		};
		VkDescriptorSetLayout desc_set_layout;
		vkCreateDescriptorSetLayout(ctx.m_device, &desc_set_layout_info, nullptr, &desc_set_layout);

		result.push_back(desc_set_layout);
	}
}

vren::vk_utils::pipeline
vren::vk_utils::create_compute_pipeline(vren::context const& ctx, shader const& comp_shader)
{
	/* Descriptor set layouts */
	std::vector<VkDescriptorSetLayout> desc_set_layouts;
	create_descriptor_set_layouts(ctx, (int32_t) comp_shader.m_max_descriptor_set_idx + 1, comp_shader.m_descriptor_slots, desc_set_layouts);

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
		.m_context = &ctx,
		.m_descriptor_set_layouts = std::move(desc_set_layouts),
		.m_pipeline_layout = vren::vk_pipeline_layout(ctx, pipeline_layout),
		.m_pipeline = vren::vk_pipeline(ctx, pipeline),
		.m_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE,
	};
}

vren::vk_utils::pipeline
vren::vk_utils::create_graphics_pipeline(
	vren::context const& ctx,
	std::span<shader> const& shaders,
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
	std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> desc_slots;
	merge_shader_descriptor_slots(shaders, desc_slots);

	int32_t max_desc_set_idx = -1;
	for (auto const& shader : shaders) {
		max_desc_set_idx = std::max(shader.m_max_descriptor_set_idx, max_desc_set_idx);
	}
	std::vector<VkDescriptorSetLayout> desc_set_layouts;
	create_descriptor_set_layouts(ctx, (uint32_t) max_desc_set_idx + 1, desc_slots, desc_set_layouts);

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
		.m_context = &ctx,
		.m_descriptor_set_layouts = std::move(desc_set_layouts),
		.m_pipeline_layout = vren::vk_pipeline_layout(ctx, pipeline_layout),
		.m_pipeline = vren::vk_pipeline(ctx, pipeline),
		.m_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS
	};
}
