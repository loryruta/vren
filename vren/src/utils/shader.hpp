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

	class compute_pipeline
	{
	private:
		std::shared_ptr<vren::context> m_context;

		explicit compute_pipeline(std::shared_ptr<vren::context> const& ctx);

	public:
		std::vector<std::shared_ptr<vren::vk_descriptor_set_layout>> m_descriptor_set_layouts;
		VkPipelineLayout m_pipeline_layout;
		VkPipeline m_pipeline;

		compute_pipeline(compute_pipeline const& other) = delete;
		compute_pipeline(compute_pipeline&& other); // TODO move ctr

		~compute_pipeline();

		static compute_pipeline create(
			std::shared_ptr<vren::context> const& ctx,
			std::vector<std::shared_ptr<vren::vk_descriptor_set_layout>> desc_set_layouts,
			std::vector<VkPushConstantRange> push_constants,
			vren::vk_shader_module&& shader_module
		);
	};
}
