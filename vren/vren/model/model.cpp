#include "model.hpp"

#include <algorithm>
#include <execution>

#include <glm/glm.hpp>

void vren::model::compute_aabb()
{
    std::for_each(std::execution::par_unseq, m_meshes.begin(), m_meshes.end(), [this](vren::model::mesh& mesh)
    {
        mesh.m_min = glm::vec3(std::numeric_limits<float>::infinity());
        mesh.m_max = glm::vec3(-std::numeric_limits<float>::infinity());

        for (uint32_t ii = mesh.m_instance_offset; ii < mesh.m_instance_offset + mesh.m_instance_count; ii++)
        {
            for (uint32_t vi = mesh.m_vertex_offset; vi < mesh.m_vertex_offset + mesh.m_vertex_count; vi++)
            {
                glm::vec3 transformed_vertex = m_instances.at(ii).m_transform * glm::vec4(m_vertices.at(vi).m_position, 1);
                mesh.m_min = glm::min(transformed_vertex, mesh.m_min);
                mesh.m_max = glm::max(transformed_vertex, mesh.m_max);
            }
        }
    });

    m_min = glm::vec3(std::numeric_limits<float>::infinity());
    m_max = glm::vec3(-std::numeric_limits<float>::infinity());

    for (vren::model::mesh const& mesh : m_meshes)
    {
        m_min = glm::min(mesh.m_min, m_min);
        m_max = glm::max(mesh.m_max, m_max);
    }
}
