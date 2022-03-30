#include "shader.hpp"

#include <fstream>

#include "misc.hpp"

vren::vk_shader_module vren::vk_utils::create_shader_module(std::shared_ptr<vren::context> const& ctx, size_t code_size, uint32_t* code)
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
	return vren::vk_shader_module(
		ctx,
		shader_mod
	);
}

vren::vk_shader_module vren::vk_utils::load_shader_module(std::shared_ptr<vren::context> const& ctx, char const* path)
{
	// Read file
	std::ifstream f(path, std::ios::ate | std::ios::binary);
	if (!f.is_open()) {
		throw std::runtime_error("Failed to open file");
	}

	auto file_size = f.tellg();
	std::vector<char> buffer((size_t) glm::ceil(file_size / (float) sizeof(uint32_t)) * sizeof(uint32_t)); // Rounds to a multiple of 4 because codeSize requires it

	f.seekg(0);
	f.read(buffer.data(), file_size);

	f.close();

	return create_shader_module(
		ctx,
		buffer.size(),
		reinterpret_cast<uint32_t*>(buffer.data())
	);
}

// --------------------------------------------------------------------------------------------------------------------------------
// compute_pipeline
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::pipeline vren::vk_utils::create_compute_pipeline(
	std::shared_ptr<vren::context> const& ctx,
	std::vector<VkDescriptorSetLayout> desc_set_layouts,
	std::vector<VkPushConstantRange> push_constants,
	vren::vk_shader_module&& shader_module
)
{
	// We're not binding `descriptor set layouts` lifetime to the `pipeline` lifetime:
	// once the pipeline is created, do we need descriptor set layouts to be still alive?

	/* Pipeline layout */
	VkPipelineLayoutCreateInfo pipeline_layout_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.setLayoutCount = (uint32_t) desc_set_layouts.size(),
		.pSetLayouts = desc_set_layouts.data(),
		.pushConstantRangeCount = (uint32_t) push_constants.size(),
		.pPushConstantRanges = push_constants.data(),
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
			.module = shader_module.m_handle,
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

	// shader_module destroyed

	return vren::vk_utils::pipeline{
		.m_pipeline_layout = vren::vk_pipeline_layout(ctx, pipeline_layout),
		.m_pipeline        = vren::vk_pipeline(ctx, pipeline)
	};
}
