#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "context.hpp"
#include "vk_raii.hpp"

namespace vren::vk_utils
{
	vren::vk_shader_module create_shader_module(
		vren::context const& ctx,
		size_t code_size,
		uint32_t* code
	);

	vren::vk_shader_module load_shader_module(
		vren::context const& ctx,
		char const* shad_path
	);

	// ------------------------------------------------------------------------------------------------
	// Shader
	// ------------------------------------------------------------------------------------------------

	struct shader
	{
		vren::vk_shader_module m_module;
		VkShaderStageFlags m_stage;
		std::string m_entry_point;
		std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> m_bindings_by_set_idx;
		std::vector<VkPushConstantRange> m_push_constant_ranges;
	};

	shader create_and_describe_shader(
		vren::context const& ctx,
		size_t spv_code_size,
		uint32_t* spv_code
	);

	shader load_and_describe_shader(
		vren::context const& ctx,
		char const* shad_path
	);

	// ------------------------------------------------------------------------------------------------
	// Pipeline
	// ------------------------------------------------------------------------------------------------

	struct pipeline
	{
		vren::vk_pipeline m_pipeline;
		vren::vk_pipeline_layout m_pipeline_layout;

		VkPipelineBindPoint m_bind_point;

		using descriptor_set_layout_table_t = std::unordered_map<VkShaderStageFlags, std::unordered_map<uint32_t, vren::vk_descriptor_set_layout>>;
		descriptor_set_layout_table_t m_descriptor_set_layouts_table;

		VkDescriptorSetLayout get_descriptor_set_layout(VkShaderStageFlags shader_stage, uint32_t desc_set_idx);

		void bind(VkCommandBuffer cmd_buf);
		void bind_descriptor_set(VkCommandBuffer cmd_buf, uint32_t desc_set_idx, VkDescriptorSet desc_set);
		void push_constants(VkCommandBuffer cmd_buf, VkShaderStageFlags shader_stage, void const* val, uint32_t val_size, uint32_t val_off = 0);

		void acquire_and_bind_descriptor_set(
			vren::context const& ctx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			VkShaderStageFlags shader_stage,
			uint32_t desc_set_idx,
			std::function<void(VkDescriptorSet desc_set)> const& update_func
		);
	};

	pipeline create_compute_pipeline(vren::context const& ctx, shader const& comp_shad);

	pipeline create_graphics_pipeline(
		vren::context const& ctx,
		std::vector<shader> const& shaders,
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
