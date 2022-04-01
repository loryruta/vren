#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

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
	// self_described_shader
	// ------------------------------------------------------------------------------------------------

	struct self_described_shader
	{
		vren::vk_shader_module m_module;
		VkShaderStageFlagBits m_stage;
		std::string m_entry_point;

		std::unordered_map<uint32_t, vren::vk_descriptor_set_layout> m_descriptor_set_layouts;
		std::vector<VkPushConstantRange> m_push_constant_ranges;

		std::vector<VkDescriptorSetLayout> get_descriptor_set_layouts() const
		{
			std::vector<VkDescriptorSetLayout> desc_set_layouts;
			for (auto& [_, desc_set_layout] : m_descriptor_set_layouts) {
				desc_set_layouts.push_back(desc_set_layout.m_handle);
			}
			return desc_set_layouts;
		};

		VkDescriptorSetLayout get_descriptor_set_layout(uint32_t desc_set_idx) const
		{
			return m_descriptor_set_layouts.at(desc_set_idx).m_handle;
		}
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

	// ------------------------------------------------------------------------------------------------
	// self_described_compute_pipeline
	// ------------------------------------------------------------------------------------------------

	struct self_described_compute_pipeline
	{
		vren::vk_pipeline m_pipeline;
		vren::vk_pipeline_layout m_pipeline_layout;

		std::shared_ptr<self_described_shader> m_shader;

		void bind(VkCommandBuffer cmd_buf)
		{
			vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.m_handle);
		}

		void bind_descriptor_set(VkCommandBuffer cmd_buf, uint32_t desc_set_idx, VkDescriptorSet desc_set)
		{
			vkCmdBindDescriptorSets(
				cmd_buf,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				m_pipeline_layout.m_handle,
				desc_set_idx,
				1,
				&desc_set,
				0,
				nullptr
			);
		}

		void push_constants(VkCommandBuffer cmd_buf, VkShaderStageFlags shader_stage, void const* val, uint32_t val_size, uint32_t val_off = 0)
		{
			vkCmdPushConstants(
				cmd_buf,
				m_pipeline_layout.m_handle,
				shader_stage,
				val_off,
				val_size,
				val
			);
		}
	};

	self_described_compute_pipeline create_compute_pipeline(
		std::shared_ptr<vren::context> const& ctx,
		std::shared_ptr<self_described_shader> const& comp_shad
	);

	// ------------------------------------------------------------------------------------------------
	// self_described_graphics_pipeline
	// ------------------------------------------------------------------------------------------------

	struct self_described_graphics_pipeline
	{
		vren::vk_pipeline m_pipeline;
		vren::vk_pipeline_layout m_pipeline_layout;
		std::vector<std::shared_ptr<self_described_shader>> m_shaders;
	};

	self_described_graphics_pipeline create_graphics_pipeline(
		std::shared_ptr<vren::context> const& ctx,
		std::vector<std::shared_ptr<self_described_shader>> const& shaders,
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
