#include "camera.hpp"

#include <glm/gtx/transform.hpp>

// --------------------------------------------------------------------------------------------------------------------------------
// Camera
// --------------------------------------------------------------------------------------------------------------------------------

glm::mat4 vren_demo::camera::get_orientation() const
{
	glm::mat4 m(1);
	m = glm::rotate(m, m_yaw, glm::vec3(0, 1, 0));
	m = glm::rotate(m, -m_pitch, glm::vec3(1, 0, 0));
	return m;
}

glm::vec3 vren_demo::camera::get_right() const
{
	return get_orientation()[0];
}

glm::vec3 vren_demo::camera::get_up() const
{
	return get_orientation()[1];
}

glm::vec3 vren_demo::camera::get_forward() const
{
	return get_orientation()[2];
}

glm::mat4 vren_demo::camera::get_view() const
{
	glm::mat4 inv_s(1), inv_r(1), inv_t(1);
	inv_s = glm::mat3(1.0f / m_zoom);
	inv_r = glm::inverse(get_orientation());
	inv_t = glm::translate(-m_position);
	return inv_s * inv_r * inv_t;
}

glm::mat4 vren_demo::camera::get_projection() const
{
	glm::mat4 m(0);
	float tan_half_fov = glm::tan(m_fov_y / 2.0f);
	m[0][0] = 1.0f / (tan_half_fov * m_aspect_ratio);
	m[1][1] = 1.0f / tan_half_fov;
	m[2][2] = m_far_plane / (m_far_plane - m_near_plane);
	m[2][3] = 1.0f;
	m[3][2] = -(m_far_plane * m_near_plane) / (m_far_plane - m_near_plane);
	return m;
}

// --------------------------------------------------------------------------------------------------------------------------------
// Freecam controller
// --------------------------------------------------------------------------------------------------------------------------------

vren_demo::freecam_controller::freecam_controller(GLFWwindow* window) :
	m_window(window)
{}

void vren_demo::freecam_controller::update(vren_demo::camera& camera, float dt, float movement_speed, float rotation_speed)
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
	if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		step *= 100;
	}

	if (direction != glm::vec3(0)) {
		camera.m_position += glm::normalize(direction) * step;
	}

	// Handle rotation
	glm::dvec2 cursor_position{};
	glfwGetCursorPos(m_window, &cursor_position.x, &cursor_position.y);

	if (m_last_cursor_position.has_value())
	{
		glm::vec2 dr = glm::vec2(cursor_position - m_last_cursor_position.value()) * rotation_speed * dt;
		camera.m_yaw += dr.x;
		camera.m_pitch = glm::clamp(camera.m_pitch - (float) dr.y, -glm::pi<float>() / 4.0f, glm::pi<float>() / 4.0f);
	}

	m_last_cursor_position = cursor_position;
}
