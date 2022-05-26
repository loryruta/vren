#pragma once

#include <string>
#include <cstdint>
#include <vector>

#include "mesh.hpp"
#include "gpu_repr.hpp"

namespace vren
{
	struct clusterized_model
	{
		std::string m_name = "unnamed";

		std::vector<vren::vertex> m_vertices;
		std::vector<uint32_t> m_meshlet_vertices;
		std::vector<uint8_t> m_meshlet_triangles;
		std::vector<vren::meshlet> m_meshlets;
		std::vector<vren::instanced_meshlet> m_instanced_meshlets;
		std::vector<vren::mesh_instance> m_instances;
	};
}
