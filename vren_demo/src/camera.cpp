#include "camera.hpp"

#include <glm/gtx/transform.hpp>

glm::mat4 vren_demo::camera::get_orientation() const
{
	auto mat = glm::identity<glm::mat4>();
	mat = glm::rotate(mat, -m_yaw, glm::vec3(0, 1, 0));
	mat = glm::rotate(mat, -m_pitch, glm::vec3(1, 0, 0));
	return mat;
}

glm::vec3 vren_demo::camera::get_forward() const
{
	return get_orientation()[2];
}

glm::vec3 vren_demo::camera::get_right() const
{
	return get_orientation()[0];
}

glm::vec3 vren_demo::camera::get_up() const
{
	return get_orientation()[1];
}

glm::mat4 vren_demo::camera::get_view() const
{
	glm::mat4 inv_s = glm::identity<glm::mat3>() * (1.0f / m_zoom);
	glm::mat4 inv_r = glm::inverse(get_orientation());
	glm::mat4 inv_t = glm::translate(-m_position);
	return inv_s * inv_r * inv_t;
}

glm::mat4 vren_demo::camera::get_projection() const
{
	return glm::perspective(m_fov_y, m_aspect_ratio, m_near_plane, m_far_plane);
}
