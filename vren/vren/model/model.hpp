#pragma once

#include <vector>
#include <string>
#include <limits>

#include "gpu_repr.hpp"

namespace vren
{
	class model
	{
	public:
		struct mesh
		{
			uint32_t m_vertex_offset, m_vertex_count;
			uint32_t m_index_offset, m_index_count;
			uint32_t m_instance_offset, m_instance_count;
			uint32_t m_material_idx;

			glm::vec3 m_min = glm::vec3(std::numeric_limits<float>::infinity());
			glm::vec3 m_max = glm::vec3(-std::numeric_limits<float>::infinity());
		};

	public:
		std::string m_name = "unnamed";

		std::vector<vren::vertex> m_vertices;
		std::vector<uint32_t> m_indices;
		std::vector<vren::mesh_instance> m_instances;
		std::vector<mesh> m_meshes;

		glm::vec3 m_min = glm::vec3(std::numeric_limits<float>::infinity());
		glm::vec3 m_max = glm::vec3(-std::numeric_limits<float>::infinity());
	
		void compute_aabb();
	};
}
