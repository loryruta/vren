#include "model_clusterizer.hpp"

#define VREN_USE_MESH_OPTIMIZER

#ifdef VREN_USE_MESH_OPTIMIZER
#	include <meshoptimizer.h>
#endif

void vren::model_clusterizer::reserve_buffer_space(vren::clusterized_model& output, vren::model const& model)
{
	size_t max_meshlets = 0;
	size_t max_instanced_meshlets = 0;

	for (auto const& mesh : model.m_meshes)
	{
		size_t max_meshlets_per_mesh;

#ifdef VREN_USE_MESH_OPTIMIZER
		max_meshlets_per_mesh = meshopt_buildMeshletsBound(mesh.m_index_count, vren::k_max_meshlet_vertex_count, vren::k_max_meshlet_primitive_count);
#else
		max_meshlets_per_mesh = mesh.m_index_count / 3;
#endif

		max_meshlets += max_meshlets_per_mesh;
		max_instanced_meshlets += max_meshlets_per_mesh * mesh.m_instance_count;
	}

	output.m_meshlet_vertices.reserve(max_meshlets * vren::k_max_meshlet_vertex_count);
	output.m_meshlet_triangles.reserve(max_meshlets * vren::k_max_meshlet_primitive_count * 3);
	output.m_meshlets.reserve(max_meshlets);
	output.m_instanced_meshlets.reserve(max_instanced_meshlets);
}

void vren::model_clusterizer::clusterize_mesh(vren::clusterized_model& output, vren::model const& model, vren::model::mesh const& mesh)
{
#ifdef VREN_USE_MESH_OPTIMIZER
	size_t meshopt_max_meshlets = meshopt_buildMeshletsBound(mesh.m_index_count, vren::k_max_meshlet_vertex_count, vren::k_max_meshlet_primitive_count);
	std::vector<meshopt_Meshlet> meshopt_meshlets(meshopt_max_meshlets);

	float const* vertices = reinterpret_cast<float const*>(&model.m_vertices.front() + mesh.m_vertex_offset);
	uint32_t const* indices = &model.m_indices.front() + mesh.m_index_offset;

	size_t meshlet_vertices_offset = output.m_meshlet_vertices.size();
	size_t meshlet_triangles_offset = output.m_meshlet_triangles.size();

	output.m_meshlet_vertices.resize(meshlet_vertices_offset + meshopt_max_meshlets * vren::k_max_meshlet_vertex_count);
	output.m_meshlet_triangles.resize(meshlet_triangles_offset + meshopt_max_meshlets * vren::k_max_meshlet_primitive_count * 3);

	size_t meshlet_count = meshopt_buildMeshlets(
		meshopt_meshlets.data(),
		&output.m_meshlet_vertices[meshlet_vertices_offset],
		&output.m_meshlet_triangles[meshlet_triangles_offset],
		indices,
		mesh.m_index_count,
		vertices,
		mesh.m_vertex_count,
		sizeof(vren::vertex),
		vren::k_max_meshlet_vertex_count,
		vren::k_max_meshlet_primitive_count,
		0.0f // cone_weight
	);

	meshopt_Meshlet const& last_meshlet = meshopt_meshlets[meshlet_count - 1];
	output.m_meshlet_vertices.resize(meshlet_vertices_offset + last_meshlet.vertex_offset + last_meshlet.vertex_count);
	output.m_meshlet_triangles.resize(meshlet_triangles_offset + last_meshlet.triangle_offset + ((last_meshlet.triangle_count * 3 + 3) & ~3));

	for (meshopt_Meshlet const& meshopt_meshlet : meshopt_meshlets)
	{
		meshopt_Bounds meshopt_meshlet_bounds = meshopt_computeMeshletBounds(
			&output.m_meshlet_vertices[meshlet_vertices_offset + meshopt_meshlet.vertex_offset],
			&output.m_meshlet_triangles[meshlet_triangles_offset + meshopt_meshlet.triangle_offset],
			meshopt_meshlet.triangle_count,
			vertices,
			mesh.m_vertex_count,
			sizeof(vren::vertex)
		);

		output.m_meshlets.push_back(vren::meshlet{
			.m_vertex_offset   = meshopt_meshlet.vertex_offset + (uint32_t) meshlet_vertices_offset,
			.m_vertex_count    = meshopt_meshlet.vertex_count,
			.m_triangle_offset = meshopt_meshlet.triangle_offset + (uint32_t) meshlet_triangles_offset,
			.m_triangle_count  = meshopt_meshlet.triangle_count,
			.m_bounding_sphere = {
				.m_center = glm::vec3(meshopt_meshlet_bounds.center[0], meshopt_meshlet_bounds.center[1], meshopt_meshlet_bounds.center[2]),
				.m_radius = meshopt_meshlet_bounds.radius
			}
		});
	}

	for (uint32_t i = meshlet_vertices_offset; i < output.m_meshlet_vertices.size(); i++)
	{
		// Since we're erasing the concept of mesh, we need to offset the meshlet' vertices by the mesh' vertex offset
		output.m_meshlet_vertices[i] += mesh.m_vertex_offset;
	}
#else
	// TODO

	meshlet_count = vren::clusterize_mesh(
		reinterpret_cast<float const*>(vertices + mesh.m_vertex_offset),
		sizeof(vren::vertex) / sizeof(float),
		indices + mesh.m_index_offset,
		mesh.m_index_count,
		meshlet_vertices,
		meshlet_triangles,
		meshlets
	);
#endif
}

void vren::model_clusterizer::clusterize_model(vren::clusterized_model& output, vren::model const& model)
{
	for (auto const& mesh : model.m_meshes)
	{
		size_t meshlet_count = output.m_meshlets.size();

		// Clusterize the current mesh and append the result
		clusterize_mesh(output, model, mesh);

		// Create an instanced meshlet for every instance and meshlet
		for (uint32_t instance_idx = 0; instance_idx < mesh.m_instance_count; instance_idx++)
		{
			for (uint32_t meshlet_idx = meshlet_count; meshlet_idx < output.m_meshlets.size(); meshlet_idx++)
			{
				output.m_instanced_meshlets.push_back(vren::instanced_meshlet{
					.m_meshlet_idx  = meshlet_idx,
					.m_instance_idx = mesh.m_instance_offset + instance_idx,
					.m_material_idx = mesh.m_material_idx
				});
			}
		}
	}

	output.m_vertices = model.m_vertices;
	output.m_instances = model.m_instances;
}

vren::clusterized_model vren::model_clusterizer::clusterize(vren::model const& model)
{
	vren::clusterized_model result{};

	reserve_buffer_space(result, model);

	clusterize_model(result, model);

	return std::move(result);
}
