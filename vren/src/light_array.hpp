#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "context.hpp"
#include "descriptor_pool.hpp"
#include "utils/buffer.hpp"

namespace vren
{
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

	// --------------------------------------------------------------------------------------------------------------------------------
	// light_array
	// --------------------------------------------------------------------------------------------------------------------------------

	struct light_array
	{
		std::vector<point_light> m_point_lights;
		std::vector<directional_light> m_directional_lights;
		std::vector<spot_light> m_spot_lights;
	};

	// --------------------------------------------------------------------------------------------------------------------------------
	// light_array_descriptor_pool
	// --------------------------------------------------------------------------------------------------------------------------------

	class light_array_descriptor_pool : public vren::descriptor_pool
	{
	public:
		light_array_descriptor_pool(std::shared_ptr<vren::context> const& ctx);

		VkDescriptorPool create_descriptor_pool(int max_sets);
	};

	// --------------------------------------------------------------------------------------------------------------------------------

	vren::vk_descriptor_set_layout create_light_array_descriptor_set_layout(std::shared_ptr<vren::context> const& ctx);
}