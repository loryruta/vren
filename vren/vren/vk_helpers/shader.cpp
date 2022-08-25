#include "shader.hpp"

#include <fstream>

#include <spirv_cross.hpp>

#include "context.hpp"
#include "toolbox.hpp"
#include "misc.hpp"

#include "log.hpp"

#ifdef VREN_LOG_SHADER_DETAILED
#	define VREN_DEBUG0(m, ...) VREN_DEBUG(m, __VA_ARGS__)
#else
#	define VREN_DEBUG0
#endif

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

vren::vk_shader_module create_vk_shader_module(vren::context const& context, uint32_t const* code, size_t code_size)
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

// --------------------------------------------------------------------------------------------------------------------------------
// Shader
// --------------------------------------------------------------------------------------------------------------------------------

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

vren::shader_module vren::load_shader_module(vren::context const& context, uint32_t const* code, size_t code_length, char const* name)
{
	spirv_cross::Compiler compiler(code, code_length);

	VREN_DEBUG0("[shader] ----------------------------------------------------------------\n");
	VREN_DEBUG0("[shader] Shader module: \"{}\"\n", name);
	VREN_DEBUG0("[shader] ----------------------------------------------------------------\n");

	// Entry points
	std::vector<vren::shader_module_entry_point> entry_points{};

	auto parse_shader_stage = [](spv::ExecutionModel execution_model) -> VkShaderStageFlags
	{
		switch (execution_model)
		{
		case spv::ExecutionModelVertex:    return VK_SHADER_STAGE_VERTEX_BIT;
		case spv::ExecutionModelFragment:  return VK_SHADER_STAGE_FRAGMENT_BIT;
		case spv::ExecutionModelTaskNV:    return VK_SHADER_STAGE_TASK_BIT_NV;
		case spv::ExecutionModelMeshNV:    return VK_SHADER_STAGE_MESH_BIT_NV;
		case spv::ExecutionModelKernel:    return VK_SHADER_STAGE_COMPUTE_BIT;
		case spv::ExecutionModelGLCompute: return VK_SHADER_STAGE_COMPUTE_BIT;
		default:
			throw std::runtime_error("Execution model not recognized");
		}
	};

	for (spirv_cross::EntryPoint const& spirv_entry_point : compiler.get_entry_points_and_stages())
	{
		vren::shader_module_entry_point entry_point{
			.m_name = spirv_entry_point.name,
			.m_shader_stage = parse_shader_stage(spirv_entry_point.execution_model)
		};
		entry_points.push_back(entry_point);

		VREN_DEBUG0("[shader] Entry point: \"{}\" (shader stage: {:#010x})\n", entry_point.m_name, entry_point.m_shader_stage);
	}

	assert(entry_points.size() > 0);

	// Descriptor set layouts
	spirv_cross::ShaderResources shader_resources = compiler.get_shader_resources();

	std::unordered_map<uint32_t, vren::shader_module_descriptor_set_layout_info_t> descriptor_set_layouts;

	auto load_resources = [&](
		spirv_cross::SmallVector<spirv_cross::Resource> const& resources,
		std::function<VkDescriptorType(spirv_cross::Resource const&)> deduct_descriptor_type
	)
	{
		for (spirv_cross::Resource const& resource : resources)
		{
			assert(compiler.has_decoration(resource.id, spv::DecorationDescriptorSet));
			assert(compiler.has_decoration(resource.id, spv::DecorationBinding));

			uint32_t descriptor_set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

			spirv_cross::SPIRType type = compiler.get_type(resource.type_id);

			assert(type.array.size() == 0 || (type.array.size() == 1 && type.array_size_literal[0])); // Multi-dimensional arrays aren't supported

			VkDescriptorType descriptor_type = deduct_descriptor_type(resource);
			vren::shader_module_binding_info binding_info{
				.m_descriptor_type = descriptor_type,
				.m_descriptor_count = type.array.size() > 0 ? type.array[0] : 1,
			};
			descriptor_set_layouts.emplace(descriptor_set, std::unordered_map<uint32_t, vren::shader_module_binding_info>{});
			descriptor_set_layouts.at(descriptor_set).emplace(binding, binding_info);

			VREN_DEBUG0("[shader] Descriptor {}.{} - descriptor type: {:#010x} - count: {} - variable count: {}\n",
				descriptor_set,
				binding,
				binding_info.m_descriptor_type,
				binding_info.m_descriptor_count,
				binding_info.is_variable_descriptor_count() ? "true" : "false"
			);
		}
	};

	load_resources(shader_resources.sampled_images, [&](spirv_cross::Resource const& resource)
	{
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	});

	load_resources(shader_resources.separate_images, [&](spirv_cross::Resource const& resource)
	{
		spirv_cross::SPIRType type = compiler.get_type(resource.type_id);

		return type.image.dim == spv::Dim::DimBuffer ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	});

	load_resources(shader_resources.storage_images, [&](spirv_cross::Resource const& resource)
	{
		spirv_cross::SPIRType type = compiler.get_type(resource.type_id);

		return type.image.dim == spv::Dim::DimBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	});

	load_resources(shader_resources.separate_samplers, [&](spirv_cross::Resource const& resource)
	{
		return VK_DESCRIPTOR_TYPE_SAMPLER;
	});

	load_resources(shader_resources.uniform_buffers, [&](spirv_cross::Resource const& resource)
	{
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	});

	load_resources(shader_resources.storage_buffers, [&](spirv_cross::Resource const& resource)
	{
		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	});

	// Push constants
	size_t push_constant_block_size = 0;
	if (shader_resources.push_constant_buffers.size() > 0)
	{
		spirv_cross::Resource const& resource = shader_resources.push_constant_buffers[0];

		if (shader_resources.push_constant_buffers.size() > 1)
		{
			VREN_WARN("[shader] Only one push-constant block per stage is supported, taking: \"{}\"\n", resource.name);
		}

		std::string name = compiler.get_name(resource.id);
		push_constant_block_size = compiler.get_declared_struct_size(compiler.get_type(resource.type_id));

		VREN_DEBUG0("[shader] Push-constant block \"{}\" - size: {}\n",
			name,
			push_constant_block_size
		);
	}

	// Specialization constants
	std::vector<vren::shader_module_specialization_constant> specialization_constants;

	for (spirv_cross::SpecializationConstant const& spirv_specialization_constant : compiler.get_specialization_constants())
	{
		spirv_cross::SPIRConstant value = compiler.get_constant(spirv_specialization_constant.id);
		spirv_cross::SPIRType type = compiler.get_type(value.constant_type);

		std::string name = compiler.get_name(spirv_specialization_constant.id);
		size_t constant_size = vren::divide_and_ceil(type.width, 8); // type.width seems to hold the size of the type in bits?

		vren::shader_module_specialization_constant specialization_constant{
			.m_name = name,
			.m_constant_id = spirv_specialization_constant.constant_id,
			.m_size = constant_size,
		};
		specialization_constants.push_back(specialization_constant);

		VREN_DEBUG0("[shader] Specialization constant {} (ID: {}) - size: {}\n",
			specialization_constant.m_name,
			specialization_constant.m_constant_id,
			specialization_constant.m_size
		);
	}

	//
	return {
		.m_name = name,
		.m_handle = create_vk_shader_module(context, code, code_length * sizeof(uint32_t)),
		.m_entry_points = std::move(entry_points),
		.m_descriptor_set_layouts = std::move(descriptor_set_layouts),
		.m_push_constant_block_size = push_constant_block_size,
		.m_specialization_constants = std::move(specialization_constants)
	};
}

vren::shader_module vren::load_shader_module_from_file(vren::context const& context, char const* filename)
{
	std::vector<char> buffer{};
	load_bin_file(filename, buffer);
	return load_shader_module(context, reinterpret_cast<uint32_t const*>(buffer.data()), buffer.size() / sizeof(uint32_t), filename);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Pipeline
// --------------------------------------------------------------------------------------------------------------------------------

vren::pipeline::~pipeline()
{
	for (VkDescriptorSetLayout desc_set_layout : m_descriptor_set_layouts)
	{
		vkDestroyDescriptorSetLayout(m_context->m_device, desc_set_layout, nullptr);
	}
}

void vren::pipeline::bind(VkCommandBuffer cmd_buf) const
{
	vkCmdBindPipeline(cmd_buf, m_bind_point, m_pipeline.m_handle);
}

void vren::pipeline::bind_vertex_buffer(VkCommandBuffer command_buffer, uint32_t binding, VkBuffer vertex_buffer, VkDeviceSize offset) const
{
	vkCmdBindVertexBuffers(command_buffer, binding, 1, &vertex_buffer, &offset);
}

void vren::pipeline::bind_index_buffer(VkCommandBuffer command_buffer, VkBuffer index_buffer, VkIndexType index_type, VkDeviceSize offset) const
{
	vkCmdBindIndexBuffer(command_buffer, index_buffer, offset, index_type);
}

void vren::pipeline::bind_descriptor_set(VkCommandBuffer command_buffer, uint32_t descriptor_set_idx, VkDescriptorSet descriptor_set) const
{
	vkCmdBindDescriptorSets(command_buffer, m_bind_point, m_pipeline_layout.m_handle, descriptor_set_idx, 1, &descriptor_set, 0, nullptr);
}

void vren::pipeline::push_constants(VkCommandBuffer command_buffer, VkShaderStageFlags shader_stage, void const* data, uint32_t length, uint32_t offset) const
{
	vkCmdPushConstants(command_buffer, m_pipeline_layout.m_handle, shader_stage, offset, length, data);
}

void vren::pipeline::acquire_and_bind_descriptor_set(vren::context const& context, VkCommandBuffer command_buffer, vren::resource_container& resource_container, uint32_t descriptor_set_idx, std::function<void(VkDescriptorSet)> const& update_func)
{
	auto desc_set = std::make_shared<vren::pooled_vk_descriptor_set>(
		context.m_toolbox->m_descriptor_pool.acquire(m_descriptor_set_layouts.at(descriptor_set_idx))
	);
	update_func(desc_set->m_handle.m_descriptor_set);
	bind_descriptor_set(command_buffer, descriptor_set_idx, desc_set->m_handle.m_descriptor_set);
	resource_container.add_resources(desc_set);
}

void vren::pipeline::dispatch(VkCommandBuffer command_buffer, uint32_t workgroup_count_x, uint32_t workgroup_count_y, uint32_t workgroup_count_z) const
{
	vkCmdDispatch(command_buffer, workgroup_count_x, workgroup_count_y, workgroup_count_z);
}

// --------------------------------------------------------------------------------------------------------------------------------

/** 
 * Merge descriptor set layout info(s) from different shaders. This operation can fail whether different shaders
 * have different definitions for the same descriptor slot (= descriptor set index and binding).
 */
std::unordered_map<uint32_t, vren::shader_module_descriptor_set_layout_info_t> merge_descriptor_info(
	std::span<vren::specialized_shader const> shaders,
	int32_t& max_descriptor_set_idx
)
{
	std::unordered_map<uint32_t, vren::shader_module_descriptor_set_layout_info_t> merged_descriptor_info;
	max_descriptor_set_idx = -1;

	for (vren::specialized_shader const& shader : shaders)
	{
		for (auto const& [descriptor_set_idx, descriptor_set_layout_info] : shader.get_shader_module().m_descriptor_set_layouts)
		{
			if (merged_descriptor_info.contains(descriptor_set_idx)) // If the descriptor set is already taken
			{
				auto& merged_bindings = merged_descriptor_info.at(descriptor_set_idx);
				for (auto const& [binding, binding_info] : descriptor_set_layout_info)
				{
					if (merged_bindings.contains(binding))
					{
						// If the descriptor set binding is already taken just verify that the merged one matches with the current
						auto& merged_binding = merged_bindings.at(binding);

						assert(binding_info.m_descriptor_type == merged_binding.m_descriptor_type);
						assert(binding_info.m_descriptor_count == merged_binding.m_descriptor_count);
						assert(binding_info.is_variable_descriptor_count() == merged_binding.is_variable_descriptor_count());
					}
					else
					{
						merged_bindings.emplace(binding, binding_info);
					}
				}
			}
			else
			{
				merged_descriptor_info.emplace(descriptor_set_idx, descriptor_set_layout_info);
				max_descriptor_set_idx = std::max<int32_t>(max_descriptor_set_idx, descriptor_set_idx);
			}
		}
	}

	return merged_descriptor_info;
}

/**
 * Merge the descriptor definitions and create the descriptor set layouts for the given shaders. 
 */
std::vector<VkDescriptorSetLayout> create_descriptor_set_layouts(
	vren::context const& context,
	std::span<vren::specialized_shader const> shaders
)
{
	int32_t max_descriptor_set_idx;
	auto merged_descriptor_info = merge_descriptor_info(shaders, max_descriptor_set_idx);

	// Descriptor set layouts
	std::vector<VkDescriptorSetLayout> descriptor_set_layouts{};

	descriptor_set_layouts.resize(max_descriptor_set_idx + 1);

	for (int32_t descriptor_set_idx = 0; descriptor_set_idx <= max_descriptor_set_idx; descriptor_set_idx++)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings{};
		std::vector<VkDescriptorBindingFlags> binding_flags{};

		if (merged_descriptor_info.contains(descriptor_set_idx))
		{
			vren::shader_module_descriptor_set_layout_info_t const& descriptor_info = merged_descriptor_info.at(descriptor_set_idx);
			for (auto const& [binding, binding_info] : descriptor_info)
			{
				VkDescriptorSetLayoutBinding descriptor_set_layout_binding{
					.binding = binding,
					.descriptorType = binding_info.m_descriptor_type,
					.descriptorCount = binding_info.is_variable_descriptor_count() ? vren::k_max_variable_count_descriptor_count : binding_info.m_descriptor_count,
					.stageFlags = VK_SHADER_STAGE_ALL,
					.pImmutableSamplers = nullptr
				};
				bindings.push_back(descriptor_set_layout_binding);

				if (binding_info.is_variable_descriptor_count())
				{
					binding_flags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT);
				}
				else
				{
					binding_flags.push_back(NULL);
				}
			}
		}

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

	return descriptor_set_layouts;
}

vren::pipeline vren::create_compute_pipeline(vren::context const& context, vren::specialized_shader const& shader)
{
	vren::shader_module const& shader_module = shader.get_shader_module();

	// Descriptor set layouts
	std::vector<VkDescriptorSetLayout> descriptor_set_layouts = create_descriptor_set_layouts(context, std::span(&shader, 1));

	// Push constant range
	VkPushConstantRange push_constant_range{
		.stageFlags = shader.get_shader_stage(),
		.offset = 0,
		.size = (uint32_t) shader_module.m_push_constant_block_size
	};

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.setLayoutCount = (uint32_t) descriptor_set_layouts.size(),
		.pSetLayouts = descriptor_set_layouts.data(),
		.pushConstantRangeCount = shader_module.has_push_constant_block() ? 1u : 0u,
		.pPushConstantRanges = shader_module.has_push_constant_block() ? &push_constant_range : nullptr,
	};
	VkPipelineLayout pipeline_layout;
	VREN_CHECK(vkCreatePipelineLayout(context.m_device, &pipeline_layout_info, nullptr, &pipeline_layout), &context);

	// Pipeline shader stage
	std::vector<VkSpecializationMapEntry> specialization_map_entries = shader_module.create_specialization_map_entries();
	VkSpecializationInfo specialization_info{
		.mapEntryCount = (uint32_t) specialization_map_entries.size(),
		.pMapEntries = specialization_map_entries.data(),
		.dataSize = shader.get_specialization_data_length(),
		.pData = shader.get_specialization_data()
	};

	VkPipelineShaderStageCreateInfo pipeline_shader_stage_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.stage = static_cast<VkShaderStageFlagBits>(shader.get_shader_stage()),
		.module = shader_module.m_handle.m_handle,
		.pName = shader.get_entry_point(),
		.pSpecializationInfo = shader.has_specialization_data() ? &specialization_info : nullptr
	};

	// Compute pipeline
	VkComputePipelineCreateInfo pipeline_info{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.stage = pipeline_shader_stage_info,
		.layout = pipeline_layout,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = 0,
	};
	VkPipeline pipeline;
	VREN_CHECK(vkCreateComputePipelines(context.m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline), &context);

	//
	return vren::pipeline{
		.m_context = &context,
		.m_descriptor_set_layouts = std::move(descriptor_set_layouts),
		.m_pipeline_layout = vren::vk_pipeline_layout(context, pipeline_layout),
		.m_pipeline = vren::vk_pipeline(context, pipeline),
		.m_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE,
	};
}

vren::pipeline vren::create_graphics_pipeline(
	vren::context const& context,
	std::span<vren::specialized_shader const> shaders,
	VkPipelineVertexInputStateCreateInfo* vtx_input_state_info,
	VkPipelineInputAssemblyStateCreateInfo* input_assembly_state_info,
	VkPipelineTessellationStateCreateInfo* tessellation_state_info,
	VkPipelineViewportStateCreateInfo* viewport_state_info,
	VkPipelineRasterizationStateCreateInfo* rasterization_state_info,
	VkPipelineMultisampleStateCreateInfo* multisample_state_info,
	VkPipelineDepthStencilStateCreateInfo* depth_stencil_state_info,
	VkPipelineColorBlendStateCreateInfo* color_blend_state_info,
	VkPipelineDynamicStateCreateInfo* dynamic_state_info,
	VkPipelineRenderingCreateInfoKHR* pipeline_rendering_info
)
{
	// Descriptor set layouts
	std::vector<VkDescriptorSetLayout> descriptor_set_layouts = create_descriptor_set_layouts(context, shaders);

	// Pipeline shader stages
	std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stages{};
	for (vren::specialized_shader const& shader : shaders)
	{
		vren::shader_module const& shader_module = shader.get_shader_module();

		std::vector<VkSpecializationMapEntry> specialization_map_entries = shader_module.create_specialization_map_entries();
		VkSpecializationInfo specialization_info{
			.mapEntryCount = (uint32_t) specialization_map_entries.size(),
			.pMapEntries = specialization_map_entries.data(),
			.dataSize = shader.get_specialization_data_length(),
			.pData = shader.get_specialization_data()
		};

		pipeline_shader_stages.push_back({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = NULL,
			.stage = static_cast<VkShaderStageFlagBits>(shader.get_shader_stage()),
			.module = shader_module.m_handle.m_handle,
			.pName = shader.get_entry_point(),
			.pSpecializationInfo = shader.has_specialization_data() ? &specialization_info : nullptr
		});
	}

	// Push constant ranges
	std::vector<VkPushConstantRange> push_constant_ranges{};
	for (vren::specialized_shader const& shader : shaders)
	{
		vren::shader_module const& shader_mod = shader.get_shader_module();

		if (shader_mod.has_push_constant_block())
		{
			push_constant_ranges.push_back(VkPushConstantRange{
				.stageFlags = shader.get_shader_stage(),
				.offset = 0,
				.size = (uint32_t) shader_mod.m_push_constant_block_size
			});
		}
	}

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.setLayoutCount = (uint32_t) descriptor_set_layouts.size(),
		.pSetLayouts = descriptor_set_layouts.data(),
		.pushConstantRangeCount = (uint32_t)push_constant_ranges.size(),
		.pPushConstantRanges = push_constant_ranges.data(),
	};
	VkPipelineLayout pipeline_layout;
	VREN_CHECK(vkCreatePipelineLayout(context.m_device, &pipeline_layout_info, nullptr, &pipeline_layout), &context);

	// Graphics pipeline
	VkGraphicsPipelineCreateInfo pipeline_info{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = pipeline_rendering_info,
		.stageCount = (uint32_t) pipeline_shader_stages.size(),
		.pStages = pipeline_shader_stages.data(),
		.pVertexInputState = vtx_input_state_info,
		.pInputAssemblyState = input_assembly_state_info,
		.pTessellationState = tessellation_state_info,
		.pViewportState = viewport_state_info,
		.pRasterizationState = rasterization_state_info,
		.pMultisampleState = multisample_state_info,
		.pDepthStencilState = depth_stencil_state_info,
		.pColorBlendState = color_blend_state_info,
		.pDynamicState = dynamic_state_info,
		.layout = pipeline_layout
	};
	VkPipeline pipeline;
	VREN_CHECK(vkCreateGraphicsPipelines(context.m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline), &context);

	//
	return {
		.m_context = &context,
		.m_descriptor_set_layouts = std::move(descriptor_set_layouts),
		.m_pipeline_layout = vren::vk_pipeline_layout(context, pipeline_layout),
		.m_pipeline = vren::vk_pipeline(context, pipeline),
		.m_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS
	};
}
