#pragma once

#include <vren/renderer.hpp>
#include <vren/mesh_shader_renderer.hpp>
#include <vren/debug_renderer.hpp>

#include "intermediate_scene.hpp"

namespace vren_demo
{
	// ------------------------------------------------------------------------------------------------
	// Basic renderer
	// ------------------------------------------------------------------------------------------------

	vren::basic_renderer_draw_buffer upload_scene_for_basic_renderer(
		vren::context const& context,
		vren_demo::intermediate_scene const& intermediate_scene
	);

	// ------------------------------------------------------------------------------------------------
	// Mesh shader renderer uploader
	// ------------------------------------------------------------------------------------------------

	void get_clusterized_scene_requested_buffer_sizes(
		vren::vertex const* vertices,
		uint32_t const* indices,
		vren::mesh_instance const* instances,
		vren_demo::intermediate_scene::mesh const* meshes,
		size_t mesh_count,
		size_t& meshlet_vertex_buffer_size,   // The number of uint32_t needed to write the clusterized scene
		size_t& meshlet_triangle_buffer_size, // The number of uint8_t needed
		size_t& meshlet_buffer_size,          // The number of vren::meshlet needed
		size_t& instanced_meshlet_buffer_size  // The number of vren::mesh_instance needed
	);

	void clusterize_mesh(
		vren::vertex const* vertices,
		uint32_t const* indices,
		vren_demo::intermediate_scene::mesh const& mesh,
		uint32_t* meshlet_vertices, // Write
		uint8_t* meshlet_triangles, // Write
		vren::meshlet* meshlets,    // Write
		size_t& meshlet_count       // Write
	);

	void clusterize_scene(
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
	);

	void write_debug_information_for_meshlet_geometry(
		vren::vertex const* vertices,
		uint32_t const* meshlet_vertices,
		uint8_t const* meshlet_triangles,
		vren::meshlet const* meshlets,
		vren::instanced_meshlet const* instanced_meshlets,
		size_t instanced_meshlet_count,
		vren::mesh_instance const* instances,
		std::vector<vren::debug_renderer_line>& lines // Write
	);

	void write_debug_information_for_meshlet_bounds(
		vren::meshlet const* meshlets,
		vren::instanced_meshlet const* instanced_meshlets,
		size_t instanced_meshlet_count,
		vren::mesh_instance const* instances,
		std::vector<vren::debug_renderer_sphere>& spheres // Write
	);

	vren::mesh_shader_renderer_draw_buffer upload_scene_for_mesh_shader_renderer(
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
	);
}
