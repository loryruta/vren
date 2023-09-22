#include "clusterized_model_uploader.hpp"

vren::clusterized_model_draw_buffer
vren::clusterized_model_uploader::upload(vren::context const& context, vren::clusterized_model const& clusterized_model)
{
    auto vertex_buffer = vren::vk_utils::create_device_only_buffer(
        context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        clusterized_model.m_vertices.data(),
        clusterized_model.m_vertices.size() * sizeof(vren::vertex)
    );

    auto meshlet_vertex_buffer = vren::vk_utils::create_device_only_buffer(
        context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        clusterized_model.m_meshlet_vertices.data(),
        clusterized_model.m_meshlet_vertices.size() * sizeof(uint32_t)
    );

    auto meshlet_triangle_buffer = vren::vk_utils::create_device_only_buffer(
        context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        clusterized_model.m_meshlet_triangles.data(),
        clusterized_model.m_meshlet_triangles.size() * 3 * sizeof(uint8_t)
    );

    auto meshlet_buffer = vren::vk_utils::create_device_only_buffer(
        context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        clusterized_model.m_meshlets.data(),
        clusterized_model.m_meshlets.size() * sizeof(vren::meshlet)
    );

    auto instanced_meshlet_buffer = vren::vk_utils::create_device_only_buffer(
        context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        clusterized_model.m_instanced_meshlets.data(),
        clusterized_model.m_instanced_meshlets.size() * sizeof(vren::instanced_meshlet)
    );

    auto instance_buffer = vren::vk_utils::create_device_only_buffer(
        context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        clusterized_model.m_instances.data(),
        clusterized_model.m_instances.size() * sizeof(vren::mesh_instance)
    );

    vren::clusterized_model_draw_buffer draw_buffer{
        .m_name = clusterized_model.m_name,
        .m_vertex_buffer = std::move(vertex_buffer),
        .m_meshlet_vertex_buffer = std::move(meshlet_vertex_buffer),
        .m_meshlet_triangle_buffer = std::move(meshlet_triangle_buffer),
        .m_meshlet_buffer = std::move(meshlet_buffer),
        .m_instanced_meshlet_buffer = std::move(instanced_meshlet_buffer),
        .m_instanced_meshlet_count = clusterized_model.m_instanced_meshlets.size(),
        .m_instance_buffer = std::move(instance_buffer)};

    draw_buffer.set_object_names(context);

    VREN_INFO("[clusterized_model_uploader] Uploading model: {}\n", draw_buffer.m_name);

    return std::move(draw_buffer);
}
