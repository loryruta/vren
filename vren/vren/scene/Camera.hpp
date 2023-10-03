#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vren
{
    struct Camera
    {
        glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
        float m_yaw = glm::radians(0.0f);
        float m_pitch = glm::radians(0.0f);

        float m_fov_y = glm::radians(45.0f);
        float m_aspect_ratio = 1.0f;
        float m_near_plane = 0.01f;
        float m_far_plane = 1000.0f;

        glm::mat4 orientation() const;

        glm::vec3 forward() const;
        glm::vec3 right() const;
        glm::vec3 up() const;

        glm::mat4 view() const;
        glm::mat4 projection() const;
    };
} // namespace vren
