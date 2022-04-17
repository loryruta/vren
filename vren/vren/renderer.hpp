#pragma once

#include <memory>

#include <volk.h>
#include <glm/glm.hpp>

#include "config.hpp"
#include "simple_draw.hpp"
#include "light.hpp"
#include "pools/command_pool.hpp"
#include "base/resource_container.hpp"
#include "vertex_pipeline_draw_pass.hpp"

namespace vren
{
	// Forward decl
	class context;

	class renderer;

	// ------------------------------------------------------------------------------------------------

	struct render_target
	{
		VkFramebuffer m_framebuffer;
		VkRect2D m_render_area;
		VkViewport m_viewport;
		VkRect2D m_scissor;
	};

	struct draw_buffer
	{
		vren::vk_utils::buffer m_vertex_buffer;
		vren::vk_utils::buffer m_meshlet_vertex_buffer;
		vren::vk_utils::buffer m_meshlet_triangle_buffer;
		vren::vk_utils::buffer m_meshlet_buffer;
		size_t m_meshlet_count;
	};

	// ------------------------------------------------------------------------------------------------
	// Renderer
	// ------------------------------------------------------------------------------------------------

	class renderer
	{
	private:
		vren::context const* m_context;

	public:
		vren::vk_render_pass m_render_pass;

	private:
		vren::vertex_pipeline_draw_pass m_vertex_pipeline_draw_pass;

        vren::vk_render_pass create_render_pass();

	public:
		vren::light_array m_light_array;

		explicit renderer(vren::context const& context);

		void render(
			uint32_t frame_idx,
			VkCommandBuffer command_buffer,
			vren::resource_container& resource_container,
			vren::render_target const& render_target,
			vren::camera const& camera,
			std::vector<vren::mesh_buffer> const& mesh_buffers
		);
	};
}
