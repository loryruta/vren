#pragma once

#include <vector>

#include "vren/gpu_repr.hpp"

namespace vren_demo
{
	class intermediate_scene
	{
	public:
		using vertex_t = vren::vertex;
		using index_t = vren::index_t;
		using instance_t = vren::mesh_instance;

		struct mesh
		{
			uint32_t m_vertex_offset, m_vertex_count;
			uint32_t m_index_offset, m_index_count;
			uint32_t m_instance_offset, m_instance_count;
			uint32_t m_material_idx;
		};

	public:
		std::vector<vertex_t> m_vertices;
		std::vector<index_t> m_indices;
		std::vector<instance_t> m_instances;

		std::vector<mesh> m_meshes;
	};
};
