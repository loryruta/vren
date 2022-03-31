#pragma once

#include <memory>
#include <vector>

#include "context.hpp"
#include "vk_raii.hpp"


namespace vren::vk_utils
{
	vren::vk_shader_module create_shader_module(
		std::shared_ptr<vren::context> const& ctx,
		size_t code_size,
		uint32_t* code
	);

	vren::vk_shader_module load_shader_module(
		std::shared_ptr<vren::context> const& ctx,
		char const* shad_path
	);

	// ------------------------------------------------------------------------------------------------
	// Self-described
	// ------------------------------------------------------------------------------------------------

	struct self_described_shader
	{
		vren::vk_shader_module m_shader_module;
		VkPipelineShaderStageCreateInfo m_pipeline_shader_stage_info;

		std::vector<vren::vk_descriptor_set_layout> m_descriptor_set_layouts;
		std::vector<VkPushConstantRange> m_push_constant_ranges;
	};

	self_described_shader create_and_describe_shader(
		std::shared_ptr<vren::context> const& ctx,
		size_t spv_code_size,
		uint32_t* spv_code
	);

	self_described_shader load_and_describe_shader(
		std::shared_ptr<vren::context> const& ctx,
		char const* shad_path
	);

	struct self_described_compute_pipeline
	{
		vren::vk_pipeline m_pipeline;
		vren::vk_pipeline_layout m_pipeline_layout;

		self_described_shader m_shader;
	};

	self_described_compute_pipeline create_compute_pipeline(
		std::shared_ptr<vren::context> const& ctx,
		self_described_shader&& comp_shad
	);

	struct self_described_graphics_pipeline
	{
		vren::vk_pipeline m_pipeline;
		vren::vk_pipeline_layout m_pipeline_layout;
		std::vector<self_described_shader> m_shaders;
	};

	self_described_graphics_pipeline create_graphics_pipeline(
		std::shared_ptr<vren::context> const& ctx,
		std::vector<self_described_shader>&& shaders,
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
	);
}
