#include "scene_gpu_uploader.hpp"

#include <meshoptimizer.h>

#include <vren/vk_helpers/buffer.hpp>
#include <vren/gpu_repr.hpp>

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
		.m_index_buffer    = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, intermediate_scene.m_indices.data(), intermediate_scene.m_indices.size() * sizeof(vren::index_t)),
		.m_instance_buffer = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, intermediate_scene.m_instances.data(), intermediate_scene.m_instances.size() * sizeof(vren::mesh_instance)),
		.m_meshes          = std::move(meshes)
	};

	vren::set_object_names(context, draw_buffer);

	return draw_buffer;
}

vren::mesh_shader_renderer_draw_buffer vren_demo::upload_scene_for_mesh_shader_renderer(
	vren::context const& context,
	vren_demo::intermediate_scene const& intermediate_scene
)
{
	const size_t k_max_vertices = 64;
	const size_t k_max_triangles = 124;
	const float k_cone_weight = 0.0f;

	size_t max_meshlets = 0;
	size_t max_instanced_meshlets = 0;

	for (auto const& mesh : intermediate_scene.m_meshes)
	{
		size_t current_max_meshlets = meshopt_buildMeshletsBound(mesh.m_index_count, k_max_vertices, k_max_triangles);

		max_meshlets += current_max_meshlets;
		max_instanced_meshlets += current_max_meshlets * mesh.m_instance_count;
	}

	std::vector<uint32_t> meshlet_vertices(max_meshlets * k_max_vertices);
	std::vector<uint8_t> meshlet_triangles(max_meshlets * k_max_triangles * 3);
	std::vector<vren::meshlet_t> meshlets(max_meshlets);
	std::vector<vren::instanced_meshlet> instanced_meshlets(max_instanced_meshlets);

	size_t
		meshlet_vertex_offset = 0,
		meshlet_triangle_offset = 0,
		meshlet_offset = 0,
		instanced_meshlet_offset = 0;
	for (auto const& mesh : intermediate_scene.m_meshes)
	{
		size_t meshlet_count = meshopt_buildMeshlets(
			meshlets.data() + meshlet_offset,
			meshlet_vertices.data() + meshlet_vertex_offset,
			meshlet_triangles.data() + meshlet_triangle_offset,
			intermediate_scene.m_indices.data() + mesh.m_index_offset,
			mesh.m_index_count,
			reinterpret_cast<float const*>(intermediate_scene.m_vertices.data() + mesh.m_vertex_offset),
			mesh.m_vertex_count,
			sizeof(vren::vertex),
			k_max_vertices,
			k_max_triangles,
			k_cone_weight
		);

		// The meshlet vertex_offset and triangle_offset are local to the sub-portion of the meshlet_vertex_buffer and meshlet_triangle_buffer
		// dedicated to the current mesh. We have to make those indices global as the final buffers will contain multiple meshes with multiple meshlets.
		for (uint32_t i = 0; i < meshlet_count; i++)
		{
			auto& meshlet = meshlets[meshlet_offset + i];
			meshlet.vertex_offset += meshlet_vertex_offset;
			meshlet.triangle_offset += meshlet_triangle_offset;
		}

		// Now we have to iterate over all the meshlets and instances in order to create the final instanced_meshlet_buffer that will be the input
		// for the task shader + mesh shader pipeline.
		for (uint32_t instance_idx = 0; instance_idx < mesh.m_instance_count; instance_idx++)
		{
			for (uint32_t meshlet_idx = 0; meshlet_idx < meshlet_count; meshlet_idx++)
			{
				uint32_t i = instance_idx * meshlet_count + meshlet_idx;
				instanced_meshlets[instanced_meshlet_offset + i] = {
					.m_meshlet_idx  = (uint32_t) meshlet_offset + meshlet_idx,
					.m_instance_idx = mesh.m_instance_offset + instance_idx,
					.m_material_idx = mesh.m_material_idx
				};
			}
		}

		size_t from_vertex_offset = meshlet_vertex_offset;

		// Take the last generated meshlet to see how many vertices and primitives it has generated. Account them to update the meshlet_vertex_buffer and
		// the meshlet_triangle_buffer.
		auto const& last_meshlet = meshlets.at(meshlet_offset + meshlet_count - 1);
		meshlet_vertex_offset = last_meshlet.vertex_offset + last_meshlet.vertex_count;
		meshlet_triangle_offset = last_meshlet.triangle_offset + ((last_meshlet.triangle_count * 3 + 3) & ~3);
		meshlet_offset += meshlet_count;
		instanced_meshlet_offset += mesh.m_instance_count * meshlet_count;

		// Finally we have to offset the meshlet_vertex_buffer entries, therefore the indices of the meshlet pointing to the vertex buffer.
		for (uint32_t i = from_vertex_offset; i < meshlet_vertex_offset; i++)
		{
			meshlet_vertices[i] += mesh.m_vertex_offset;
		}
	}

	meshlet_vertices.resize(meshlet_vertex_offset);
	meshlet_triangles.resize(meshlet_triangle_offset);
	meshlets.resize(meshlet_offset);
	instanced_meshlets.resize(instanced_meshlet_offset);

	// TODO save the obtained buffers on a cache

	// Uploading
	auto vertex_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, intermediate_scene.m_vertices.data(), intermediate_scene.m_vertices.size() * sizeof(vren::vertex));

	auto meshlet_vertex_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlet_vertices.data(), meshlet_vertices.size() * sizeof(uint32_t));

	auto meshlet_triangle_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlet_triangles.data(), meshlet_triangles.size() * sizeof(uint8_t));

	auto meshlet_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlets.data(), meshlets.size() * sizeof(vren::meshlet_t));

	auto instanced_meshlet_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, instanced_meshlets.data(), instanced_meshlets.size() * sizeof(vren::instanced_meshlet));

	auto instance_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, intermediate_scene.m_instances.data(), intermediate_scene.m_instances.size() * sizeof(vren::mesh_instance));

	vren::mesh_shader_renderer_draw_buffer draw_buffer{
		.m_vertex_buffer            = std::move(vertex_buffer),
		.m_meshlet_vertex_buffer    = std::move(meshlet_vertex_buffer),
		.m_meshlet_triangle_buffer  = std::move(meshlet_triangle_buffer),
		.m_meshlet_buffer           = std::move(meshlet_buffer),
		.m_instanced_meshlet_buffer = std::move(instanced_meshlet_buffer),
		.m_instanced_meshlet_count  = instanced_meshlets.size(),
		.m_instance_buffer          = std::move(instance_buffer)
	};

	vren::set_object_names(context, draw_buffer);

	return std::move(draw_buffer);
}