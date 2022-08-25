#pragma once

#include <optional>

#include <GLFW/glfw3.h>

#include <vren/camera.hpp>

namespace vren_demo
{
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

		void update(vren::camera& camera, float dt, float movement_speed = 1.0f, float rotation_speed = glm::radians(90.0f));
	};
}
