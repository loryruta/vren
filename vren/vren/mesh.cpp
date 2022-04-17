#include "mesh.hpp"

#include <cassert>
#include <vector>
#include <array>

#include <meshoptimizer.h>


void vren::build_meshlet_groups(
	float* vertices,
	size_t vertex_count,
	uint32_t* indices,
	size_t index_count,
	size_t max_meshlets_per_group,
	std::vector<mesh_cluster>& mesh_clusters
)
{
	const size_t k_max_meshlet_vertices = 64;
	const size_t k_max_meshlet_triangles = 124;

	// build groups
	size_t max_group_vertices = max_meshlets_per_group * k_max_meshlet_vertices;
	size_t max_group_triangles = max_meshlets_per_group * k_max_meshlet_triangles;
	size_t meshlet_group_count = meshopt_buildMeshletsBound(index_count, max_group_vertices, max_group_triangles);

	std::vector<meshopt_Meshlet> groups(meshlet_group_count);
	std::vector<uint32_t> group_vertices(meshlet_group_count * max_group_vertices);
	std::vector<uint8_t> group_triangles(meshlet_group_count * max_group_triangles * 3);

	/*
	size_t meshlet_count = meshopt_buildMeshlets(
		groups.data(),
		group_vertices.data(),
		group_triangles.data(),
		indices, index_count,
		vertices, vertex_count, sizeof(Vertex),
		max_vertices, max_triangles,
		0.0f
	);*/
}
