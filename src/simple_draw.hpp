
#pragma once

#include <vector>
#include <array>

#include "config.hpp"
#include "material.hpp"
#include "frame.hpp"
#include "light.hpp"

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
		void create_graphics_pipeline();

	public:
		vren::renderer& m_renderer;

		// General
		VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
		VkPipeline m_graphics_pipeline = VK_NULL_HANDLE;

		// Per-frame data
		simple_draw_pass(renderer& renderer);
		~simple_draw_pass();

		void record_commands(
			vren::frame& frame,
			vren::render_list const& render_list,
			vren::lights_array const& lights_array,
			vren::camera const& camera
		);
	};
}

