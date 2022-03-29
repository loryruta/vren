#include "shader.hpp"

#include <fstream>

#include "misc.hpp"

vren::vk_shader_module create_shader_module(std::shared_ptr<vren::context> const& ctx, size_t code_size, uint32_t* code)
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

vren::vk_utils::compute_pipeline::compute_pipeline(std::shared_ptr<vren::context> const& ctx) :
	m_context(ctx)
{}

vren::vk_utils::compute_pipeline::~compute_pipeline()
{
	vkDestroyPipeline(m_context->m_device, m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_context->m_device, m_pipeline_layout, nullptr);
}

vren::vk_utils::compute_pipeline vren::vk_utils::compute_pipeline::create(
	std::shared_ptr<vren::context> const& ctx,
	std::vector<std::shared_ptr<vren::vk_descriptor_set_layout>> desc_set_layouts,
	std::vector<VkPushConstantRange> push_constants,
	vren::vk_shader_module&& shader_module
)
{
	compute_pipeline res(ctx);

	/* Descriptor set layouts */
	res.m_descriptor_set_layouts = std::move(desc_set_layouts);

	std::vector<VkDescriptorSetLayout> raw_desc_set_layouts;
	for (auto& desc_set_layout : res.m_descriptor_set_layouts) {
		raw_desc_set_layouts.emplace_back(desc_set_layout->m_handle);
	}

	/* Pipeline layout */
	VkPipelineLayoutCreateInfo pipeline_layout_info{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.setLayoutCount = (uint32_t) raw_desc_set_layouts.size(),
		.pSetLayouts = raw_desc_set_layouts.data(),
		.pushConstantRangeCount = (uint32_t) push_constants.size(),
		.pPushConstantRanges = push_constants.data(),
	};
	vren::vk_utils::check(
		vkCreatePipelineLayout(ctx->m_device, &pipeline_layout_info, nullptr, &res.m_pipeline_layout)
	);

	/* Pipeline */
	VkComputePipelineCreateInfo pipeline_info{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = shader_module.m_handle,
			.pName = "main"
		},
		.layout = res.m_pipeline_layout,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = 0,
	};
	vren::vk_utils::check(
		vkCreateComputePipelines(ctx->m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &res.m_pipeline)
	);

	// shader_module destroyed

	return res;
}

