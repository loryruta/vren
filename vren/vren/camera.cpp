#include "camera.hpp"

#include <glm/gtx/transform.hpp>

// --------------------------------------------------------------------------------------------------------------------------------
// Camera
// --------------------------------------------------------------------------------------------------------------------------------

glm::mat4 vren::camera::get_orientation() const
{
    glm::mat4 m(1);
    m = glm::rotate(m, m_yaw, glm::vec3(0, 1, 0));
    m = glm::rotate(m, -m_pitch, glm::vec3(1, 0, 0));
    return m;
}

glm::vec3 vren::camera::get_right() const
{
    return get_orientation()[0];
}

glm::vec3 vren::camera::get_up() const
{
    return get_orientation()[1];
}

glm::vec3 vren::camera::get_forward() const
{
    return get_orientation()[2];
}

glm::mat4 vren::camera::get_view() const
{
    glm::mat4 inv_r(1), inv_t(1);
    inv_r = glm::inverse(get_orientation());
    inv_t = glm::translate(-m_position);
    return inv_r * inv_t;
}

glm::mat4 vren::camera::get_projection() const
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
