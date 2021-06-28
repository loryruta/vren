
#pragma once

#include <vector>
#include <array>

#include "config.hpp"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace vren
{
	// Forward decl

	class renderer;
	class render_list;
	class camera;

	//

	class simple_draw_pass
	{
	private:
		/* Init */
		void create_descriptor_set_layout();
		void create_graphics_pipeline();
		void init_descriptor_sets();

	public:
		vren::renderer& m_renderer;

		/* General */
		VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
		VkPipeline m_graphics_pipeline = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_descriptor_set_layout;

		/* Per frame data */
		std::array<VkDescriptorSet, VREN_MAX_FRAME_COUNT> m_descriptor_sets;

		simple_draw_pass(renderer& renderer);
		~simple_draw_pass();

		void fill_descriptor_pool_sizes(std::vector<VkDescriptorPoolSize>& descriptor_pool_sizes);
		void init();

		void fill_dst_dependency(VkSubpassDependency& dependency);
		void record_commands(
			uint32_t frame_idx,
			VkCommandBuffer command_buffer,
			vren::render_list const& render_list,
			vren::camera const& camera
		);
	};
}

