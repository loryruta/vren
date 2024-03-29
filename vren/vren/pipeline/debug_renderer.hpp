#pragma once

#include "base/resource_container.hpp"
#include "vk_helpers/vk_raii.hpp"
#include "vk_helpers/buffer.hpp"
#include "vk_helpers/shader.hpp"
#include "render_target.hpp"
#include "render_graph.hpp"
#include "gpu_repr.hpp"
#include "camera.hpp"

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

	struct debug_renderer_cube
	{
		glm::vec3 m_min, m_max;
		uint32_t m_color;
	};

	// ------------------------------------------------------------------------------------------------
	// Debug renderer draw buffer
	// ------------------------------------------------------------------------------------------------

	class debug_renderer_draw_buffer
	{
		friend vren::debug_renderer;

	public:
		vren::vk_utils::resizable_buffer m_vertex_buffer;
		size_t m_vertex_count = 0;

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

		void add_cubes(vren::debug_renderer_cube const* cubes, size_t cube_count);

		inline void add_cube(vren::debug_renderer_cube const& cube)
		{
			add_cubes(&cube, 1);
		}
	};

	// ------------------------------------------------------------------------------------------------
	// debug_renderer
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

		vren::pipeline m_pipeline;
		vren::pipeline m_no_depth_test_pipeline;


	public:
		explicit debug_renderer(vren::context const& context);

	private:
		vren::pipeline create_graphics_pipeline(bool depth_test);

	public:
		vren::render_graph_t render(
			vren::render_graph_allocator& render_graph_allocator,
			vren::render_target const& render_target,
			vren::camera_data const& camera_data,
			vren::debug_renderer_draw_buffer const& draw_buffer,
			bool world_space = true
		);
	};
}
