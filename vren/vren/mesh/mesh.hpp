#pragma once

#include <cstdint>

#include "base/base.hpp"

namespace vren
{
	struct meshlet
	{
		// The number of uint32_t to skip before reaching the current meshlet' vertices
		uint32_t m_vertex_offset;

		// The number of uint32_t held by this meshlet
		uint32_t m_vertex_count;

		// The number of uint8_t to skip before reaching the current meshlet' triangles
		uint32_t m_triangle_offset;

		// The number of triangles of the meshlet (actual uint8_t count will be m_triangle_count * 3)
		uint32_t m_triangle_count;

		vren::bounding_sphere m_bounding_sphere;
	};

	inline constexpr size_t k_max_meshlet_vertex_count = 64;
	inline constexpr size_t k_max_meshlet_primitive_count = 124;

	size_t clusterize_mesh(float const* vertices, size_t vertex_stride, uint32_t const* indices, size_t index_count, uint32_t* meshlet_vertices, uint8_t* meshlet_triangles, vren::meshlet* meshlets);
}
