#pragma once

#include <vector>

#include <glm/glm.hpp>

namespace vren
{
	struct camera
	{
		glm::vec3 m_position; float _pad;
		glm::mat4 m_view;
		glm::mat4 m_projection;
	};

	struct vertex
	{
		glm::vec3 m_position;    float _pad;
		glm::vec3 m_normal;      float _pad1;
		glm::vec2 m_texcoords;   float _pad2[2];
		uint32_t m_material_idx; float _pad3[3];
	};

	struct mesh_instance
	{
		glm::mat4 m_transform;
	};

	struct mesh
	{
		size_t m_vertex_offset, m_vertex_count;
		size_t m_index_offset, m_index_count;
		size_t m_instance_offset, m_instance_count;

		uint32_t m_material_idx;
	};

	struct mesh_cluster{};

	void build_meshlets();

	void build_meshlet_groups(
		float* vertices,
		size_t vertex_count,
		uint32_t* indices,
		size_t index_count,
		size_t max_meshlets_per_group,
		std::vector<mesh_cluster>& mesh_clusters
	);
}

