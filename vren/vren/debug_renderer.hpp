#pragma once

#include "base/resource_container.hpp"
#include "vk_helpers/vk_raii.hpp"
#include "vk_helpers/buffer.hpp"
#include "vk_helpers/shader.hpp"
#include "renderer.hpp"
#include "render_graph.hpp"

namespace vren
{
	// Forward decl
	class context;
	class debug_renderer;

	// ------------------------------------------------------------------------------------------------
	// Debug renderer geometry
	// ------------------------------------------------------------------------------------------------

	constexpr float k_debug_renderer_point_size = 0.01f;
	constexpr uint32_t k_debug_sphere_segment_count = 16;

#pragma pack(push, 4) // Force the alignment to be 4 so the final struct size will be 16 instead of 24
	struct debug_renderer_vertex
	{
		glm::vec3 m_position;
		uint32_t m_color; // ARGB (e.g. 0xFF0000 is red)
	};
#pragma pack(pop)

	struct debug_renderer_point
	{
		glm::vec3 m_position;
		uint32_t m_color;
	};

	struct debug_renderer_line
	{
		glm::vec3 m_from, m_to;
		uint32_t m_color;
	};

	struct debug_renderer_sphere
	{
		glm::vec3 m_center;
		float m_radius;
		uint32_t m_color;
	};

	// ------------------------------------------------------------------------------------------------
	// Debug renderer draw buffer
	// ------------------------------------------------------------------------------------------------

	class debug_renderer_draw_buffer
	{
		friend vren::debug_renderer;

	private:
		vren::vk_utils::resizable_buffer m_vertex_buffer;
		size_t m_vertex_count = 0;

	public:
		explicit debug_renderer_draw_buffer(vren::context const& context);

		void clear();

		void add_lines(vren::debug_renderer_line const* lines, size_t line_count);

		inline void add_line(vren::debug_renderer_line const& line)
		{
			add_lines(&line, 1);
		}

		void add_points(vren::debug_renderer_point const* points, size_t point_count);

		inline void add_point(vren::debug_renderer_point const& point)
		{
			add_points(&point, 1);
		}

		void add_spheres(vren::debug_renderer_sphere const* spheres, size_t sphere_count);

		inline void add_sphere(vren::debug_renderer_sphere const& sphere)
		{
			add_spheres(&sphere, 1);
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Debug renderer
	// ------------------------------------------------------------------------------------------------

	class debug_renderer
	{
		struct instance_data
		{
			glm::mat4 m_transform;
			glm::vec3 m_color;
		};

	private:
		vren::context const* m_context;

		vren::vk_utils::pipeline m_pipeline;

	public:
		explicit debug_renderer(vren::context const& context);

	private:
		vren::vk_utils::pipeline create_graphics_pipeline();

	public:
		vren::render_graph::node* render(
			vren::render_graph::allocator& render_graph_allocator,
			vren::render_target const& render_target,
			vren::camera const& camera,
			vren::debug_renderer_draw_buffer const& draw_buffer
		);
	};
}
