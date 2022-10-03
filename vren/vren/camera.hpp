#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vren
{
	// ------------------------------------------------------------------------------------------------
	// Camera
	// ------------------------------------------------------------------------------------------------

	struct camera
	{
		glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
		float m_yaw = glm::radians(0.0f);
		float m_pitch = glm::radians(0.0f);

		float m_fov_y = glm::radians(45.0f);
		float m_aspect_ratio = 1.0f;
		float m_near_plane = 0.01f;
		float m_far_plane = 1000.0f;

		glm::mat4 get_orientation() const;

		glm::vec3 get_forward() const;
		glm::vec3 get_right() const;
		glm::vec3 get_up() const;

		glm::mat4 get_view() const;
		glm::mat4 get_projection() const;
	};
}
