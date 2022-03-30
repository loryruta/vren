#pragma once

#include <memory>
#include <vector>

#include "context.hpp"
#include "vk_raii.hpp"


namespace vren::vk_utils
{
	vren::vk_shader_module create_shader_module(std::shared_ptr<vren::context> const& ctx, size_t code_size, uint32_t* code);
	vren::vk_shader_module load_shader_module(std::shared_ptr<vren::context> const& ctx, char const* path);

	// ------------------------------------------------------------------------------------------------
	// compute_pipeline
	// ------------------------------------------------------------------------------------------------

	struct pipeline
	{
		vren::vk_pipeline_layout m_pipeline_layout;
		vren::vk_pipeline m_pipeline;
	};

	static pipeline create_compute_pipeline(
		std::shared_ptr<vren::context> const& ctx,
		std::vector<VkDescriptorSetLayout> desc_set_layouts,
		std::vector<VkPushConstantRange> push_constants,
		vren::vk_shader_module&& shader_module
	);
}
