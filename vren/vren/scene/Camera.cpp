#include "Camera.hpp"

#include <glm/gtx/transform.hpp>

using namespace vren;

glm::mat4 Camera::orientation() const
{
    glm::mat4 m(1);
    m = glm::rotate(m, m_yaw, glm::vec3(0, 1, 0));
    m = glm::rotate(m, -m_pitch, glm::vec3(1, 0, 0));
    return m;
}

glm::vec3 Camera::right() const
{
    return orientation()[0];
}

glm::vec3 Camera::up() const
{
    return orientation()[1];
}

glm::vec3 Camera::forward() const
{
    return orientation()[2];
}

glm::mat4 Camera::view() const
{
    glm::mat4 inv_r(1), inv_t(1);
    inv_r = glm::inverse(orientation());
    inv_t = glm::translate(-m_position);
    return inv_r * inv_t;
}

glm::mat4 Camera::projection() const
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
