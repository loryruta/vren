#pragma once

#include <vector>

#include "vren/gpu_repr.hpp"

namespace vren_demo
{
	class intermediate_scene
	{
	public:
		struct mesh
		{
			uint32_t m_vertex_offset, m_vertex_count;
			uint32_t m_index_offset, m_index_count;
			uint32_t m_instance_offset, m_instance_count;
			uint32_t m_material_idx;
		};

	public:
		std::vector<vren::vertex> m_vertices;
		std::vector<uint32_t> m_indices;
		std::vector<vren::mesh_instance> m_instances;

		std::vector<mesh> m_meshes;
	};
};
