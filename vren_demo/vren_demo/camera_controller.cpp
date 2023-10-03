#include "camera_controller.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Freecam controller
// --------------------------------------------------------------------------------------------------------------------------------

vren_demo::freecam_controller::freecam_controller(GLFWwindow* window) :
	m_window(window)
{}

void vren_demo::freecam_controller::update(vren::Camera& camera, float dt, float movement_speed, float rotation_speed)
{
	if (glfwGetInputMode(m_window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
	{
		if (m_last_cursor_position) {
			m_last_cursor_position.reset();
		}
		return;
	}

	const glm::vec4 k_world_up = glm::vec4(0, 1, 0, 0);

	// Handle movement
	glm::vec3 direction{};
	if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) direction += camera.get_forward();
	if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) direction += -camera.get_forward();
	if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) direction += -camera.get_right();
	if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) direction += camera.get_right();
	if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) direction += camera.get_up();
	if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) direction += -camera.get_up();

	float step = movement_speed * dt;
	if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
	{
		step *= 100;
	}

	if (direction != glm::vec3(0))
	{
		camera.m_position += glm::normalize(direction) * step;
	}

	// Handle rotation
	glm::dvec2 cursor_position{};
	glfwGetCursorPos(m_window, &cursor_position.x, &cursor_position.y);

	if (m_last_cursor_position.has_value())
	{
		glm::vec2 dr = glm::vec2(cursor_position - m_last_cursor_position.value()) * rotation_speed * dt;
		camera.m_yaw += dr.x;
		camera.m_pitch = glm::clamp(camera.m_pitch - (float)dr.y, -glm::pi<float>() / 4.0f, glm::pi<float>() / 4.0f);
	}

	m_last_cursor_position = cursor_position;
}
