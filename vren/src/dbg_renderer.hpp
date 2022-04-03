#pragma once

#include "utils/vk_raii.hpp"
#include "utils/vk_toolbox.hpp"
#include "utils/shader.hpp"

namespace vren
{
	class dbg_renderer
	{
	public:
		struct point { glm::vec3 m_position; };
		struct line  { glm::vec3 m_from, m_to; };

	private:
		vren::vk_utils::toolbox const* m_toolbox;

		vren::vk_utils::resizable_buffer m_points_buffer;
		uint32_t m_point_idx = 0;

		vren::vk_utils::resizable_buffer m_lines_buffer;

		/*
		vren::vk_buffer m_cube_vertex_buffer;

		std::optional<vren::vk_buffer> m_cube_instance_buffer;
		uint32_t m_cube_idx = 0;
		*/

		vren::vk_render_pass m_render_pass;
		vren::vk_utils::self_described_graphics_pipeline m_points_pipeline;
		vren::vk_utils::self_described_graphics_pipeline m_lines_pipeline;
		vren::vk_utils::self_described_graphics_pipeline m_generic_pipeline;

		vren::vk_render_pass create_render_pass();

		vren::vk_utils::self_described_graphics_pipeline create_graphics_pipeline();

		vren::vk_utils::self_described_graphics_pipeline create_point_graphics_pipeline();
		vren::vk_utils::self_described_graphics_pipeline create_line_graphics_pipeline();
		vren::vk_utils::self_described_graphics_pipeline create_generic_graphics_pipeline();

	public:
		dbg_renderer(vren::vk_utils::toolbox const& toolbox);
		~dbg_renderer();

		void clear();

		void draw_point(point p);
		void draw_line(line l);

		void render(
			uint32_t frame_idx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container
		);
	};
}
