#include "scene_gpu_uploader.hpp"

#include <execution>


#include <vren/mesh/mesh.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/gpu_repr.hpp>
#include <vren/utils/log.hpp>

//#define VREN_USE_MESH_OPTIMIZER

#ifdef VREN_USE_MESH_OPTIMIZER
#	include <meshoptimizer.h>
#endif

vren::basic_renderer_draw_buffer vren_demo::upload_scene_for_basic_renderer(
	vren::context const& context,
	vren_demo::intermediate_scene const& intermediate_scene
)
{
	std::vector<vren::basic_renderer_draw_buffer::mesh> meshes(intermediate_scene.m_meshes.size());
	std::memcpy(meshes.data(), intermediate_scene.m_meshes.data(), intermediate_scene.m_meshes.size() * sizeof(vren_demo::intermediate_scene::mesh));

	// TODO save the obtained buffers on a cache

	// Uploading
	vren::basic_renderer_draw_buffer draw_buffer{
		.m_vertex_buffer   = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, intermediate_scene.m_vertices.data(), intermediate_scene.m_vertices.size() * sizeof(vren::vertex)),
		.m_index_buffer    = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, intermediate_scene.m_indices.data(), intermediate_scene.m_indices.size() * sizeof(uint32_t)),
		.m_instance_buffer = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, intermediate_scene.m_instances.data(), intermediate_scene.m_instances.size() * sizeof(vren::mesh_instance)),
		.m_meshes          = std::move(meshes)
	};

	vren::set_object_names(context, draw_buffer);

	return draw_buffer;
}

// --------------------------------------------------------------------------------------------------------------------------------
// Mesh shader renderer uploader
// --------------------------------------------------------------------------------------------------------------------------------

void vren_demo::get_clusterized_scene_requested_buffer_sizes(
	vren::vertex const* vertices,
	uint32_t const* indices,
	vren::mesh_instance const* instances,
	vren_demo::intermediate_scene::mesh const* meshes,
	size_t mesh_count,
	size_t& meshlet_vertex_buffer_size,   // Write
	size_t& meshlet_triangle_buffer_size, // Write
	size_t& meshlet_buffer_size,          // Write
	size_t& instanced_meshlet_buffer_size // Write
)
{
	size_t max_meshlets = 0;
	size_t max_instanced_meshlets = 0;

	for (uint32_t i = 0; i < mesh_count; i++)
	{
		size_t max_meshlets_per_mesh;

#ifdef VREN_USE_MESH_OPTIMIZER
		max_meshlets_per_mesh = meshopt_buildMeshletsBound(meshes[i].m_index_count, vren::k_max_meshlet_vertex_count, vren::k_max_meshlet_primitive_count);
#else
		max_meshlets_per_mesh = meshes[i].m_index_count / 3;
#endif

		max_meshlets += max_meshlets_per_mesh;
		max_instanced_meshlets += max_meshlets_per_mesh * meshes[i].m_instance_count;
	}

	meshlet_vertex_buffer_size   = max_meshlets * vren::k_max_meshlet_vertex_count;
	meshlet_triangle_buffer_size = max_meshlets * vren::k_max_meshlet_primitive_count * 3;
	meshlet_buffer_size          = max_meshlets;
	instanced_meshlet_buffer_size = max_instanced_meshlets;
}

void vren_demo::clusterize_mesh(
	vren::vertex const* vertices,
	uint32_t const* indices,
	vren_demo::intermediate_scene::mesh const& mesh,
	uint32_t* meshlet_vertices, // Write
	uint8_t* meshlet_triangles, // Write
	vren::meshlet* meshlets,    // Write
	size_t& meshlet_count       // Write
)
{
#ifdef VREN_USE_MESH_OPTIMIZER
	size_t meshopt_max_meshlet_count = meshopt_buildMeshletsBound(mesh.m_index_count, vren::k_max_meshlet_vertex_count, vren::k_max_meshlet_primitive_count);
	std::vector<meshopt_Meshlet> meshopt_meshlets(meshopt_max_meshlet_count);

	meshlet_count = meshopt_buildMeshlets(
		meshopt_meshlets.data(),
		meshlet_vertices,
		meshlet_triangles,
		indices + mesh.m_index_offset,
		mesh.m_index_count,
		reinterpret_cast<float const*>(vertices + mesh.m_vertex_offset),
		mesh.m_vertex_count,
		sizeof(vren::vertex),
		vren::k_max_meshlet_vertex_count,
		vren::k_max_meshlet_primitive_count,
		0.0f // k_cone_weight
	);

	for (uint32_t i = 0; i < meshlet_count; i++)
	{
		meshopt_Meshlet const& meshlet = meshopt_meshlets.at(i);
		meshopt_Bounds meshopt_meshlet_bounds = meshopt_computeMeshletBounds(
			&meshlet_vertices[meshlet.vertex_offset],
			&meshlet_triangles[meshlet.triangle_offset],
			meshlet.triangle_count,
			reinterpret_cast<float const*>(vertices + mesh.m_vertex_offset),
			mesh.m_vertex_count,
			sizeof(vren::vertex)
		);

		meshlets[i] = {
			.m_vertex_offset   = meshopt_meshlets[i].vertex_offset,
			.m_vertex_count    = meshopt_meshlets[i].vertex_count,
			.m_triangle_offset = meshopt_meshlets[i].triangle_offset,
			.m_triangle_count  = meshopt_meshlets[i].triangle_count,
			.m_bounding_sphere = {
				.m_center = glm::vec3(meshopt_meshlet_bounds.center[0], meshopt_meshlet_bounds.center[1], meshopt_meshlet_bounds.center[2]),
				.m_radius = meshopt_meshlet_bounds.radius
			}
		};
	}
#else
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

void vren_demo::clusterize_scene(
	vren::vertex const* vertices,
	uint32_t const* indices,
	vren::mesh_instance const* instances,
	vren_demo::intermediate_scene::mesh const* meshes,
	size_t mesh_count,
	uint32_t* meshlet_vertices, // Write
	size_t& meshlet_vertex_count, // Write
	uint8_t* meshlet_triangles, // Write
	size_t& meshlet_triangle_count,
	vren::meshlet* meshlets, // Write
	size_t& meshlet_count,  // Write
	vren::instanced_meshlet* instanced_meshlets, // Write
	size_t& instanced_meshlet_count // Write
)
{
	size_t
		meshlet_vertex_offset = 0,
		meshlet_triangle_offset = 0,
		meshlet_offset = 0,
		instanced_meshlet_offset = 0;
	for (uint32_t i = 0; i < mesh_count; i++)
	{
		vren_demo::intermediate_scene::mesh const& mesh = meshes[i];

		size_t per_mesh_meshlet_count;
		vren_demo::clusterize_mesh(
			vertices,
			indices,
			mesh,
			meshlet_vertices + meshlet_vertex_offset,
			meshlet_triangles + meshlet_triangle_offset,
			meshlets + meshlet_offset,
			per_mesh_meshlet_count
		);

		// The meshlet vertex_offset and triangle_offset are local to the sub-portion of the meshlet_vertex_buffer and meshlet_triangle_buffer
		// dedicated to the current mesh. We have to make those indices global as the final buffers will contain multiple meshes with multiple meshlets.
		for (uint32_t j = 0; j < per_mesh_meshlet_count; j++)
		{
			auto& meshlet = meshlets[meshlet_offset + j];
			meshlet.m_vertex_offset += meshlet_vertex_offset;
			meshlet.m_triangle_offset += meshlet_triangle_offset;
		}

		// Now we have to iterate over all the meshlets and instances in order to create the final instanced_meshlet_buffer that will be the input
		// for the task shader + mesh shader pipeline.
		for (uint32_t instance_idx = 0; instance_idx < mesh.m_instance_count; instance_idx++)
		{
			for (uint32_t meshlet_idx = 0; meshlet_idx < per_mesh_meshlet_count; meshlet_idx++)
			{
				uint32_t instanced_meshlet_idx = instance_idx * per_mesh_meshlet_count + meshlet_idx;
				instanced_meshlets[instanced_meshlet_offset + instanced_meshlet_idx] = {
					.m_meshlet_idx  = (uint32_t) meshlet_offset + meshlet_idx,
					.m_instance_idx = mesh.m_instance_offset + instance_idx,
					.m_material_idx = mesh.m_material_idx
				};
			}
		}

		size_t from_vertex_offset = meshlet_vertex_offset;

		// Take the last generated meshlet to see how many vertices and primitives it has generated. Account them to update the meshlet_vertex_buffer and
		// the meshlet_triangle_buffer.
		auto const& last_meshlet = meshlets[meshlet_offset + per_mesh_meshlet_count - 1];
		meshlet_vertex_offset = last_meshlet.m_vertex_offset + last_meshlet.m_vertex_count;
		meshlet_triangle_offset = last_meshlet.m_triangle_offset + ((last_meshlet.m_triangle_count * 3 + 3) & ~3);
		meshlet_offset += per_mesh_meshlet_count;
		instanced_meshlet_offset += mesh.m_instance_count * per_mesh_meshlet_count;

		// Finally we have to offset the meshlet_vertex_buffer entries, therefore the indices of the meshlet pointing to the vertex buffer.
		for (uint32_t j = from_vertex_offset; j < meshlet_vertex_offset; j++)
		{
			meshlet_vertices[j] += mesh.m_vertex_offset;
		}
	}

	meshlet_vertex_count = meshlet_vertex_offset;
	meshlet_triangle_count = meshlet_triangle_offset;
	meshlet_count = meshlet_offset;
	instanced_meshlet_count = instanced_meshlet_offset;
}

void vren_demo::get_clusterized_scene_debug_draw_requested_buffer_sizes(
	uint8_t const* meshlet_triangles,
	vren::meshlet const* meshlets,
	vren::instanced_meshlet const* instanced_meshlets,
	size_t instanced_meshlet_count,
	size_t& line_buffer_size
)
{
	line_buffer_size = 0;
	for (uint32_t i = 0; i < instanced_meshlet_count; i++)
	{
		vren::meshlet const& meshlet = meshlets[instanced_meshlets[i].m_meshlet_idx];
		line_buffer_size += meshlet.m_triangle_count * 3;
	}
}

void vren_demo::write_clusterized_scene_debug_information(
	vren::vertex const* vertices,
	uint32_t const* meshlet_vertices,
	uint8_t const* meshlet_triangles,
	vren::meshlet const* meshlets,
	vren::instanced_meshlet const* instanced_meshlets,
	size_t instanced_meshlet_count,
	vren::mesh_instance const* instances,
	vren::debug_renderer::line* lines, // Write
	size_t& line_count // Write
)
{
	line_count = 0;
	for (uint32_t i = 0; i < instanced_meshlet_count; i++)
	{
		vren::instanced_meshlet const& instanced_meshlet = instanced_meshlets[i];

		uint32_t color_hash = std::hash<uint32_t>()(i);
		glm::vec3 color = glm::vec3(
			(color_hash & 0xff) / (float) 255.0f,
			((color_hash & 0xff00) >> 8) / (float) 255.0f,
			((color_hash & 0xff0000) >> 16) / (float) 255.0f
		);

		vren::mesh_instance const& instance = instances[instanced_meshlet.m_instance_idx];
		vren::meshlet const& meshlet = meshlets[instanced_meshlet.m_meshlet_idx];

		for (uint32_t j = 0; j < meshlet.m_triangle_count; j++)
		{
			vren::vertex const& v0 = vertices[meshlet_vertices[meshlet.m_vertex_offset + meshlet_triangles[meshlet.m_triangle_offset + j * 3 + 0]]];
			vren::vertex const& v1 = vertices[meshlet_vertices[meshlet.m_vertex_offset + meshlet_triangles[meshlet.m_triangle_offset + j * 3 + 1]]];
			vren::vertex const& v2 = vertices[meshlet_vertices[meshlet.m_vertex_offset + meshlet_triangles[meshlet.m_triangle_offset + j * 3 + 2]]];

			glm::vec3 p0 = glm::vec3(instance.m_transform * glm::vec4(v0.m_position, 1.0f));
			glm::vec3 p1 = glm::vec3(instance.m_transform * glm::vec4(v1.m_position, 1.0f));
			glm::vec3 p2 = glm::vec3(instance.m_transform * glm::vec4(v2.m_position, 1.0f));

			lines[line_count++] = { .m_from = p0, .m_to = p1, .m_color = color };
			lines[line_count++] = { .m_from = p1, .m_to = p2, .m_color = color };
			lines[line_count++] = { .m_from = p2, .m_to = p0, .m_color = color };
		}
	}
}

vren::mesh_shader_renderer_draw_buffer vren_demo::upload_scene_for_mesh_shader_renderer(
	vren::context const& context,
	vren::vertex const* vertices,
	size_t vertex_count,
	uint32_t const* meshlet_vertices,
	size_t meshlet_vertex_count,
	uint8_t const* meshlet_triangles,
	size_t meshlet_triangle_count,
	vren::meshlet const* meshlets,
	size_t meshlet_count,
	vren::instanced_meshlet const* instanced_meshlets,
	size_t instanced_meshlet_count,
	vren::mesh_instance const* instances,
	size_t instance_count
)
{
	auto vertex_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vertices, vertex_count * sizeof(vren::vertex));

	auto meshlet_vertex_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlet_vertices, meshlet_vertex_count * sizeof(uint32_t));

	auto meshlet_triangle_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlet_triangles, meshlet_triangle_count * sizeof(uint8_t));

	auto meshlet_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlets, meshlet_count * sizeof(vren::meshlet));

	auto instanced_meshlet_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, instanced_meshlets, instanced_meshlet_count * sizeof(vren::instanced_meshlet));

	auto instance_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, instances, instance_count * sizeof(vren::mesh_instance));

	vren::mesh_shader_renderer_draw_buffer draw_buffer{
		.m_vertex_buffer            = std::move(vertex_buffer),
		.m_meshlet_vertex_buffer    = std::move(meshlet_vertex_buffer),
		.m_meshlet_triangle_buffer  = std::move(meshlet_triangle_buffer),
		.m_meshlet_buffer           = std::move(meshlet_buffer),
		.m_instanced_meshlet_buffer = std::move(instanced_meshlet_buffer),
		.m_instanced_meshlet_count  = instanced_meshlet_count,
		.m_instance_buffer          = std::move(instance_buffer)
	};

	vren::set_object_names(context, draw_buffer);

	return std::move(draw_buffer);
}
