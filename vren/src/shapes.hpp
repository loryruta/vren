#pragma once

#include <numeric>

#include "render_object.hpp"

namespace vren::utils
{
	// TODO Make a singleton of must used shapes (buffer, sphere...) in toolbox?

	void create_cube(
		vren::vk_utils::toolbox const& toolbox,
		vren::render_object& render_obj
	)
	{
		/* Vertex buffer */
		vren::vertex vertices[]{
			// Bottom face
			{ .m_position = { 0, 0, 1 }, .m_normal = { 0, -1, 0 } },
			{ .m_position = { 0, 0, 0 }, .m_normal = { 0, -1, 0 } },
			{ .m_position = { 1, 0, 0 }, .m_normal = { 0, -1, 0 } },
			{ .m_position = { 1, 0, 0 }, .m_normal = { 0, -1, 0 } },
			{ .m_position = { 1, 0, 1 }, .m_normal = { 0, -1, 0 } },
			{ .m_position = { 0, 0, 1 }, .m_normal = { 0, -1, 0 } },

			// Top face
			{ .m_position = { 0, 1, 1 }, .m_normal = { 0, 1, 0 } },
			{ .m_position = { 0, 1, 0 }, .m_normal = { 0, 1, 0 } },
			{ .m_position = { 1, 1, 0 }, .m_normal = { 0, 1, 0 } },
			{ .m_position = { 1, 1, 0 }, .m_normal = { 0, 1, 0 } },
			{ .m_position = { 1, 1, 1 }, .m_normal = { 0, 1, 0 } },
			{ .m_position = { 0, 1, 1 }, .m_normal = { 0, 1, 0 } },

			// Left face
			{ .m_position = { 0, 1, 0 }, .m_normal = { -1, 0, 0 } },
			{ .m_position = { 0, 0, 0 }, .m_normal = { -1, 0, 0 } },
			{ .m_position = { 0, 0, 1 }, .m_normal = { -1, 0, 0 } },
			{ .m_position = { 0, 0, 1 }, .m_normal = { -1, 0, 0 } },
			{ .m_position = { 0, 1, 1 }, .m_normal = { -1, 0, 0 } },
			{ .m_position = { 0, 1, 0 }, .m_normal = { -1, 0, 0 } },

			// Right face
			{ .m_position = { 1, 1, 0 }, .m_normal = { 1, 0, 0 } },
			{ .m_position = { 1, 0, 0 }, .m_normal = { 1, 0, 0 } },
			{ .m_position = { 1, 0, 1 }, .m_normal = { 1, 0, 0 } },
			{ .m_position = { 1, 0, 1 }, .m_normal = { 1, 0, 0 } },
			{ .m_position = { 1, 1, 1 }, .m_normal = { 1, 0, 0 } },
			{ .m_position = { 1, 1, 0 }, .m_normal = { 1, 0, 0 } },

			// Back face
			{ .m_position = { 0, 0, 0 }, .m_normal = { 0, 0, -1 } },
			{ .m_position = { 0, 1, 0 }, .m_normal = { 0, 0, -1 } },
			{ .m_position = { 1, 1, 0 }, .m_normal = { 0, 0, -1 } },
			{ .m_position = { 1, 1, 0 }, .m_normal = { 0, 0, -1 } },
			{ .m_position = { 1, 0, 0 }, .m_normal = { 0, 0, -1 } },
			{ .m_position = { 0, 0, 0 }, .m_normal = { 0, 0, -1 } },

			// Front face
			{ .m_position = { 0, 0, 1 }, .m_normal = { 0, 0, 1 } },
			{ .m_position = { 0, 1, 1 }, .m_normal = { 0, 0, 1 } },
			{ .m_position = { 1, 1, 1 }, .m_normal = { 0, 0, 1 } },
			{ .m_position = { 1, 1, 1 }, .m_normal = { 0, 0, 1 } },
			{ .m_position = { 1, 0, 1 }, .m_normal = { 0, 0, 1 } },
			{ .m_position = { 0, 0, 1 }, .m_normal = { 0, 0, 1 } },
		};
		size_t vertices_count = std::size(vertices);
		auto vertices_buf = std::make_shared<vren::vk_utils::buffer>(
			vren::vk_utils::create_vertex_buffer(toolbox, vertices, vertices_count)
		);
		render_obj.set_vertices_buffer(vertices_buf, vertices_count);

		/* Index buffer */
		std::vector<uint32_t> indices(vertices_count);
		std::iota(indices.begin(), indices.end(), 0);

		auto indices_buf =
			std::make_shared<vren::vk_utils::buffer>(
				vren::vk_utils::create_indices_buffer(toolbox, indices.data(), indices.size())
			);
		render_obj.set_indices_buffer(indices_buf, indices.size());
	}
}
