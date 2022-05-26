#include "clusterized_model_debugger.hpp"

#include "vren/log.hpp"

void vren_demo::clusterized_model_debugger::write_debug_info_for_meshlet_geometry(
	vren::clusterized_model const& model,
	vren::debug_renderer_draw_buffer& draw_buffer
)
{
	for (uint32_t i = 0; i < model.m_instanced_meshlets.size(); i++)
	{
		vren::instanced_meshlet const& instanced_meshlet = model.m_instanced_meshlets.at(i);

		uint32_t color = std::hash<uint32_t>()(i);

		vren::mesh_instance const& instance = model.m_instances[instanced_meshlet.m_instance_idx];
		vren::meshlet const& meshlet = model.m_meshlets[instanced_meshlet.m_meshlet_idx];

		for (uint32_t j = 0; j < meshlet.m_triangle_count; j++)
		{
			vren::vertex const& v0 = model.m_vertices[model.m_meshlet_vertices[meshlet.m_vertex_offset + model.m_meshlet_triangles[meshlet.m_triangle_offset + j * 3 + 0]]];
			vren::vertex const& v1 = model.m_vertices[model.m_meshlet_vertices[meshlet.m_vertex_offset + model.m_meshlet_triangles[meshlet.m_triangle_offset + j * 3 + 1]]];
			vren::vertex const& v2 = model.m_vertices[model.m_meshlet_vertices[meshlet.m_vertex_offset + model.m_meshlet_triangles[meshlet.m_triangle_offset + j * 3 + 2]]];

			glm::vec3 p0 = glm::vec3(instance.m_transform * glm::vec4(v0.m_position, 1.0f));
			glm::vec3 p1 = glm::vec3(instance.m_transform * glm::vec4(v1.m_position, 1.0f));
			glm::vec3 p2 = glm::vec3(instance.m_transform * glm::vec4(v2.m_position, 1.0f));

			draw_buffer.add_line({ .m_from = p0, .m_to = p1, .m_color = color });
			draw_buffer.add_line({ .m_from = p1, .m_to = p2, .m_color = color });
			draw_buffer.add_line({ .m_from = p2, .m_to = p0, .m_color = color });
		}
	}
}

void vren_demo::clusterized_model_debugger::write_debug_info_for_meshlet_bounds(
	vren::clusterized_model const& model,
	vren::debug_renderer_draw_buffer& draw_buffer
)
{
	for (uint32_t i = 0; i < model.m_instanced_meshlets.size(); i++)
	{
		vren::instanced_meshlet const& instanced_meshlet = model.m_instanced_meshlets.at(i);

		uint32_t color = std::hash<uint32_t>()(i);

		vren::mesh_instance const& instance = model.m_instances[instanced_meshlet.m_instance_idx];
		vren::meshlet const& meshlet = model.m_meshlets[instanced_meshlet.m_meshlet_idx];

		glm::vec3 center = glm::vec3(instance.m_transform * glm::vec4(meshlet.m_bounding_sphere.m_center, 1.0f));

		glm::vec3 scale(
			glm::length(instance.m_transform[0]),
			glm::length(instance.m_transform[1]),
			glm::length(instance.m_transform[2])
		);
		float radius = glm::max(glm::max(scale.x, scale.y), scale.z) * meshlet.m_bounding_sphere.m_radius;

		draw_buffer.add_sphere({ .m_center = center, .m_radius = radius, .m_color = color });
	}
}

void vren_demo::clusterized_model_debugger::write_debug_info_for_projected_sphere_bounds(
	vren::clusterized_model const& model,
	vren::camera const& camera,
	vren::debug_renderer_draw_buffer& draw_buffer
)
{
	// TODO
}
