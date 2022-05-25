#include "scene_gpu_uploader.hpp"

#include <execution>

#include <vren/mesh/mesh.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/gpu_repr.hpp>
#include "vren/log.hpp"

void vren_demo::write_debug_information_for_meshlet_geometry(
	vren::vertex const* vertices,
	uint32_t const* meshlet_vertices,
	uint8_t const* meshlet_triangles,
	vren::meshlet const* meshlets,
	vren::instanced_meshlet const* instanced_meshlets,
	size_t instanced_meshlet_count,
	vren::mesh_instance const* instances,
	std::vector<vren::debug_renderer_line>& lines // Write
)
{
	for (uint32_t i = 0; i < instanced_meshlet_count; i++)
	{
		vren::instanced_meshlet const& instanced_meshlet = instanced_meshlets[i];

		uint32_t color = std::hash<uint32_t>()(i);

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

			lines.push_back({ .m_from = p0, .m_to = p1, .m_color = color });
			lines.push_back({ .m_from = p1, .m_to = p2, .m_color = color });
			lines.push_back({ .m_from = p2, .m_to = p0, .m_color = color });
		}
	}
}

void vren_demo::write_debug_information_for_meshlet_bounds(
	vren::meshlet const* meshlets,
	vren::instanced_meshlet const* instanced_meshlets,
	size_t instanced_meshlet_count,
	vren::mesh_instance const* instances,
	std::vector<vren::debug_renderer_sphere>& spheres // Write
)
{
	//spheres.push_back({ .m_center = glm::vec3(0), .m_radius = 0.01f, .m_color = 0xffffff });

	for (uint32_t i = 0; i < instanced_meshlet_count; i++)
	{
		vren::instanced_meshlet const& instanced_meshlet = instanced_meshlets[i];

		uint32_t color = std::hash<uint32_t>()(i);

		vren::mesh_instance const& instance = instances[instanced_meshlet.m_instance_idx];
		vren::meshlet const& meshlet = meshlets[instanced_meshlet.m_meshlet_idx];

		glm::vec3 center = glm::vec3(instance.m_transform * glm::vec4(meshlet.m_bounding_sphere.m_center, 1.0f));

		glm::vec3 scale(
			glm::length(instance.m_transform[0]),
			glm::length(instance.m_transform[1]),
			glm::length(instance.m_transform[2])
		);
		float radius = glm::max(glm::max(scale.x, scale.y), scale.z) * meshlet.m_bounding_sphere.m_radius;

		spheres.push_back({ .m_center = center, .m_radius = radius, .m_color = color });
	}
}
