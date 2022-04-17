#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <span>
#include <functional>

#include <spirv_reflect.h>

#include "vk_raii.hpp"
#include "base/resource_container.hpp"

namespace vren
{
	// Forward decl
	class context;
}

namespace vren::vk_utils
{
	vren::vk_shader_module create_shader_module(vren::context const& context, size_t code_size, uint32_t* code);
	vren::vk_shader_module load_shader_module(vren::context const& context, char const* shader_filename);

	// ------------------------------------------------------------------------------------------------
	// Descriptor set layout info
	// ------------------------------------------------------------------------------------------------

	static constexpr uint32_t k_max_variable_count_descriptor_count = 65536;

	struct descriptor_set_layout_binding_info
	{
		VkDescriptorType m_descriptor_type;
		uint32_t m_descriptor_count;
		bool m_variable_descriptor_count;
	};

	struct descriptor_set_layout_info
	{
		std::unordered_map<uint32_t, descriptor_set_layout_binding_info> m_bindings;
	};

	void print_descriptor_set_layouts(std::unordered_map<uint32_t, descriptor_set_layout_info> const& descriptor_set_layouts);

	// ------------------------------------------------------------------------------------------------
	// Shader
	// ------------------------------------------------------------------------------------------------

	struct shader
	{
		vren::vk_shader_module m_module;
		VkShaderStageFlags m_stage;
		std::string m_entry_point;
		std::unordered_map<uint32_t, descriptor_set_layout_info> m_descriptor_set_layouts;
		std::vector<VkPushConstantRange> m_push_constant_ranges;
	};

	shader load_shader(vren::context const& context, size_t code_size, uint32_t* code);
	shader load_shader_from_file(vren::context const& context, char const* shader_filename);

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

		void bind(VkCommandBuffer cmd_buf) const;
		void bind_vertex_buffer(VkCommandBuffer command_buffer, uint32_t binding, VkBuffer vertex_buffer, VkDeviceSize offset = 0) const;
		void bind_index_buffer(VkCommandBuffer command_buffer, VkBuffer index_buffer, VkIndexType index_type, VkDeviceSize offset = 0) const;
		void bind_descriptor_set(VkCommandBuffer command_buffer, uint32_t descriptor_set_idx, VkDescriptorSet descriptor_set) const;
		void push_constants(VkCommandBuffer command_buffer, VkShaderStageFlags shader_stage, void const* data, uint32_t length, uint32_t offset = 0) const;

		void acquire_and_bind_descriptor_set(vren::context const& context, VkCommandBuffer command_buffer, vren::resource_container& resource_container, uint32_t descriptor_set_idx, std::function<void(VkDescriptorSet desc_set)> const& update_func);
	};

	void create_descriptor_set_layouts(vren::context const& context, std::span<shader const> const& shaders, std::vector<VkDescriptorSetLayout>& descriptor_set_layouts);

	pipeline create_compute_pipeline(vren::context const& context, shader const& shader);

	pipeline create_graphics_pipeline(
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
	);
}
