#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <meshoptimizer.h>

namespace vren
{
	// All the following GPU structures are at least 16 bytes aligned (supporting minStorageBufferOffsetAlignment <= 16).

	// ------------------------------------------------------------------------------------------------

	struct camera
	{
		glm::vec3 m_position; float _pad;
		glm::mat4 m_view;
		glm::mat4 m_projection;
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

	using index_t = uint32_t;

	using meshlet_t = meshopt_Meshlet;

	struct instanced_meshlet
	{
		uint32_t m_meshlet_idx;
		uint32_t m_instance_idx;
		uint32_t m_material_idx;
		float _pad;
	};

	struct mesh_instance
	{
		glm::mat4 m_transform;
	};

	// ------------------------------------------------------------------------------------------------
	// Material
	// ------------------------------------------------------------------------------------------------

	struct material
	{
		uint32_t m_base_color_texture_idx;
		uint32_t m_metallic_roughness_texture_idx;
		float _pad[2];
	};

	// ------------------------------------------------------------------------------------------------
	// Lighting
	// ------------------------------------------------------------------------------------------------

	struct point_light
	{
		glm::vec3 m_position; float _pad;
		glm::vec3 m_color;    float _pad1;
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
