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

vren::vk_shader_module vren::vk_utils::create_shader_module(vren::context const& context, size_t code_size, uint32_t* code)
{
	VkShaderModuleCreateInfo shader_info{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.codeSize = code_size,
		.pCode = code
	};
	VkShaderModule shader_module;
	VREN_CHECK(vkCreateShaderModule(context.m_device, &shader_info, nullptr, &shader_module), &context);
	return vren::vk_shader_module(context, shader_module);
}

vren::vk_shader_module vren::vk_utils::load_shader_module(vren::context const& context, char const* shader_filename)
{
	std::vector<char> buf;
	load_bin_file(shader_filename, buf);
	return create_shader_module(context, buf.size(), reinterpret_cast<uint32_t*>(buf.data()));
}

// --------------------------------------------------------------------------------------------------------------------------------
// Shader
// --------------------------------------------------------------------------------------------------------------------------------

void spirv_reflect_check(SpvReflectResult res)
{
	if (res != SPV_REFLECT_RESULT_SUCCESS)
	{
		VREN_ERROR("SPIRV-Reflect error: {}\n", res);
		assert(false);
	}
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

void vren::vk_utils::print_descriptor_set_layouts(std::unordered_map<uint32_t, descriptor_set_layout_info> const& descriptor_set_layouts)
{
	if (descriptor_set_layouts.size() > 0)
	{
		for (auto const& [descriptor_set_idx, descriptor_set_info] : descriptor_set_layouts)
		{
			for (auto const& [binding, binding_info] : descriptor_set_info.m_bindings)
			{
				printf("Descriptor #%d.%d, type: %s, count: %d, variable length: %s\n",
					   descriptor_set_idx, binding,
					   descriptor_type_name(binding_info.m_descriptor_type),
					   binding_info.m_descriptor_count,
					   binding_info.m_variable_descriptor_count ? "true" : "false"
				);
			}
		}
	}
	else
	{
		printf("No descriptors\n");
	}
}

vren::vk_utils::shader vren::vk_utils::load_shader(vren::context const& context, size_t code_size, uint32_t* code)
{
	spv_reflect::ShaderModule spv_shader_module(code_size, code);

	VkShaderStageFlags shader_stage = parse_shader_stage(spv_shader_module.GetShaderStage());

	uint32_t num;

	/* Descriptor sets */
	spirv_reflect_check(spv_shader_module.EnumerateDescriptorSets(&num, nullptr));

	std::vector<SpvReflectDescriptorSet*> spv_descriptor_sets(num);
	spirv_reflect_check(spv_shader_module.EnumerateDescriptorSets(&num, spv_descriptor_sets.data()));

	std::unordered_map<uint32_t, descriptor_set_layout_info> descriptor_set_layouts;
	for (uint32_t i = 0; i < spv_descriptor_sets.size(); i++)
	{
		SpvReflectDescriptorSet* spv_descriptor_set = spv_descriptor_sets.at(i);

		descriptor_set_layout_info descriptor_set_layout{};

		for (uint32_t j = 0; j < spv_descriptor_set->binding_count; j++)
		{
			SpvReflectDescriptorBinding* spv_binding = spv_descriptor_set->bindings[j];

			descriptor_set_layout_binding_info binding{
				.m_descriptor_type = parse_descriptor_type(spv_binding->descriptor_type),
				.m_descriptor_count = 1,
				.m_variable_descriptor_count = spv_binding->type_description->op == SpvOpTypeRuntimeArray
			};
			descriptor_set_layout.m_bindings.emplace(spv_binding->binding, binding);
		}
		descriptor_set_layouts.emplace(spv_descriptor_set->set, descriptor_set_layout);
	}

	/* Push constants */
	spirv_reflect_check(spv_shader_module.EnumeratePushConstantBlocks(&num, nullptr));

	std::vector<SpvReflectBlockVariable*> spv_refl_block_vars(num);
	spirv_reflect_check(spv_shader_module.EnumeratePushConstantBlocks(&num, spv_refl_block_vars.data()));

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

	printf("Shader %s:\n", spv_shader_module.GetSourceFile());
	print_descriptor_set_layouts(descriptor_set_layouts);

	return {
		.m_module = create_shader_module(context, code_size, code),
		.m_stage = shader_stage,
		.m_entry_point = "main",
		.m_descriptor_set_layouts = std::move(descriptor_set_layouts),
		.m_push_constant_ranges = std::move(push_constant_ranges)
	};
}

vren::vk_utils::shader vren::vk_utils::load_shader_from_file(vren::context const& context, char const* shader_filename)
{
	std::vector<char> buffer;
	load_bin_file(shader_filename, buffer);
	return load_shader(context, buffer.size(), reinterpret_cast<uint32_t*>(buffer.data()));
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

void vren::vk_utils::pipeline::bind(VkCommandBuffer cmd_buf) const
{
	vkCmdBindPipeline(cmd_buf, m_bind_point, m_pipeline.m_handle);
}

void vren::vk_utils::pipeline::bind_vertex_buffer(VkCommandBuffer command_buffer, uint32_t binding, VkBuffer vertex_buffer, VkDeviceSize offset) const
{
	vkCmdBindVertexBuffers(command_buffer, binding, 1, &vertex_buffer, &offset);
}

void vren::vk_utils::pipeline::bind_index_buffer(VkCommandBuffer command_buffer, VkBuffer index_buffer, VkIndexType index_type, VkDeviceSize offset) const
{
	vkCmdBindIndexBuffer(command_buffer, index_buffer, offset, index_type);
}

void vren::vk_utils::pipeline::bind_descriptor_set(VkCommandBuffer command_buffer, uint32_t descriptor_set_idx, VkDescriptorSet descriptor_set) const
{
	vkCmdBindDescriptorSets(command_buffer, m_bind_point, m_pipeline_layout.m_handle, descriptor_set_idx, 1, &descriptor_set, 0, nullptr);
}

void vren::vk_utils::pipeline::push_constants(VkCommandBuffer command_buffer, VkShaderStageFlags shader_stage, void const* data, uint32_t length, uint32_t offset) const
{
	vkCmdPushConstants(command_buffer, m_pipeline_layout.m_handle, shader_stage, offset, length, data);
}

void vren::vk_utils::pipeline::acquire_and_bind_descriptor_set(vren::context const& context, VkCommandBuffer command_buffer, vren::resource_container& resource_container, uint32_t descriptor_set_idx, std::function<void(VkDescriptorSet)> const& update_func)
{
	auto desc_set = std::make_shared<vren::pooled_vk_descriptor_set>(
		context.m_toolbox->m_descriptor_pool.acquire(m_descriptor_set_layouts.at(descriptor_set_idx))
	);
	update_func(desc_set->m_handle.m_descriptor_set);
	bind_descriptor_set(command_buffer, descriptor_set_idx, desc_set->m_handle.m_descriptor_set);
	resource_container.add_resources(desc_set);
}

void vren::vk_utils::create_descriptor_set_layouts(vren::context const& context, std::span<shader const> const& shaders, std::vector<VkDescriptorSetLayout>& descriptor_set_layouts)
{
	/* Merging */
	std::unordered_map<uint32_t, descriptor_set_layout_info> merged_descriptor_set_layout_info;
	uint32_t max_descriptor_set_idx = 0;

	for (auto const& shader : shaders)
	{
		for (auto const& [descriptor_set_idx, descriptor_set_layout_info] : shader.m_descriptor_set_layouts)
		{
			if (merged_descriptor_set_layout_info.contains(descriptor_set_idx)) // If the descriptor set is already taken
			{
				auto& merged_bindings = merged_descriptor_set_layout_info.at(descriptor_set_idx).m_bindings;
				for (auto const& [binding, binding_info] : descriptor_set_layout_info.m_bindings)
				{
					if (merged_bindings.contains(binding))
					{
						// If the descriptor set binding is already taken just verify that the merged one matches with the current
						auto& merged_binding = merged_bindings.at(binding);

						//assert(binding_info.m_binding == merged_binding.m_binding);
						assert(binding_info.m_descriptor_type == merged_binding.m_descriptor_type);
						assert(binding_info.m_descriptor_count == merged_binding.m_descriptor_count);
						assert(binding_info.m_variable_descriptor_count == merged_binding.m_variable_descriptor_count);
					}
					else
					{
						merged_bindings.emplace(binding, binding_info);
					}
				}
			}
			else
			{
				merged_descriptor_set_layout_info.emplace(descriptor_set_idx, descriptor_set_layout_info);
				max_descriptor_set_idx = std::max(max_descriptor_set_idx, descriptor_set_idx);
			}
		}
	}

	printf("Merged descriptor set layouts:\n");
	print_descriptor_set_layouts(merged_descriptor_set_layout_info);

	/* Creation */
	descriptor_set_layouts.resize(max_descriptor_set_idx + 1);

	for (uint32_t descriptor_set_idx = 0; descriptor_set_idx <= max_descriptor_set_idx; descriptor_set_idx++)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		std::vector<VkDescriptorBindingFlags> binding_flags;

		// Bindings
		if (merged_descriptor_set_layout_info.contains(descriptor_set_idx))
		{
			auto const& descriptor_set_layout_info = merged_descriptor_set_layout_info.at(descriptor_set_idx);
			for (auto const& [binding, binding_info] : descriptor_set_layout_info.m_bindings)
			{
				VkDescriptorSetLayoutBinding descriptor_set_layout_binding{
					.binding = binding,
					.descriptorType = binding_info.m_descriptor_type,
					.descriptorCount = binding_info.m_variable_descriptor_count ? k_max_variable_count_descriptor_count : binding_info.m_descriptor_count,
					.stageFlags = VK_SHADER_STAGE_ALL,
					.pImmutableSamplers = nullptr
				};
				bindings.push_back(descriptor_set_layout_binding);

				if (binding_info.m_variable_descriptor_count) {
					binding_flags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT);
				} else {
					binding_flags.push_back(NULL);
				}
			}
		}

		// Create descriptor set layout
		VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_create_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.pNext = nullptr,
			.bindingCount = (uint32_t) binding_flags.size(),
			.pBindingFlags = binding_flags.data()
		};
		VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &binding_flags_create_info,
			.flags = NULL,
			.bindingCount = (uint32_t) bindings.size(),
			.pBindings = bindings.data()
		};
		VkDescriptorSetLayout descriptor_set_layout;
		VREN_CHECK(vkCreateDescriptorSetLayout(context.m_device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout), &context);
		descriptor_set_layouts[descriptor_set_idx] = descriptor_set_layout;
	}
}


vren::vk_utils::pipeline vren::vk_utils::create_compute_pipeline(vren::context const& context, shader const& shader)
{
	/* Descriptor set layouts */
	std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
	create_descriptor_set_layouts(context, std::span(&shader, 1), descriptor_set_layouts);

	/* Push constant ranges */
	auto& push_const_ranges = shader.m_push_constant_ranges;

	/* Pipeline layout */
	VkPipelineLayoutCreateInfo pipeline_layout_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.setLayoutCount = (uint32_t) descriptor_set_layouts.size(),
		.pSetLayouts = descriptor_set_layouts.data(),
		.pushConstantRangeCount = (uint32_t) push_const_ranges.size(),
		.pPushConstantRanges = push_const_ranges.data(),
	};
	VkPipelineLayout pipeline_layout;
	VREN_CHECK(vkCreatePipelineLayout(context.m_device, &pipeline_layout_info, nullptr, &pipeline_layout), &context);

	/* Pipeline */
	VkComputePipelineCreateInfo pipeline_info{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = NULL,
			.stage = static_cast<VkShaderStageFlagBits>(shader.m_stage),
			.module = shader.m_module.m_handle,
			.pName = shader.m_entry_point.c_str(),
			.pSpecializationInfo = nullptr
		},
		.layout = pipeline_layout,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = 0,
	};
	VkPipeline pipeline;
	VREN_CHECK(vkCreateComputePipelines(context.m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline), &context);

	return vren::vk_utils::pipeline{
		.m_context = &context,
		.m_descriptor_set_layouts = std::move(descriptor_set_layouts),
		.m_pipeline_layout = vren::vk_pipeline_layout(context, pipeline_layout),
		.m_pipeline = vren::vk_pipeline(context, pipeline),
		.m_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE,
	};
}

vren::vk_utils::pipeline
vren::vk_utils::create_graphics_pipeline(
	vren::context const& context,
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
	std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
	create_descriptor_set_layouts(context, shaders, descriptor_set_layouts);

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
		.setLayoutCount = (uint32_t) descriptor_set_layouts.size(),
		.pSetLayouts = descriptor_set_layouts.data(),
		.pushConstantRangeCount = (uint32_t) push_const_ranges.size(),
		.pPushConstantRanges = push_const_ranges.data(),
	};
	VkPipelineLayout pipeline_layout;
	VREN_CHECK(vkCreatePipelineLayout(context.m_device, &pipeline_layout_info, nullptr, &pipeline_layout), &context);

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
	VREN_CHECK(vkCreateGraphicsPipelines(context.m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline), &context);

	/* */

	return {
		.m_context = &context,
		.m_descriptor_set_layouts = std::move(descriptor_set_layouts),
		.m_pipeline_layout = vren::vk_pipeline_layout(context, pipeline_layout),
		.m_pipeline = vren::vk_pipeline(context, pipeline),
		.m_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS
	};
}
