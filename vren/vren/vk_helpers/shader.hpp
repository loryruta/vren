#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <span>
#include <functional>

#include "vk_raii.hpp"
#include "base/resource_container.hpp"

namespace vren
{
	// Forward decl
	class context;
}

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

	using descriptor_slots_t = std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>;

	struct shader
	{
		vren::vk_shader_module m_module;
		VkShaderStageFlags m_stage;
		std::string m_entry_point;

		descriptor_slots_t m_descriptor_slots;
		int32_t m_max_descriptor_set_idx;

		std::vector<VkPushConstantRange> m_push_constant_ranges;
	};

	shader load_shader(
		vren::context const& ctx,
		size_t spv_code_size,
		uint32_t* spv_code
	);

	shader load_shader_from_file(
		vren::context const& ctx,
		char const* shad_path
	);

	// ------------------------------------------------------------------------------------------------
	// Pipeline
	// ------------------------------------------------------------------------------------------------

	struct pipeline
	{
		vren::context const* m_context;

		std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;

		vren::vk_pipeline_layout m_pipeline_layout;
		vren::vk_pipeline m_pipeline;

		VkPipelineBindPoint m_bind_point;

		~pipeline();

		VkDescriptorSetLayout get_descriptor_set_layout(uint32_t desc_set_idx) const;

		void bind(VkCommandBuffer cmd_buf) const;
		void bind_descriptor_set(VkCommandBuffer cmd_buf, uint32_t desc_set_idx, VkDescriptorSet desc_set) const;
		void push_constants(VkCommandBuffer cmd_buf, VkShaderStageFlags shader_stage, void const* val, uint32_t val_size, uint32_t val_off = 0) const;

		void acquire_and_bind_descriptor_set(
			vren::context const& ctx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			uint32_t desc_set_idx,
			std::function<void(VkDescriptorSet desc_set)> const& update_func
		);
	};

	void merge_shader_descriptor_slots(std::span<shader> const& shaders, descriptor_slots_t& result);
	void create_descriptor_set_layouts(vren::context const& ctx, uint32_t desc_set_count, descriptor_slots_t const& desc_slots, std::vector<VkDescriptorSetLayout>& desc_set_layouts);

	pipeline create_compute_pipeline(vren::context const& ctx, shader const& comp_shad);
	pipeline create_graphics_pipeline(
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
	);
}
