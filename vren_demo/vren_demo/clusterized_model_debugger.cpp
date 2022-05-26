#include "clusterized_model_debugger.hpp"

#include <glm/glm.hpp>

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

bool project_sphere(glm::vec3 C, float r, vren::camera const& camera, glm::vec4& aabb) // C and r in camera space
{
	using namespace glm;
	using glm::isnan;

	float m00 = camera.m_projection[0][0];
	float m11 = camera.m_projection[1][1];
	float m22 = camera.m_projection[2][2];
	float m32 = camera.m_projection[3][2];

	if (C.z + r < camera.m_z_near) // The sphere is entirely clipped by the near plane
	{
		//debugPrintfEXT("CLIPPED BY NEAR-PLANE: ID: %d, C.z: %.2f, r: %.2f, m22: %.2f, m32: %.2f\n", gl_GlobalInvocationID.x, C.z, r, m22, m32);
		return false; // The sphere is entirely clipped by the near plane
	}

	float k = sqrt(r * r - (camera.m_z_near - C.z) * (camera.m_z_near - C.z));

	// Consider XZ plane to find minX and maxX
	vec2 minx, maxx;

	vec2 cx = vec2(C.x, C.z);
	float cxl = length(cx);
	float tx = sqrt(dot(cx, cx) - r * r);
	minx = mat2(tx, r, -r, tx) * (cx / cxl);
	maxx = mat2(tx, -r, r, tx) * (cx / cxl);

	// If (dot(cx, cx) - r * r) < 0 then tx is NaN, or if minx or maxx are behind the near plane, we use the intersections of the circle with the near plane
	if (isnan(tx) || minx.y < camera.m_z_near) minx = vec2(C.x - k, camera.m_z_near);
	if (isnan(tx) || maxx.y < camera.m_z_near) maxx = vec2(C.x + k, camera.m_z_near);

	// Consider YZ plane to find minY and maxY
	vec2 miny, maxy;

	vec2 cy = vec2(C.y, C.z);
	float cyl = length(cy);
	float ty = sqrt(dot(cy, cy) - r * r);
	miny = mat2(ty, r, -r, ty) * (cy / cyl);
	maxy = mat2(ty, -r, r, ty) * (cy / cyl);

	if (isnan(ty) || miny.y < camera.m_z_near) miny = vec2(C.y - k, camera.m_z_near);
	if (isnan(ty) || maxy.y < camera.m_z_near) maxy = vec2(C.y + k, camera.m_z_near);

	// Apply the perspective and convert homogeneous coordinates to euclidean coordinates
	aabb = vec4(
		(minx.x * m00) / minx.y, // min_x
		(miny.x * m11) / miny.y, // min_y
		(maxx.x * m00) / maxx.y, // max_x
		(maxy.x * m11) / maxy.y  // max_y
	);

	// TODO WHEN THE USER IS INSIDE THE SPHERE EVERYTHING BECOMES VERY VERY VERY WRONG

	if (aabb.x > aabb.z || aabb.y > aabb.w || isnan(aabb.x) || isnan(aabb.y) || isnan(aabb.z) || isnan(aabb.w)) {
		assert(false);
		//debugPrintfEXT("INVALID SITUATION -> ID: %d, C: %.2v3f, aabb min: %.2v2f, aabb max: %.2v2f\n", gl_GlobalInvocationID.x, (camera.projection * vec4(C, 1)).xyz, aabb.xy, aabb.zw);
	}

	// Convert the AABB to UV space
	//aabb = vec4(aabb.x, aabb.w, aabb.z, aabb.y) * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f);

	return true;
}

void vren_demo::clusterized_model_debugger::write_debug_info_for_projected_sphere_bounds(
	vren::clusterized_model const& model,
	vren::camera const& camera,
	vren::debug_renderer_draw_buffer& draw_buffer
)
{
	using namespace glm;

	for (uint32_t i = 0; i < model.m_instanced_meshlets.size(); i++)
	{
		vren::instanced_meshlet const& instanced_meshlet = model.m_instanced_meshlets.at(i);

		vren::mesh_instance const& instance = model.m_instances[instanced_meshlet.m_instance_idx];
		vren::meshlet const& meshlet = model.m_meshlets[instanced_meshlet.m_meshlet_idx];

		uint32_t color = std::hash<uint32_t>()(i);

		auto sphere = meshlet.m_bounding_sphere;

		mat4 MV = camera.m_view * instance.m_transform;
		vec3 center = vec3(MV * vec4(sphere.m_center, 1.0f));

		float max_scale = max(length(MV[0]), max(length(MV[1]), length(MV[2])));
		float radius = sphere.m_radius * max_scale;

		vec4 aabb;
		project_sphere(center, radius, camera, aabb);

		draw_buffer.add_line({ .m_from = vec3(aabb.x, aabb.y, 0), .m_to = vec3(aabb.x, aabb.w, 0), .m_color = color });
		draw_buffer.add_line({ .m_from = vec3(aabb.x, aabb.w, 0), .m_to = vec3(aabb.z, aabb.w, 0), .m_color = color });
		draw_buffer.add_line({ .m_from = vec3(aabb.z, aabb.w, 0), .m_to = vec3(aabb.z, aabb.y, 0), .m_color = color });
		draw_buffer.add_line({ .m_from = vec3(aabb.z, aabb.y, 0), .m_to = vec3(aabb.x, aabb.y, 0), .m_color = color });
	}
}
