#pragma once

#include "base/resource_container.hpp"
#include "vk_helpers/vk_raii.hpp"
#include "vk_helpers/buffer.hpp"
#include "vk_helpers/shader.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Debug renderer
	// ------------------------------------------------------------------------------------------------

	class dbg_renderer
	{
	public:
		struct point
		{
			glm::vec3 m_position;
			glm::vec4 m_color;
		};

		struct line
		{
			glm::vec3 m_from, m_to;
			glm::vec4 m_color;
		};

		struct vertex
		{
			glm::vec3 m_position;
			glm::vec3 m_color;
		};

		struct instance_data
		{
			glm::mat4 m_transform;
			glm::vec3 m_color;
		};

	private:
		vren::context const* m_context;

		vren::vk_utils::buffer m_identity_instance_buffer;

		vren::vk_utils::resizable_buffer m_points_vertex_buffer; size_t m_points_vertex_count = 0;
		vren::vk_utils::resizable_buffer m_lines_vertex_buffer;  size_t m_lines_vertex_count = 0;

		vren::vk_render_pass m_render_pass;
		vren::vk_utils::pipeline m_pipeline;

		vren::vk_utils::buffer create_identity_instance_buffer();

		vren::vk_render_pass create_render_pass();
		vren::vk_utils::pipeline create_graphics_pipeline();

	public:
		dbg_renderer(vren::context const& ctx);

		void clear();

		void draw_point(dbg_renderer::point point);
		void draw_line(dbg_renderer::line line);

		void render(
			uint32_t frame_idx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container
		);
	};
}