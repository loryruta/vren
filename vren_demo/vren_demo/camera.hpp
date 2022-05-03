#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include <vren/gpu_repr.hpp>

namespace vren_demo
{
	// ------------------------------------------------------------------------------------------------
	// Camera
	// ------------------------------------------------------------------------------------------------

	struct camera
	{
		glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
		float m_yaw = glm::radians(0.0f);
		float m_pitch = glm::radians(0.0f);
		float m_zoom = 1.0f;

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

		inline vren::camera to_vren()
		{
			return {
				.m_position = m_position,
				.m_view = get_view(),
				.m_projection = get_projection()
			};
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Freecam controller
	// ------------------------------------------------------------------------------------------------

	class freecam_controller
	{
	private:
		GLFWwindow* m_window;
		std::optional<glm::dvec2> m_last_cursor_position;

	public:
		freecam_controller(GLFWwindow* window);

		void update(vren_demo::camera& camera, float dt, float movement_speed = 1.0f, float rotation_speed = glm::radians(90.0f));
	};
}
