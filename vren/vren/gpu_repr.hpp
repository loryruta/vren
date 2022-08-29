#pragma once

#include <glm/glm.hpp>

#include "base/base.hpp" // Because of bounding_sphere

namespace vren
{
	// All the following GPU structures are at least 16 bytes aligned (supporting minStorageBufferOffsetAlignment <= 16).

	// TODO GPU structures alignment should be at least VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT

	// ------------------------------------------------------------------------------------------------

	struct camera_data
	{
		glm::vec3 m_position; float _pad;
		glm::mat4 m_view;
		glm::mat4 m_projection;
		float m_z_near;
		float _pad1[3];
	};

	// ------------------------------------------------------------------------------------------------
	// Geometry
	// ------------------------------------------------------------------------------------------------

	struct vertex
	{
		glm::vec3 m_position;  float _pad;
		glm::vec3 m_normal;    float _pad1;
		glm::vec2 m_texcoords; float _pad2[2];
	};

	struct instanced_meshlet
	{
		uint32_t m_meshlet_idx;
		uint32_t m_instance_idx;
		uint32_t m_material_idx;
	};

	struct mesh_instance
	{
		glm::mat4 m_transform;
	};

	struct meshlet
	{
		uint32_t m_vertex_offset;   // The number of uint32_t to skip before reaching the current meshlet' vertices
		uint32_t m_vertex_count;    // The number of uint32_t held by this meshlet
		uint32_t m_triangle_offset; // The number of uint8_t to skip before reaching the current meshlet' triangles
		uint32_t m_triangle_count;  // The number of triangles of the meshlet (actual uint8_t count will be m_triangle_count * 3)

		vren::bounding_sphere m_bounding_sphere;
	};

	// ------------------------------------------------------------------------------------------------
	// Lighting
	// ------------------------------------------------------------------------------------------------

	struct point_light
	{
		glm::vec3 m_color; float m_intensity;
	};

	struct directional_light
	{
		glm::vec3 m_direction; float _pad;
		glm::vec3 m_color;     float _pad1;
	};

	struct spot_light
	{
		glm::vec3 m_direction; float _pad;
		glm::vec3 m_color;
		float m_radius;
	};
}
