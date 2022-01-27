#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "renderer.hpp"
#include "command_pool.hpp"

namespace vren
{
	class simple_draw_pass
	{
	private:
		std::shared_ptr<vren::renderer> m_renderer;

		void _create_graphics_pipeline();

	public:
		VkShaderModule m_vertex_shader   = VK_NULL_HANDLE;
		VkShaderModule m_fragment_shader = VK_NULL_HANDLE;

		VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
		VkPipeline m_graphics_pipeline     = VK_NULL_HANDLE;

		simple_draw_pass(std::shared_ptr<vren::renderer> const& renderer);
		~simple_draw_pass();

		void record_commands(
			vren::frame& frame,
			vren::vk_command_buffer const& cmd_buffer,
			vren::render_list const& render_list,
			vren::lights_array const& lights_array,
			vren::camera const& camera
		);
	};
}

