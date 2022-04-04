#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "pools/descriptor_pool.hpp"
#include "vk_helpers/buffer.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------

	struct point_light
	{
		glm::vec3 m_position; float _pad;
		glm::vec3 m_color; float _pad_1;
	};

	struct directional_light
	{
		glm::vec3 m_direction; float _pad;
		glm::vec3 m_color; float _pad_1;
	};

	struct spot_light
	{
		glm::vec3 m_direction; float _pad;
		glm::vec3 m_color;
		float m_radius;
	};

	// ------------------------------------------------------------------------------------------------
	// Light array
	// ------------------------------------------------------------------------------------------------

	struct light_array
	{
		std::vector<point_light> m_point_lights;
		std::vector<directional_light> m_directional_lights;
		std::vector<spot_light> m_spot_lights;
	};

	// ------------------------------------------------------------------------------------------------

	vren::vk_descriptor_set_layout create_light_array_descriptor_set_layout(vren::context const& ctx);
}
