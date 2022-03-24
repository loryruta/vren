#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "resource_container.hpp"
#include "render_list.hpp"
#include "light_array.hpp"

namespace vren
{
	// Forward decl
	class renderer;
	struct camera;

	// ------------------------------------------------------------------------------------------------
	// simple_draw_pass
	// ------------------------------------------------------------------------------------------------

	class simple_draw_pass
	{
	private:
		void _create_graphics_pipeline();

	public:
		std::shared_ptr<vren::renderer> m_renderer;

		VkShaderModule m_vertex_shader = VK_NULL_HANDLE;
		VkShaderModule m_fragment_shader = VK_NULL_HANDLE;

		VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
		VkPipeline m_graphics_pipeline = VK_NULL_HANDLE;

		simple_draw_pass(
			std::shared_ptr<vren::renderer> const& renderer
		);
		~simple_draw_pass();

		void record_commands(
            int frame_idx,
            VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			vren::render_list const& render_list,
			vren::light_array const& lights_array,
			vren::camera const& camera
		);
	};
}

